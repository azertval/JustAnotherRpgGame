// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/QuestEditing.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
#include <iterator>
#include <set>
#include <sstream>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <variant>

#include <nlohmann/json.hpp>

#include "Core/Gameplay/WorldFlags.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/WorldGraph.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/MapTexts.h"

namespace hmi {

namespace {

[[nodiscard]] std::string readText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

[[nodiscard]] std::filesystem::path questsDirectory(const std::filesystem::path& dataRoot) {
    return dataRoot / "World" / "quests";
}

[[nodiscard]] std::filesystem::path dialoguesDirectory(const std::filesystem::path& dataRoot) {
    return dataRoot / "World" / "dialogues";
}

// Les fichiers `*.json` d'un dossier, triés ; aucun si le dossier manque.
[[nodiscard]] std::vector<std::filesystem::path> jsonFiles(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    for (auto entry = std::filesystem::directory_iterator(directory, error);
         !error && entry != std::filesystem::directory_iterator(); entry.increment(error)) {
        if (entry->is_regular_file() && entry->path().extension() == ".json") {
            files.push_back(entry->path());
        }
    }
    std::ranges::sort(files);
    return files;
}

[[nodiscard]] RefactorPlan refused(std::string error) {
    RefactorPlan plan;
    plan.error = std::move(error);
    return plan;
}

[[nodiscard]] std::string joined(const std::vector<std::string>& parts,
                                 std::string_view separator) {
    std::string text;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        text += (i == 0 ? "" : std::string{separator}) + parts[i];
    }
    return text;
}

// --- Les chaînes d'un document JSON, par leur pointeur -------------------------------------------

// Où se tient une chaîne dans le texte : ses guillemets compris.
struct StringSpan {
    std::size_t begin = 0;
    std::size_t end = 0;
};

// Parcourt un texte JSON **déjà reconnu valide** et note la place de chaque chaîne qui est une
// valeur, par son pointeur (`/nodes/3/actions/0/value`). C'est ce qui permet de changer une valeur
// d'un dialogue écrit à la main sans toucher à un autre octet.
class StringSpanScanner {
public:
    explicit StringSpanScanner(std::string_view text) : _text(text) {}

    [[nodiscard]] std::map<std::string, StringSpan, std::less<>> scan() {
        value({});
        return std::move(_spans);
    }

private:
    void skipSpaces() {
        while (_at < _text.size() && std::isspace(static_cast<unsigned char>(_text[_at])) != 0) {
            ++_at;
        }
    }

    [[nodiscard]] StringSpan string() {
        const std::size_t begin = _at++;
        while (_at < _text.size() && _text[_at] != '"') {
            _at += _text[_at] == '\\' ? 2 : 1;
        }
        ++_at;
        return {.begin = begin, .end = std::min(_at, _text.size())};
    }

    // Le pointeur d'une clé : `~` et `/` échappés (RFC 6901).
    [[nodiscard]] static std::string escaped(const std::string& key) {
        std::string result;
        for (const char c : key) {
            result += c == '~' ? "~0" : c == '/' ? "~1" : std::string(1, c);
        }
        return result;
    }

    void value(const std::string& pointer) {
        skipSpaces();
        if (_at >= _text.size()) {
            return;
        }
        const char c = _text[_at];
        if (c == '{') {
            members(pointer);
        } else if (c == '[') {
            elements(pointer);
        } else if (c == '"') {
            _spans[pointer] = string();
        } else {
            while (_at < _text.size() &&
                   std::string_view{",]} \t\r\n"}.find(_text[_at]) == std::string_view::npos) {
                ++_at;
            }
        }
    }

    void members(const std::string& pointer) {
        ++_at;
        for (;;) {
            skipSpaces();
            if (_at >= _text.size() || _text[_at] == '}') {
                ++_at;
                return;
            }
            const StringSpan key = string();
            const nlohmann::json name =
                nlohmann::json::parse(_text.substr(key.begin, key.end - key.begin), nullptr, false);
            skipSpaces();
            ++_at;  // ':'
            value(pointer + "/" + escaped(name.is_string() ? name.get<std::string>() : ""));
            skipSpaces();
            if (_at < _text.size() && _text[_at] == ',') {
                ++_at;
            }
        }
    }

    void elements(const std::string& pointer) {
        ++_at;
        for (std::size_t index = 0;; ++index) {
            skipSpaces();
            if (_at >= _text.size() || _text[_at] == ']') {
                ++_at;
                return;
            }
            value(pointer + "/" + std::to_string(index));
            skipSpaces();
            if (_at < _text.size() && _text[_at] == ',') {
                ++_at;
            }
        }
    }

    std::string_view _text;
    std::size_t _at = 0;
    std::map<std::string, StringSpan, std::less<>> _spans;
};

// @p text où chaque chaîne nommée par son pointeur prend sa nouvelle valeur ; rien d'autre ne
// bouge.
[[nodiscard]] std::string withStrings(
    const std::string& text, const std::vector<std::pair<std::string, std::string>>& edits) {
    const std::map<std::string, StringSpan, std::less<>> spans = StringSpanScanner(text).scan();
    std::vector<std::pair<StringSpan, std::string>> placed;
    for (const auto& [pointer, value] : edits) {
        if (const auto found = spans.find(pointer); found != spans.end()) {
            placed.emplace_back(found->second, nlohmann::json(value).dump());
        }
    }
    // De la fin vers le début : une chaîne remplacée ne décale pas les suivantes.
    std::ranges::sort(placed,
                      [](const auto& a, const auto& b) { return a.first.begin > b.first.begin; });
    std::string result = text;
    for (const auto& [span, replacement] : placed) {
        result.replace(span.begin, span.end - span.begin, replacement);
    }
    return result;
}

// --- Les endroits qui se servent d'un drapeau
// -----------------------------------------------------

// Reçoit chaque usage ; rend vrai s'il l'a changé (drapeau ou valeurs), et le parcours l'écrit.
using UseVisitor = std::function<bool(FlagUse&)>;

// Une quête renommée : son fichier, et les dialogues qui la démarrent.
struct QuestIdRenaming {
    std::string from;
    std::string to;
};

// Ce que @p visit a changé d'un usage, comparé à ce qu'il était.
[[nodiscard]] bool visitUse(const UseVisitor& visit, FlagUse& use) {
    const FlagUse before = use;
    return visit(use) && (use.flag != before.flag || use.values != before.values);
}

// Un usage qui ne se récrit pas : le fait qu'un jet raté, une victoire ou une étape pose
// d'eux-mêmes.
void visitDerived(const UseVisitor& visit, std::string flag, Citation where) {
    FlagUse use{.flag = std::move(flag),
                .values = {},
                .role = FlagUseRole::Writes,
                .where = std::move(where)};
    static_cast<void>(visit(use));
}

// Les quêtes. @return Le refus, si @p plan est donné et qu'une quête ne se lit pas.
std::string visitQuests(const std::filesystem::path& dataRoot, const UseVisitor& visit,
                        RefactorPlan* plan, const QuestIdRenaming* renaming = nullptr) {
    for (const std::filesystem::path& file : jsonFiles(questsDirectory(dataRoot))) {
        core::QuestLoad loaded = core::loadQuest(file);
        if (!loaded.quest) {
            if (plan != nullptr) {
                return "quest " + file.stem().string() +
                       " cannot be read: " + joined(loaded.errors, "; ");
            }
            continue;
        }
        core::Quest& quest = *loaded.quest;
        const std::string questId = quest.id;
        const auto cite = [&file, &questId](const std::string& what) {
            return Citation{.file = file, .what = "quest " + questId + ": " + what};
        };
        bool changed = false;
        const auto changedAt = [&changed, plan](const Citation& where) {
            changed = true;
            if (plan != nullptr) {
                plan->changes.push_back(where);
            }
        };
        for (core::QuestFlag& declared : quest.flags) {
            FlagUse use{.flag = declared.id,
                        .values = declared.values,
                        .role = FlagUseRole::Declares,
                        .where = cite("flags")};
            if (visitUse(visit, use)) {
                const auto initial = std::ranges::find(declared.values, declared.initial);
                if (initial != declared.values.end() &&
                    use.values.size() == declared.values.size()) {
                    declared.initial =
                        use.values[static_cast<std::size_t>(initial - declared.values.begin())];
                }
                declared.id = use.flag;
                declared.values = use.values;
                changedAt(use.where);
            }
        }
        for (core::QuestStep& step : quest.steps) {
            for (core::FlagCondition& condition : step.when) {
                FlagUse use{.flag = condition.flag,
                            .values = condition.values,
                            .role = FlagUseRole::Reads,
                            .where = cite("step " + step.id + ": when")};
                if (visitUse(visit, use)) {
                    condition.flag = use.flag;
                    condition.values = use.values;
                    changedAt(use.where);
                }
            }
            for (core::QuestEffect& effect : step.effects) {
                FlagUse use{.flag = effect.flag,
                            .values = effect.value.empty() ? std::vector<std::string>{}
                                                           : std::vector<std::string>{effect.value},
                            .role = FlagUseRole::Writes,
                            .where = cite("step " + step.id + ": effects")};
                if (visitUse(visit, use)) {
                    effect.flag = use.flag;
                    effect.value = use.values.empty() ? std::string{} : use.values.front();
                    changedAt(use.where);
                }
            }
            visitDerived(visit, core::questStepFlag(questId, step.id), cite("step " + step.id));
        }
        if (plan == nullptr) {
            continue;
        }
        if (renaming != nullptr && questId == renaming->from) {
            quest.id = renaming->to;
            plan->changes.push_back(cite("id"));
            plan->edits.push_back(
                ProjectEdit{.file = questsDirectory(dataRoot) / (quest.id + ".json"),
                            .text = core::writeQuest(quest)});
            plan->edits.push_back(ProjectEdit{.file = file, .text = std::nullopt});
        } else if (changed) {
            plan->edits.push_back(ProjectEdit{.file = file, .text = core::writeQuest(quest)});
        }
    }
    return {};
}

// Un dialogue, lu pour être parcouru et, au besoin, récrit chaîne par chaîne.
class DialogueVisit {
public:
    DialogueVisit(std::filesystem::path file, std::string id, const UseVisitor& visit,
                  RefactorPlan* plan)
        : _file(std::move(file)), _id(std::move(id)), _visit(visit), _plan(plan) {}

    // Une condition `{flag, equals|notEquals}` à @p pointer.
    void condition(const nlohmann::json& object, const std::string& pointer,
                   const std::string& what) {
        if (!object.is_object() || !object.contains("flag") || !object["flag"].is_string()) {
            return;
        }
        FlagUse use{.flag = object["flag"].get<std::string>(),
                    .values = {},
                    .role = FlagUseRole::Reads,
                    .where = cite(what)};
        std::vector<std::string> pointers;
        for (const char* key : {"equals", "notEquals"}) {
            if (!object.contains(key)) {
                continue;
            }
            const nlohmann::json& values = object[key];
            if (values.is_string()) {
                use.values.push_back(values.get<std::string>());
                pointers.push_back(pointer + "/" + key);
            } else if (values.is_array()) {
                for (std::size_t k = 0; k < values.size(); ++k) {
                    if (values[k].is_string()) {
                        use.values.push_back(values[k].get<std::string>());
                        pointers.push_back(pointer + "/" + key + "/" + std::to_string(k));
                    }
                }
            }
        }
        rewrite(use, pointer + "/flag", pointers);
    }

    // Une action `setFlag` / `clearFlag` à @p pointer.
    void action(const nlohmann::json& object, const std::string& pointer, const std::string& what) {
        if (!object.contains("flag") || !object["flag"].is_string()) {
            return;
        }
        FlagUse use{.flag = object["flag"].get<std::string>(),
                    .values = {},
                    .role = FlagUseRole::Writes,
                    .where = cite(what)};
        std::vector<std::string> pointers;
        if (object.contains("value") && object["value"].is_string()) {
            use.values.push_back(object["value"].get<std::string>());
            pointers.push_back(pointer + "/value");
        }
        rewrite(use, pointer + "/flag", pointers);
    }

    void derived(std::string flag, const std::string& what) {
        visitDerived(_visit, std::move(flag), cite(what));
    }

    // Une chaîne changée hors de tout drapeau : la quête qu'une action démarre.
    void replace(const std::string& pointer, std::string value, const std::string& what) {
        _edits.emplace_back(pointer, std::move(value));
        if (_plan != nullptr) {
            _plan->changes.push_back(cite(what));
        }
    }

    [[nodiscard]] Citation cite(const std::string& what) const {
        return Citation{.file = _file, .what = "dialogue " + _id + ": " + what};
    }

    // Écrit ce qui a changé dans le plan.
    void finish(const std::string& text) {
        if (_plan != nullptr && !_edits.empty()) {
            _plan->edits.push_back(ProjectEdit{.file = _file, .text = withStrings(text, _edits)});
        }
    }

private:
    void rewrite(FlagUse& use, const std::string& flagPointer,
                 const std::vector<std::string>& valuePointers) {
        const FlagUse before = use;
        if (!visitUse(_visit, use)) {
            return;
        }
        if (use.flag != before.flag) {
            _edits.emplace_back(flagPointer, use.flag);
        }
        for (std::size_t k = 0; k < valuePointers.size() && k < use.values.size(); ++k) {
            if (use.values[k] != before.values[k]) {
                _edits.emplace_back(valuePointers[k], use.values[k]);
            }
        }
        if (_plan != nullptr) {
            _plan->changes.push_back(use.where);
        }
    }

    std::filesystem::path _file;
    std::string _id;
    const UseVisitor& _visit;
    RefactorPlan* _plan;
    std::vector<std::pair<std::string, std::string>> _edits;
};

void visitDialogueNode(DialogueVisit& dialogue, const std::string& dialogueId,
                       const nlohmann::json& node, const std::string& base,
                       const QuestIdRenaming* renaming) {
    const std::string nodeId = node.value("id", std::string{});
    const std::string what = "node " + nodeId;
    const std::string type = node.value("type", std::string{});
    if (type == "condition") {
        dialogue.condition(node, base, what);
    } else if (type == "check") {
        // Un jet raté pose son fait de lui-même (LOT-117) : une quête le lit.
        dialogue.derived(core::dialogueCheckFailedFlag(dialogueId, nodeId), what);
    }
    if (const auto choices = node.find("choices"); choices != node.end() && choices->is_array()) {
        for (std::size_t j = 0; j < choices->size(); ++j) {
            const nlohmann::json& choice = (*choices)[j];
            if (choice.is_object() && choice.contains("condition")) {
                dialogue.condition(choice["condition"],
                                   base + "/choices/" + std::to_string(j) + "/condition",
                                   what + ": choice " + choice.value("id", std::string{}));
            }
        }
    }
    const auto actions = node.find("actions");
    if (actions == node.end() || !actions->is_array()) {
        return;
    }
    for (std::size_t k = 0; k < actions->size(); ++k) {
        const nlohmann::json& action = (*actions)[k];
        if (!action.is_object()) {
            continue;
        }
        const std::string kind = action.value("type", std::string{});
        const std::string pointer = base + "/actions/" + std::to_string(k);
        if (kind == "setFlag" || kind == "clearFlag") {
            dialogue.action(action, pointer, what + ": " + kind);
        } else if (kind == "startQuest") {
            const std::string quest = action.value("quest", std::string{});
            dialogue.derived(core::questStartedFlag(quest), what + ": startQuest");
            if (renaming != nullptr && quest == renaming->from) {
                dialogue.replace(pointer + "/quest", renaming->to, what + ": startQuest");
            }
        } else if (kind == "startEncounter") {
            dialogue.derived(core::encounterWonFlag(action.value("encounter", std::string{})),
                             what + ": startEncounter");
        }
    }
}

// Les dialogues : leurs conditions, leurs actions, et les faits qu'ils posent d'eux-mêmes.
void visitDialogues(const std::filesystem::path& dataRoot, const UseVisitor& visit,
                    RefactorPlan* plan, const QuestIdRenaming* renaming = nullptr) {
    for (const std::filesystem::path& file : jsonFiles(dialoguesDirectory(dataRoot))) {
        const std::string text = readText(file);
        const nlohmann::json json =
            nlohmann::json::parse(text, nullptr, /*allow_exceptions=*/false);
        if (!json.is_object()) {
            continue;
        }
        const std::string id = json.value("id", file.stem().string());
        DialogueVisit dialogue(file, id, visit, plan);
        if (const auto nodes = json.find("nodes"); nodes != json.end() && nodes->is_array()) {
            for (std::size_t i = 0; i < nodes->size(); ++i) {
                if ((*nodes)[i].is_object()) {
                    visitDialogueNode(dialogue, id, (*nodes)[i], "/nodes/" + std::to_string(i),
                                      renaming);
                }
            }
        }
        dialogue.finish(text);
    }
}

// Les propriétés que l'inspecteur montre pour l'entité : celles de sa famille, puis les communes ;
// les communes seules pour une famille inconnue (la présence vaut pour toute entité).
[[nodiscard]] std::vector<const core::EntityPropertySpec*> specsOf(const core::MapEntity& entity) {
    if (const core::EntityKind* kind = core::findEntityKind(entity.type)) {
        return core::inspectedProperties(*kind);
    }
    std::vector<const core::EntityPropertySpec*> specs;
    for (const core::EntityPropertySpec& spec : core::commonEntityProperties()) {
        specs.push_back(&spec);
    }
    return specs;
}

[[nodiscard]] std::string* textProperty(core::MapEntity& entity, std::string_view key) {
    const auto found = entity.properties.find(std::string{key});
    return found != entity.properties.end() ? std::get_if<std::string>(&found->second) : nullptr;
}

// Les drapeaux d'une entité, par la source de ses propriétés (règle 2). @return Vrai si changée.
[[nodiscard]] bool visitEntity(core::MapEntity& entity, const std::string& mapId,
                               const std::filesystem::path& file, const UseVisitor& visit,
                               RefactorPlan* plan) {
    bool changed = false;
    const std::vector<const core::EntityPropertySpec*> specs = specsOf(entity);
    for (const core::EntityPropertySpec* spec : specs) {
        if (spec->source != core::EntityChoiceSource::Flags &&
            spec->source != core::EntityChoiceSource::WrittenFlags) {
            continue;
        }
        std::string* flag = textProperty(entity, spec->key);
        if (flag == nullptr || flag->empty()) {
            continue;
        }
        const auto valueSpec = std::ranges::find_if(specs, [spec](const auto* candidate) {
            return candidate->source == core::EntityChoiceSource::FlagValues &&
                   candidate->relatedKey == spec->key;
        });
        std::string* values =
            valueSpec != specs.end() ? textProperty(entity, (*valueSpec)->key) : nullptr;
        FlagUse use{
            .flag = *flag,
            .values =
                values != nullptr ? core::splitFlagValues(*values) : std::vector<std::string>{},
            .role = spec->source == core::EntityChoiceSource::WrittenFlags ? FlagUseRole::Writes
                                                                           : FlagUseRole::Reads,
            .where =
                Citation{.file = file,
                         .mapId = mapId,
                         .entityId = entity.id,
                         .cell = entity.position,
                         .what = entity.type + " " + entity.id + ": " + std::string{spec->key}}};
        const std::vector<std::string> before = use.values;
        if (!visitUse(visit, use)) {
            continue;
        }
        *flag = use.flag;
        if (values != nullptr && use.values != before) {
            *values = joined(use.values, "|");
        }
        changed = true;
        if (plan != nullptr) {
            plan->changes.push_back(use.where);
        }
    }
    return changed;
}

// Les cartes. @return Le refus, si @p plan est donné et qu'une carte ne se lit pas : ses usages ne
// se verraient pas.
std::string visitMaps(const std::filesystem::path& dataRoot, const UseVisitor& visit,
                      RefactorPlan* plan) {
    const std::filesystem::path levels = dataRoot / "Levels";
    for (const std::filesystem::path& file : mapFiles(dataRoot)) {
        const std::string mapId = core::mapIdOf(levels, file);
        const core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(file);
        if (!loaded.ok()) {
            if (plan != nullptr) {
                return mapId + " cannot be read: " + loaded.error;
            }
            continue;
        }
        core::LevelData data = loaded.level->data();
        bool changed = false;
        for (core::MapEntity& entity : data.entities) {
            changed = visitEntity(entity, mapId, file, visit, plan) || changed;
        }
        if (changed && plan != nullptr) {
            plan->edits.push_back(
                ProjectEdit{.file = file, .text = core::LevelWriter::buildJson(data)});
        }
    }
    return {};
}

// Récrit tout le projet par @p visit : quêtes, dialogues, cartes.
[[nodiscard]] RefactorPlan rewriteProject(const std::filesystem::path& dataRoot,
                                          const UseVisitor& visit,
                                          const QuestIdRenaming* renaming = nullptr) {
    RefactorPlan plan;
    if (std::string error = visitMaps(dataRoot, visit, &plan); !error.empty()) {
        return refused(std::move(error));
    }
    if (std::string error = visitQuests(dataRoot, visit, &plan, renaming); !error.empty()) {
        return refused(std::move(error));
    }
    visitDialogues(dataRoot, visit, &plan, renaming);
    return plan;
}

[[nodiscard]] bool contains(const std::vector<std::string>& values, std::string_view value) {
    return std::ranges::find(values, value) != values.end();
}

// Le drapeau déclaré @p flag, dans les quêtes de @p dataRoot ; `std::nullopt` sinon.
[[nodiscard]] std::optional<core::QuestFlag> declaredFlag(const std::filesystem::path& dataRoot,
                                                          std::string_view flag) {
    const core::QuestCatalog quests = core::loadQuests(questsDirectory(dataRoot));
    const core::QuestFlag* found = quests.findFlag(flag);
    return found != nullptr ? std::optional<core::QuestFlag>{*found} : std::nullopt;
}

// Les textes d'une quête, clé par clé, tirés d'un catalogue.
[[nodiscard]] QuestLanguageTexts textsFrom(
    const std::unordered_map<std::string, std::string>& catalog, const core::Quest& quest) {
    QuestLanguageTexts texts;
    for (const std::string& key : core::questTextKeys(quest)) {
        if (const auto found = catalog.find(key); found != catalog.end()) {
            texts.emplace(key, found->second);
        }
    }
    return texts;
}

// Le préfixe des clés de journal d'une quête : `quest.pommes.`.
[[nodiscard]] std::string questKeyPrefix(std::string_view questId) {
    return "quest." + std::string{questId} + ".";
}

}  // namespace

std::filesystem::path questFile(const std::filesystem::path& dataRoot, std::string_view questId) {
    return questsDirectory(dataRoot) / (std::string{questId} + ".json");
}

std::vector<std::string> questIds(const std::filesystem::path& dataRoot) {
    std::vector<std::string> ids;
    for (const std::filesystem::path& file : jsonFiles(questsDirectory(dataRoot))) {
        ids.push_back(file.stem().string());
    }
    return ids;
}

QuestDraftLoad loadQuestDraft(const std::filesystem::path& dataRoot, std::string_view questId) {
    QuestDraftLoad result;
    core::QuestLoad loaded = core::loadQuest(questFile(dataRoot, questId));
    if (!loaded.quest) {
        result.errors = std::move(loaded.errors);
        return result;
    }
    QuestDraft draft{.quest = std::move(*loaded.quest), .texts = {}};
    for (const auto& [language, catalog] :
         loadTranslationCatalogs(localizationDirectory(dataRoot))) {
        draft.texts.emplace(language, textsFrom(catalog, draft.quest));
    }
    result.draft = std::move(draft);
    return result;
}

bool isValidQuestName(std::string_view id, bool allowSlash) {
    if (id.empty() || id.front() == '/' || id.back() == '/') {
        return false;
    }
    return std::ranges::all_of(id, [allowSlash](const char c) {
        const auto u = static_cast<unsigned char>(c);
        return (std::islower(u) != 0) || (std::isdigit(u) != 0) || c == '-' || c == '_' ||
               (allowSlash && (c == '/' || c == '.'));
    });
}

RefactorPlan planSaveQuest(const std::filesystem::path& dataRoot, const QuestDraft& draft,
                           bool isNew) {
    const core::Quest& quest = draft.quest;
    if (!isValidQuestName(quest.id)) {
        return refused("\"" + quest.id +
                       "\" is not a valid quest id (lowercase letters, digits, - and _)");
    }
    for (const core::QuestStep& step : quest.steps) {
        if (!isValidQuestName(step.id)) {
            return refused("step \"" + step.id +
                           "\": not a valid step id (lowercase letters, digits, - and _)");
        }
    }
    const std::filesystem::path file = questFile(dataRoot, quest.id);
    std::error_code absent;
    if (isNew && std::filesystem::exists(file, absent)) {
        return refused("a quest \"" + quest.id + "\" already exists");
    }

    // Le texte que le jeu lira, relu par le jeu : le mode n'a pas sa propre idée d'une quête.
    const std::string text = core::writeQuest(quest);
    core::QuestLoad reread = core::readQuest(text, "World/quests/" + quest.id + ".json");
    if (!reread.quest) {
        return refused(joined(reread.errors, "\n"));
    }
    const core::QuestCatalog before = core::loadQuests(questsDirectory(dataRoot));
    core::QuestCatalog after;
    for (const core::Quest& other : before.quests) {
        if (other.id == quest.id) {
            continue;
        }
        for (const core::QuestFlag& flag : reread.quest->flags) {
            if (std::ranges::find(other.flags, flag.id, &core::QuestFlag::id) !=
                other.flags.end()) {
                return refused("flag \"" + flag.id + "\" is already declared by quest \"" +
                               other.id + "\"");
            }
        }
        after.quests.push_back(other);
    }
    after.quests.push_back(*reread.quest);
    const core::DialogueCatalog dialogues = core::loadDialogues(dialoguesDirectory(dataRoot));
    const std::vector<std::string> previous = core::validateFlagUses(before, dialogues);
    std::vector<std::string> introduced;
    for (const std::string& misuse : core::validateFlagUses(after, dialogues)) {
        if (!contains(previous, misuse)) {
            introduced.push_back(misuse);
        }
    }
    if (!introduced.empty()) {
        return refused(joined(introduced, "\n"));
    }

    RefactorPlan plan;
    plan.changes.push_back(Citation{.file = file, .what = "quest " + quest.id});
    plan.edits.push_back(ProjectEdit{.file = file, .text = text});

    // Les textes du journal, langue par langue.
    const std::vector<std::string> keys = core::questTextKeys(quest);
    const std::string prefix = questKeyPrefix(quest.id);
    for (const std::filesystem::path& catalog : catalogFilesIn(localizationDirectory(dataRoot))) {
        const std::string language = catalog.stem().string();
        const auto given = draft.texts.find(language);
        const std::string content = readText(catalog);
        std::string updated = withoutCatalogEntries(content, [&](std::string_view key) {
            if (!key.starts_with(prefix)) {
                return false;
            }
            if (!contains(keys, key)) {
                return true;  // une étape retirée
            }
            // Vidé dans cette langue : retiré. Une langue que le brouillon ne donne pas reste.
            if (given == draft.texts.end()) {
                return false;
            }
            const auto text = given->second.find(key);
            return text == given->second.end() || text->second.empty();
        });
        if (given != draft.texts.end()) {
            for (const std::string& key : keys) {
                const auto found = given->second.find(key);
                if (found != given->second.end() && !found->second.empty()) {
                    updated = withCatalogEntry(updated, key, found->second, prefix);
                }
            }
        }
        if (updated != content) {
            plan.changes.push_back(Citation{.file = catalog, .what = prefix + "*"});
            plan.edits.push_back(ProjectEdit{.file = catalog, .text = std::move(updated)});
        }
    }
    return plan;
}

RefactorPlan planDeleteQuest(const std::filesystem::path& dataRoot, std::string_view questId) {
    const std::filesystem::path file = questFile(dataRoot, questId);
    std::error_code absent;
    if (!std::filesystem::exists(file, absent)) {
        return refused("no quest \"" + std::string{questId} + "\"");
    }
    RefactorPlan plan;
    plan.changes.push_back(Citation{.file = file, .what = "quest " + std::string{questId}});
    plan.edits.push_back(ProjectEdit{.file = file, .text = std::nullopt});
    const std::string prefix = questKeyPrefix(questId);
    for (const std::filesystem::path& catalog : catalogFilesIn(localizationDirectory(dataRoot))) {
        const std::string content = readText(catalog);
        std::string updated = withoutCatalogEntries(
            content, [&prefix](std::string_view key) { return key.starts_with(prefix); });
        if (updated != content) {
            plan.changes.push_back(Citation{.file = catalog, .what = prefix + "*"});
            plan.edits.push_back(ProjectEdit{.file = catalog, .text = std::move(updated)});
        }
    }
    return plan;
}

std::vector<FlagUse> flagUses(const std::filesystem::path& dataRoot) {
    std::vector<FlagUse> uses;
    const UseVisitor collect = [&uses](FlagUse& use) {
        uses.push_back(use);
        return false;
    };
    static_cast<void>(visitMaps(dataRoot, collect, nullptr));
    static_cast<void>(visitQuests(dataRoot, collect, nullptr));
    visitDialogues(dataRoot, collect, nullptr);
    return uses;
}

std::vector<FlagUse> usesOfFlag(const std::vector<FlagUse>& uses, std::string_view flag,
                                std::string_view value) {
    std::vector<FlagUse> found;
    for (const FlagUse& use : uses) {
        if (use.flag == flag && (value.empty() || contains(use.values, value))) {
            found.push_back(use);
        }
    }
    return found;
}

std::vector<Citation> flagUseCitations(const std::vector<FlagUse>& uses) {
    std::vector<Citation> citations;
    for (const FlagUse& use : uses) {
        Citation citation = use.where;
        citation.what += use.role == FlagUseRole::Declares ? " (declares"
                         : use.role == FlagUseRole::Reads  ? " (reads"
                                                           : " (writes";
        citation.what += use.values.empty() ? ")" : " " + joined(use.values, "|") + ")";
        citations.push_back(std::move(citation));
    }
    return citations;
}

RefactorPlan planRenameFlag(const std::filesystem::path& dataRoot, std::string_view oldFlag,
                            std::string_view newFlag) {
    if (oldFlag == newFlag) {
        return refused("the flag is already named \"" + std::string{newFlag} + "\"");
    }
    if (!isValidQuestName(newFlag, /*allowSlash=*/true)) {
        return refused("\"" + std::string{newFlag} + "\" is not a valid flag name");
    }
    if (!declaredFlag(dataRoot, oldFlag)) {
        return refused("no quest declares the flag \"" + std::string{oldFlag} + "\"");
    }
    if (!usesOfFlag(flagUses(dataRoot), newFlag).empty()) {
        return refused("the flag \"" + std::string{newFlag} + "\" is already used");
    }
    const std::string from{oldFlag};
    const std::string to{newFlag};
    return rewriteProject(dataRoot, [&from, &to](FlagUse& use) {
        if (use.flag != from) {
            return false;
        }
        use.flag = to;
        return true;
    });
}

RefactorPlan planRenameFlagValue(const std::filesystem::path& dataRoot, std::string_view flag,
                                 std::string_view oldValue, std::string_view newValue) {
    if (oldValue == newValue) {
        return refused("the value is already named \"" + std::string{newValue} + "\"");
    }
    if (!isValidQuestName(newValue)) {
        return refused("\"" + std::string{newValue} +
                       "\" is not a valid value (lowercase letters, digits, - and _)");
    }
    const std::optional<core::QuestFlag> declared = declaredFlag(dataRoot, flag);
    if (!declared) {
        return refused("no quest declares the flag \"" + std::string{flag} + "\"");
    }
    if (!contains(declared->values, oldValue)) {
        return refused("the flag \"" + std::string{flag} + "\" has no value \"" +
                       std::string{oldValue} + "\"");
    }
    if (contains(declared->values, newValue)) {
        return refused("the flag \"" + std::string{flag} + "\" already has a value \"" +
                       std::string{newValue} + "\"");
    }
    const std::string target{flag};
    const std::string from{oldValue};
    const std::string to{newValue};
    return rewriteProject(dataRoot, [&](FlagUse& use) {
        if (use.flag != target) {
            return false;
        }
        bool changed = false;
        for (std::string& value : use.values) {
            if (value == from) {
                value = to;
                changed = true;
            }
        }
        return changed;
    });
}

RefactorPlan planRenameQuest(const std::filesystem::path& dataRoot, std::string_view oldId,
                             std::string_view newId) {
    if (oldId == newId) {
        return refused("the quest is already named \"" + std::string{newId} + "\"");
    }
    if (!isValidQuestName(newId)) {
        return refused("\"" + std::string{newId} +
                       "\" is not a valid quest id (lowercase letters, digits, - and _)");
    }
    std::error_code absent;
    if (!std::filesystem::exists(questFile(dataRoot, oldId), absent)) {
        return refused("no quest \"" + std::string{oldId} + "\"");
    }
    if (std::filesystem::exists(questFile(dataRoot, newId), absent)) {
        return refused("a quest \"" + std::string{newId} + "\" already exists");
    }
    // Les faits que la quête pose (`quest/<id>/…`) suivent son nom là où on les lit.
    const std::string fromFacts = "quest/" + std::string{oldId} + "/";
    const std::string toFacts = "quest/" + std::string{newId} + "/";
    const QuestIdRenaming renaming{.from = std::string{oldId}, .to = std::string{newId}};
    RefactorPlan plan = rewriteProject(
        dataRoot,
        [&](FlagUse& use) {
            if (!use.flag.starts_with(fromFacts)) {
                return false;
            }
            use.flag = toFacts + use.flag.substr(fromFacts.size());
            return true;
        },
        &renaming);
    if (!plan.ok()) {
        return plan;
    }
    // Les clés du journal changent de nom à leur place, texte gardé.
    const std::string fromKeys = questKeyPrefix(oldId);
    const std::string toKeys = questKeyPrefix(newId);
    for (const std::filesystem::path& catalog : catalogFilesIn(localizationDirectory(dataRoot))) {
        const std::string content = readText(catalog);
        std::string updated;
        std::size_t start = 0;
        while (start < content.size()) {
            std::size_t end = content.find('\n', start);
            end = end == std::string::npos ? content.size() : end + 1;
            std::string line = content.substr(start, end - start);
            const std::string key = catalogLineKey(line);
            if (key.starts_with(toKeys)) {
                return refused(catalog.filename().string() + " already has \"" + key + "\"");
            }
            if (key.starts_with(fromKeys)) {
                const std::size_t at = line.find(fromKeys);
                line.replace(at, fromKeys.size(), toKeys);
            }
            updated += line;
            start = end;
        }
        if (updated != content) {
            plan.changes.push_back(Citation{.file = catalog, .what = fromKeys + "*"});
            plan.edits.push_back(ProjectEdit{.file = catalog, .text = std::move(updated)});
        }
    }
    return plan;
}

std::vector<std::string> worldStateReaching(const core::Quest& quest, std::string_view stepId,
                                            const std::vector<core::QuestFlag>& declared) {
    std::vector<std::string> entries;
    const core::QuestStep* step = quest.find(stepId);
    if (step == nullptr) {
        return entries;
    }
    std::set<std::string, std::less<>> given;
    const auto add = [&entries, &given](const std::string& flag, std::string entry) {
        if (given.insert(flag).second) {
            entries.push_back(std::move(entry));
        }
    };
    for (const core::FlagCondition& condition : step->when) {
        const auto declaration = std::ranges::find(declared, condition.flag, &core::QuestFlag::id);
        const bool typed = declaration != declared.end();
        switch (condition.test) {
            case core::FlagTest::IsSet:
                // Un drapeau déclaré a toujours une valeur, l'initiale : rien à poser.
                if (!typed) {
                    add(condition.flag, condition.flag);
                }
                break;
            case core::FlagTest::IsUnset:
                break;
            case core::FlagTest::Equals:
                if (typed && !condition.values.empty()) {
                    add(condition.flag, condition.flag + "=" + condition.values.front());
                }
                break;
            case core::FlagTest::NotEquals:
                if (typed) {
                    std::vector<std::string> candidates{declaration->initial};
                    candidates.insert(candidates.end(), declaration->values.begin(),
                                      declaration->values.end());
                    const auto allowed =
                        std::ranges::find_if(candidates, [&condition](const std::string& value) {
                            return !contains(condition.values, value);
                        });
                    if (allowed != candidates.end()) {
                        add(condition.flag, condition.flag + "=" + *allowed);
                    }
                }
                break;
        }
    }
    return entries;
}

void citeStepPlaces(const std::filesystem::path& dataRoot,
                    const std::function<std::optional<std::string>(std::string_view)>& rename,
                    bool write, RefactorPlan& plan) {
    for (const std::filesystem::path& file : jsonFiles(questsDirectory(dataRoot))) {
        core::QuestLoad loaded = core::loadQuest(file);
        if (!loaded.quest) {
            continue;
        }
        bool changed = false;
        for (core::QuestStep& step : loaded.quest->steps) {
            if (step.at.empty()) {
                continue;
            }
            const std::optional<std::string> renamed = rename(step.at);
            if (!renamed) {
                continue;
            }
            plan.changes.push_back(Citation{
                .file = file, .what = "quest " + loaded.quest->id + ": step " + step.id + ": at"});
            changed = changed || *renamed != step.at;
            step.at = *renamed;
        }
        if (write && changed) {
            plan.edits.push_back(
                ProjectEdit{.file = file, .text = core::writeQuest(*loaded.quest)});
        }
    }
}

namespace {

// Le plan écrit, ce qu'il change listé ; rien s'il est refusé (sortie 1).
[[nodiscard]] int carryOutQuestPlan(const RefactorPlan& plan, const std::filesystem::path& dataRoot,
                                    std::string& output) {
    if (!plan.ok()) {
        output += "error: " + plan.error + "\nnothing written\n";
        return 1;
    }
    for (const Citation& change : plan.changes) {
        output += formatCitation(change, dataRoot) + "\n";
    }
    std::string error;
    if (!applyRefactorPlan(plan, error)) {
        output += "error: " + error + "\n";
        return 1;
    }
    output += std::to_string(plan.edits.size()) + " files written or removed\n";
    return 0;
}

// Les arguments qui suivent @p option, jusqu'à la prochaine option.
[[nodiscard]] std::optional<std::vector<std::string>> argumentsOf(
    const std::vector<std::string>& arguments, std::string_view option) {
    const auto found = std::ranges::find(arguments, option);
    if (found == arguments.end()) {
        return std::nullopt;
    }
    std::vector<std::string> values;
    for (auto value = std::next(found); value != arguments.end() && !value->starts_with("--");
         ++value) {
        values.push_back(*value);
    }
    return values;
}

// `--save-quest <brouillon.json>` : la quête du fichier, ses textes sous `journal`.
[[nodiscard]] int saveQuestFrom(const std::filesystem::path& draftFile,
                                const std::filesystem::path& dataRoot, std::string& output) {
    const std::string text = readText(draftFile);
    core::QuestLoad loaded = core::readQuest(text, draftFile.string());
    if (!loaded.quest) {
        output += "error: " + joined(loaded.errors, "\n") + "\nnothing written\n";
        return 1;
    }
    QuestDraft draft{.quest = std::move(*loaded.quest), .texts = {}};
    const nlohmann::json json = nlohmann::json::parse(text, nullptr, false);
    if (const auto journal = json.find("journal"); journal != json.end() && journal->is_object()) {
        for (const auto& [language, entries] : journal->items()) {
            QuestLanguageTexts& texts = draft.texts[language];
            for (const auto& [entry, value] : entries.items()) {
                if (!value.is_string()) {
                    continue;
                }
                texts[entry == "title" ? core::questTitleKey(draft.quest.id)
                                       : core::questStepKey(draft.quest.id, entry)] =
                    value.get<std::string>();
            }
        }
    }
    std::error_code absent;
    const bool isNew = !std::filesystem::exists(questFile(dataRoot, draft.quest.id), absent);
    return carryOutQuestPlan(planSaveQuest(dataRoot, draft, isNew), dataRoot, output);
}

// `--quest-state <quête> <étape>` : l'état de partie, tel que `--flags=` le lit.
[[nodiscard]] int printQuestState(const std::vector<std::string>& values,
                                  const std::filesystem::path& dataRoot, std::string& output) {
    const core::QuestCatalog quests = core::loadQuests(questsDirectory(dataRoot));
    const core::Quest* quest = quests.find(values[0]);
    if (quest == nullptr || quest->find(values[1]) == nullptr) {
        output += "error: no step \"" + values[1] + "\" in quest \"" + values[0] + "\"\n";
        return 1;
    }
    std::vector<core::QuestFlag> declared;
    for (const core::Quest& each : quests.quests) {
        declared.insert(declared.end(), each.flags.begin(), each.flags.end());
    }
    output += "--flags=" + joined(worldStateReaching(*quest, values[1], declared), ",") + "\n";
    return 0;
}

}  // namespace

std::optional<int> runQuestCommand(const std::vector<std::string>& arguments,
                                   const std::filesystem::path& dataRoot, std::string& output) {
    const auto usage = [&output](std::string_view text) {
        output += "usage: " + std::string{text} + "\n";
        return 2;
    };
    if (const auto cites = argumentsOf(arguments, "--who-cites");
        cites && !cites->empty() && cites->front() == "flag") {
        if (cites->size() != 2 && cites->size() != 3) {
            return usage("--who-cites flag <flag> [<value>]");
        }
        const std::vector<Citation> citations = flagUseCitations(usesOfFlag(
            flagUses(dataRoot), (*cites)[1], cites->size() == 3 ? (*cites)[2] : std::string{}));
        for (const Citation& citation : citations) {
            output += formatCitation(citation, dataRoot) + "\n";
        }
        output += std::to_string(citations.size()) +
                  (citations.size() == 1 ? " citation\n" : " citations\n");
        return 0;
    }
    if (const auto flag = argumentsOf(arguments, "--rename-flag")) {
        if (flag->size() != 2) {
            return usage("--rename-flag <old> <new>");
        }
        return carryOutQuestPlan(planRenameFlag(dataRoot, (*flag)[0], (*flag)[1]), dataRoot,
                                 output);
    }
    if (const auto value = argumentsOf(arguments, "--rename-flag-value")) {
        if (value->size() != 3) {
            return usage("--rename-flag-value <flag> <old> <new>");
        }
        return carryOutQuestPlan(
            planRenameFlagValue(dataRoot, (*value)[0], (*value)[1], (*value)[2]), dataRoot, output);
    }
    if (const auto quest = argumentsOf(arguments, "--rename-quest")) {
        if (quest->size() != 2) {
            return usage("--rename-quest <old> <new>");
        }
        return carryOutQuestPlan(planRenameQuest(dataRoot, (*quest)[0], (*quest)[1]), dataRoot,
                                 output);
    }
    if (const auto draft = argumentsOf(arguments, "--save-quest")) {
        if (draft->size() != 1) {
            return usage("--save-quest <draft.json>");
        }
        return saveQuestFrom(draft->front(), dataRoot, output);
    }
    if (const auto state = argumentsOf(arguments, "--quest-state")) {
        if (state->size() != 2) {
            return usage("--quest-state <quest> <step>");
        }
        return printQuestState(*state, dataRoot, output);
    }
    return std::nullopt;
}

}  // namespace hmi
