// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/BlenderRetouch.h"

#include <system_error>

#include "Core/Resources/SkeletonFile.h"

namespace hmi {

namespace {

constexpr const char* SCRIPT = "scripts/assetsGeneration/retouch_character.py";
constexpr const char* RETOUCH_FILE = "retouche.json";
// L'emplacement de Blender sur le poste de production : celui de `reduce_model.py`.
constexpr const char* DEFAULT_BLENDER = "D:/Blender Foundation/Blender 5.2/blender.exe";

[[nodiscard]] bool isFile(const std::filesystem::path& path) {
    std::error_code error;
    return std::filesystem::is_regular_file(path, error);
}

}  // namespace

RetouchFiles retouchFiles(const std::filesystem::path& dataRoot,
                          const std::filesystem::path& baseDirectory, const CharacterDraft& draft) {
    const std::filesystem::path root = (baseDirectory / draft.root).lexically_normal();
    const auto resolve = [&root](const std::string& written) {
        return written.empty() ? std::filesystem::path{} : (root / written).lexically_normal();
    };
    RetouchFiles files;
    files.model = resolve(draft.model);
    files.received = resolve(draft.workshop.received);
    files.sheet = resolve(draft.workshop.sheet);
    files.retouch = resolve(draft.workshop.retouch);
    if (files.retouch.empty() && !files.sheet.empty()) {
        files.retouch = files.sheet.parent_path() / RETOUCH_FILE;
    }
    files.blend = resolve(draft.workshop.blend);
    if (files.blend.empty() && !files.model.empty()) {
        files.blend = std::filesystem::path(files.model).replace_extension(".blend");
    }
    if (!draft.skeleton.empty()) {
        files.skeleton =
            dataRoot / "Assets" / std::filesystem::path(core::skeletonFilePath(draft.skeleton));
    }
    return files;
}

std::string retouchReadiness(const RetouchFiles& files) {
    if (files.model.empty()) {
        return "name the linked model first";
    }
    if (!isFile(files.model)) {
        return "linked model not found: " + files.model.generic_string();
    }
    if (files.received.empty() || !isFile(files.received)) {
        return "name the received model (workshop): the import links it again";
    }
    if (files.sheet.empty() || !isFile(files.sheet)) {
        return "name the liaison sheet (workshop): moved joints are written to it";
    }
    if (!isFile(files.skeleton)) {
        return "skeleton not installed: " + files.skeleton.generic_string();
    }
    return {};
}

RetouchTools findRetouchTools(const std::filesystem::path& dataRoot,
                              const EnvironmentLookup& environment, std::string& error) {
    RetouchTools tools;
    error.clear();
    // Le script : à la racine du dépôt, que la racine des données a pour ancêtre.
    std::error_code ignored;
    std::filesystem::path folder = std::filesystem::weakly_canonical(dataRoot, ignored);
    if (ignored) {
        folder = dataRoot;
    }
    while (true) {
        if (isFile(folder / SCRIPT)) {
            tools.script = folder / SCRIPT;
            break;
        }
        const std::filesystem::path parent = folder.parent_path();
        if (parent.empty() || parent == folder) {
            break;
        }
        folder = parent;
    }
    if (const std::optional<std::string> python = environment("JADG_PYTHON");
        python && !python->empty()) {
        tools.python = *python;
    } else {
#ifdef _WIN32
        tools.python = "py";
        tools.pythonArguments = {"-3"};
#else
        tools.python = "python3";
#endif
    }
    if (const std::optional<std::string> blender = environment("BLENDER");
        blender && !blender->empty()) {
        tools.blender = *blender;
    } else if (isFile(DEFAULT_BLENDER)) {
        tools.blender = DEFAULT_BLENDER;
    }
    if (tools.script.empty()) {
        error = std::string{"script not found above the data root: "} + SCRIPT;
    } else if (tools.blender.empty() || !isFile(tools.blender)) {
        error = "Blender not found: set the BLENDER environment variable to blender.exe";
    }
    return tools;
}

namespace {

[[nodiscard]] ProcessCommand scriptCommand(const RetouchTools& tools, const char* verb) {
    ProcessCommand command;
    command.program = tools.python;
    command.arguments = tools.pythonArguments;
    command.arguments.push_back(tools.script.generic_string());
    command.arguments.emplace_back(verb);
    return command;
}

void appendTools(ProcessCommand& command, const RetouchTools& tools, const RetouchFiles& files) {
    command.arguments.insert(command.arguments.end(),
                             {"--skeleton", files.skeleton.generic_string(), "--blender",
                              tools.blender.generic_string()});
}

}  // namespace

ProcessCommand openInBlenderCommand(const RetouchTools& tools, const RetouchFiles& files) {
    ProcessCommand command = scriptCommand(tools, "open");
    command.arguments.insert(command.arguments.end(), {files.model.generic_string(), "--blend",
                                                       files.blend.generic_string()});
    appendTools(command, tools, files);
    return command;
}

ProcessCommand importFromBlenderCommand(const RetouchTools& tools, const RetouchFiles& files) {
    ProcessCommand command = scriptCommand(tools, "import");
    command.arguments.insert(
        command.arguments.end(),
        {files.blend.generic_string(), "--sheet", files.sheet.generic_string(), "--retouch",
         files.retouch.generic_string(), "--source", files.received.generic_string(), "--output",
         files.model.generic_string()});
    appendTools(command, tools, files);
    return command;
}

}  // namespace hmi
