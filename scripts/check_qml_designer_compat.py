# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Garde-fou : le contrat entre les formulaires et Qt Design Studio (LOT-87).

POURQUOI CE CONTROLE EXISTE
---------------------------
Qt Design Studio dessine avec son propre Qt et un marionnettiste (`qmlpuppet`) qui ne charge aucun
plugin C++ du projet. Tout ce qu'un formulaire nomme doit donc se resoudre SANS le jeu : dans les
modules de Qt, dans `Jadg.Ui` (QML pur) ou, pour les jumeaux de cablage, dans les doublures de
`Source/Ui/Mocks/`. Un type C++ qui se glisse dans un formulaire ne casse rien a la construction et
rien a l'execution : il rend seulement l'ecran irresolvable dans l'atelier, ce qui se decouvre le
jour ou quelqu'un veut le dessiner.

La premiere version de ce script (branche abandonnee) verifiait une STRUCTURE CMake -- noms de
cibles, mots-cles, macros d'import de plugin. Elle a suivi cette structure dans sa derive. Celle-ci
verifie un CONTRAT, ecrit ci-dessous, qui ne depend d'aucun detail de construction hors les deux
seuls interdits de la phase 1 (aucun alias de ressource, aucune desactivation des qmldir
engendres).

CE QUI EST VERIFIE
------------------
1. Chaque `.ui.qml` de Source/Ui n'importe que la liste autorisee (jamais Jadg.Runtime ni Jadg.App).
2. Chaque `.ui.qml` ne contient aucun motif que Design Studio detruirait ou ne saurait afficher.
3. Aucun formulaire ne nomme un type C++ de `Jadg.Runtime` (ni comme element, ni comme singleton).
4. Chaque type C++ expose au QML (QML_ELEMENT / QML_NAMED_ELEMENT dans Source/HMI/Runtime) a sa
   doublure dans Mocks/Jadg/Runtime/qmldir, le fichier existe, et chaque Q_PROPERTY et Q_INVOKABLE
   du C++ y a son pendant.
5. Chaque jumeau (Source/App/Game/Qml/Screens/X.qml) a son formulaire (Source/Ui/Screens/XForm.ui.qml)
   et reciproquement ; les jumeaux importent Jadg.Runtime quand ils en nomment un type.
6. Chaque fichier QML de Source/Ui est liste par Source/Ui/CMakeLists.txt et chaque fichier de
   Source/App/Game/Qml par Source/App/CMakeLists.txt : un fichier non liste n'existe pas pour le jeu.
7. Aucun CMakeLists.txt du depot ne pose d'alias de ressource ni ne desactive les qmldir engendres.
8. Le `qmldir` engendre de Jadg.Ui, s'il existe, est `designersupported` et sans plugin ; la
   galerie de l'atelier existe.
9. Aucune dependance circulaire entre fichiers QML du projet.

Controle purement textuel : aucun binaire, aucune dependance. Il s'auto-verifie contre la vacuite,
comme `check_ui_layers.py` : une regle qui n'a rien lu est un ECHEC.

Usage :
    python scripts/check_qml_designer_compat.py     # code de sortie non nul si le contrat est rompu
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UI = ROOT / "Source" / "Ui"
UI_CMAKE = UI / "CMakeLists.txt"
MOCKS = UI / "Mocks" / "Jadg" / "Runtime"
MOCKS_QMLDIR = MOCKS / "qmldir"
DEV_QMLDIR = UI / "Jadg" / "Ui" / "qmldir"
DESIGN_ENTRY = UI / "DesignStudio" / "Main.ui.qml"
APP_QML = ROOT / "Source" / "App" / "Game" / "Qml"
APP_CMAKE = ROOT / "Source" / "App" / "CMakeLists.txt"
RUNTIME = ROOT / "Source" / "HMI" / "Runtime"

# Modules importables depuis un formulaire : l'intersection de ce que connaissent Qt ET Design
# Studio. Jadg.Runtime (C++) et Jadg.App (cablage) n'y sont pas, et ne le seront jamais.
ALLOWED_FORM_IMPORTS = {
    "QtQuick", "QtQuick.Window", "QtQuick.Layouts", "QtQuick.Controls",
    "QtQuick.Shapes", "QtQuick.Effects", "Jadg.Ui",
}

# Ce que Design Studio perd a l'enregistrement, ou ne sait pas afficher.
FORBIDDEN_IN_FORMS = (
    (re.compile(r"\bfunction\s+\w+\s*\("), "une fonction JavaScript"),
    (re.compile(r"\bComponent\.onCompleted\b"), "un Component.onCompleted"),
    (re.compile(r"^\s*Connections\s*\{", re.M), "un bloc Connections"),
    (re.compile(r"^\s*Timer\s*\{", re.M), "un Timer"),
    (re.compile(r"\bconsole\.(log|warn|error|debug)\b"), "un appel a console"),
    (re.compile(r"\bQt\.callLater\b"), "un Qt.callLater"),
    (re.compile(r"\bQt\.createQmlObject\b"), "un Qt.createQmlObject"),
)

# Les deux interdits de la phase 1 du LOT-87, composes pour que ce fichier ne les contienne pas
# lui-meme en clair : ils designent une plomberie CMake, pas un contrat.
FORBIDDEN_CMAKE = (
    ("QT_RESOURCE" + "_ALIAS", "un alias de ressource par fichier"),
    ("NO_GENERATE_" + "EXTRA_QMLDIRS", "la desactivation des qmldir engendres par sous-repertoire"),
)

IMPORT_RE = re.compile(r"^\s*import\s+([A-Za-z][\w.]*)", re.M)
TYPE_USE_RE = re.compile(r"(?<![\w.])([A-Z][A-Za-z0-9_]*)\s*\{")
QML_ELEMENT_RE = re.compile(r"^\s*QML_ELEMENT\b", re.M)
QML_NAMED_RE = re.compile(r"^\s*QML_NAMED_ELEMENT\(\s*(\w+)\s*\)", re.M)
CLASS_RE = re.compile(r"^\s*class\s+(\w+)\s*(?:final\s*)?:\s*public", re.M)
Q_PROPERTY_RE = re.compile(r"Q_PROPERTY\(\s*[\w:<>*\s]+?\s+(\w+)\s+READ", re.M)
Q_INVOKABLE_RE = re.compile(r"Q_INVOKABLE\s+[\w:<>*&\s]+?\s+(\w+)\s*\(", re.M)
CMAKE_FILE_RE = re.compile(r"^\s*([\w./-]+\.qml)\s*$", re.M)
QMLDIR_ENTRY_RE = re.compile(r"^(?:singleton\s+)?(\w+)\s+[\d.]+\s+(\S+)\s*$", re.M)

LINE_COMMENT = re.compile(r"//[^\n]*")
BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.S)


def strip_comments(text: str) -> str:
    return LINE_COMMENT.sub("", BLOCK_COMMENT.sub("", text))


def strip_cmake_comments(text: str) -> str:
    return re.sub(r"#[^\n]*", "", text)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def project_qml(directory: Path) -> list[Path]:
    """Les fichiers QML ECRITS du projet sous un repertoire : ni le qmldir engendre, ni Mocks."""
    if not directory.is_dir():
        return []
    return sorted(
        p for p in directory.rglob("*.qml")
        if (UI / "Jadg") not in p.parents and MOCKS not in p.parents
    )


def forms() -> list[Path]:
    return [p for p in project_qml(UI) if p.name.endswith(".ui.qml")]


def twins() -> list[Path]:
    screens = APP_QML / "Screens"
    return sorted(screens.glob("*.qml")) if screens.is_dir() else []


def runtime_types() -> dict[str, dict[str, set[str]]]:
    """Les types C++ exposes au QML, avec leurs proprietes et methodes invocables, lus dans les
    en-tetes de Source/HMI/Runtime. Nom QML -> {"properties": ..., "methods": ...}."""
    types: dict[str, dict[str, set[str]]] = {}
    for header in sorted(RUNTIME.glob("*.h")):
        body = read(header)
        if QML_ELEMENT_RE.search(body) is None and QML_NAMED_RE.search(body) is None:
            continue
        named = QML_NAMED_RE.search(body)
        cls = CLASS_RE.search(body)
        name = named.group(1) if named else (cls.group(1) if cls else header.stem)
        types[name] = {
            "properties": set(Q_PROPERTY_RE.findall(body)),
            "methods": set(Q_INVOKABLE_RE.findall(body)),
        }
    return types


def check_form_imports_and_patterns(failures: list[str]) -> int:
    checked = 0
    for path in forms():
        checked += 1
        body = strip_comments(read(path))
        for module in IMPORT_RE.findall(body):
            if module not in ALLOWED_FORM_IMPORTS:
                failures.append(
                    f"{rel(path)} : import de « {module} ». Un formulaire n'importe que ce que "
                    f"Design Studio resout sans le jeu : {', '.join(sorted(ALLOWED_FORM_IMPORTS))}."
                )
        for pattern, what in FORBIDDEN_IN_FORMS:
            if pattern.search(body):
                failures.append(
                    f"{rel(path)} : un formulaire contient {what}. Design Studio le perdrait a "
                    f"l'enregistrement -- la logique va dans le jumeau `.qml` de Jadg.App."
                )
    return checked


def check_forms_name_no_cpp_type(failures: list[str], types: dict) -> int:
    checked = 0
    for path in forms():
        checked += 1
        body = strip_comments(read(path))
        used = set(TYPE_USE_RE.findall(body))
        singletons = {name for name in types if re.search(rf"(?<![\w.]){name}\.", body)}
        for name in sorted((used & set(types)) | singletons):
            failures.append(
                f"{rel(path)} : nomme le type C++ « {name} » (Jadg.Runtime). Un formulaire ne voit "
                f"que du QML : le jumeau pose ce type, ou lui passe la valeur par une propriete."
            )
    return checked


def check_mocks(failures: list[str], types: dict) -> int:
    if not MOCKS_QMLDIR.is_file():
        failures.append(f"{rel(MOCKS_QMLDIR)} introuvable : aucune doublure pour Design Studio.")
        return 0
    qmldir = read(MOCKS_QMLDIR)
    if re.search(r"^module\s+Jadg\.Runtime\s*$", qmldir, re.M) is None:
        failures.append(f"{rel(MOCKS_QMLDIR)} : doit declarer `module Jadg.Runtime`.")
    entries = {name: file for name, file in QMLDIR_ENTRY_RE.findall(qmldir)}
    checked = 0
    for name, api in sorted(types.items()):
        checked += 1
        if name not in entries:
            failures.append(
                f"{rel(MOCKS_QMLDIR)} : pas de doublure pour le type C++ « {name} ». Sans elle, "
                f"tout jumeau qui le nomme reste irresolu dans l'atelier."
            )
            continue
        mock = MOCKS / entries[name]
        if not mock.is_file():
            failures.append(f"{rel(MOCKS_QMLDIR)} : « {name} » designe {entries[name]}, absent.")
            continue
        body = strip_comments(read(mock))
        for prop in sorted(api["properties"]):
            if re.search(rf"\bproperty\s+[\w<>]+\s+{prop}\b", body) is None:
                failures.append(
                    f"{rel(mock)} : la propriete « {prop} » de {name} (Q_PROPERTY) n'a pas de "
                    f"pendant. Un jumeau qui la lit verrait un `undefined` dans l'atelier."
                )
        for method in sorted(api["methods"]):
            if re.search(rf"\bfunction\s+{method}\s*\(", body) is None:
                failures.append(
                    f"{rel(mock)} : la methode « {method} » de {name} (Q_INVOKABLE) n'a pas de "
                    f"pendant."
                )
    for name, file in sorted(entries.items()):
        if name not in types:
            failures.append(
                f"{rel(MOCKS_QMLDIR)} : « {name} » ({file}) ne correspond a aucun type C++ de "
                f"Source/HMI/Runtime. Une doublure sans original est un mensonge."
            )
    return checked


def check_twins(failures: list[str], types: dict) -> int:
    checked = 0
    screens = UI / "Screens"
    for twin in twins():
        checked += 1
        form = screens / f"{twin.stem}Form.ui.qml"
        if not form.is_file():
            failures.append(f"{rel(twin)} : jumeau sans formulaire. Attendu : {rel(form)}.")
        body = strip_comments(read(twin))
        imports = set(IMPORT_RE.findall(body))
        uses_cpp = any(re.search(rf"(?<![\w.]){name}\b", body) for name in types)
        if uses_cpp and "Jadg.Runtime" not in imports:
            failures.append(
                f"{rel(twin)} : nomme un type C++ sans `import Jadg.Runtime`. Il se resoudrait par "
                f"hasard dans le jeu, jamais dans l'atelier."
            )
    for form in sorted(screens.glob("*Form.ui.qml")) if screens.is_dir() else []:
        checked += 1
        twin = APP_QML / "Screens" / f"{form.name[:-len('Form.ui.qml')]}.qml"
        if not twin.is_file():
            failures.append(f"{rel(form)} : formulaire sans jumeau de cablage. Attendu : {rel(twin)}.")
    return checked


def check_cmake_lists_every_file(failures: list[str]) -> int:
    checked = 0
    for cmake, directory, base in ((UI_CMAKE, UI, UI), (APP_CMAKE, APP_QML, APP_CMAKE.parent)):
        if not cmake.is_file():
            failures.append(f"{rel(cmake)} introuvable.")
            continue
        listed = set(CMAKE_FILE_RE.findall(strip_cmake_comments(read(cmake))))
        for path in project_qml(directory):
            checked += 1
            expected = path.relative_to(base).as_posix()
            if expected not in listed:
                failures.append(
                    f"{rel(path)} : absent de {rel(cmake)}. Un fichier QML non liste n'existe pas "
                    f"pour le jeu, et l'atelier le verrait pourtant."
                )
    return checked


def check_no_forbidden_cmake(failures: list[str]) -> int:
    checked = 0
    for cmake in sorted(ROOT.rglob("CMakeLists.txt")):
        if "build" in cmake.parts or "External" in cmake.parts:
            continue
        checked += 1
        body = strip_cmake_comments(read(cmake))
        for token, what in FORBIDDEN_CMAKE:
            if token in body:
                failures.append(
                    f"{rel(cmake)} : contient {what} ({token}). Un module QML se declare depuis le "
                    f"repertoire de ses fichiers, jamais en reecrivant leurs chemins."
                )
    return checked


def check_qmldirs(failures: list[str]) -> int:
    checked = 0
    if DEV_QMLDIR.is_file():
        checked += 1
        body = read(DEV_QMLDIR)
        if re.search(r"^designersupported\s*$", body, re.M) is None:
            failures.append(
                f"{rel(DEV_QMLDIR)} : sans `designersupported`, la bibliotheque de composants de "
                f"Design Studio reste vide."
            )
        if re.search(r"^(optional\s+)?plugin\s", body, re.M):
            failures.append(f"{rel(DEV_QMLDIR)} : un module de conception ne declare aucun plugin.")
        if re.search(r"^singleton\s+Tokens\s", body, re.M) is None:
            failures.append(f"{rel(DEV_QMLDIR)} : Tokens doit etre declare singleton.")
    if DESIGN_ENTRY.is_file():
        checked += 1
    else:
        failures.append(f"{rel(DESIGN_ENTRY)} introuvable : l'atelier n'a plus de galerie.")
    return checked


def local_dependency_cycles(failures: list[str]) -> int:
    files = project_qml(UI) + project_qml(APP_QML)
    by_type: dict[str, list[Path]] = {}
    for path in files:
        by_type.setdefault(path.name.split(".")[0], []).append(path)
    graph: dict[Path, set[Path]] = {}
    for path in files:
        body = strip_comments(read(path))
        graph[path] = {
            dep for name in set(TYPE_USE_RE.findall(body))
            for dep in by_type.get(name, []) if dep != path
        }
    state: dict[Path, int] = {}
    stack: list[Path] = []

    def visit(node: Path) -> None:
        state[node] = 1
        stack.append(node)
        for nxt in graph[node]:
            if state.get(nxt, 0) == 0:
                visit(nxt)
            elif state.get(nxt) == 1:
                cycle = stack[stack.index(nxt):] + [nxt]
                failures.append("dependance QML circulaire : " + " -> ".join(rel(p) for p in cycle))
        stack.pop()
        state[node] = 2

    for node in graph:
        if state.get(node, 0) == 0:
            visit(node)
    return len(files)


def main() -> int:
    failures: list[str] = []
    types = runtime_types()
    counts = {
        "1-2. formulaires : imports autorises, sans imperatif": check_form_imports_and_patterns(failures),
        "3. formulaires sans type C++": check_forms_name_no_cpp_type(failures, types),
        "4. doublures completes pour chaque type C++": check_mocks(failures, types),
        "5. jumeaux et formulaires appareilles": check_twins(failures, types),
        "6. chaque fichier QML liste par son CMake": check_cmake_lists_every_file(failures),
        "7. aucun alias ni desactivation dans les CMake": check_no_forbidden_cmake(failures),
        "8. qmldir de conception et galerie": check_qmldirs(failures),
        "9. aucune dependance circulaire": local_dependency_cycles(failures),
    }

    empty = [name for name, count in counts.items() if count == 0]
    if empty:
        print("check_qml_designer_compat : ECHEC -- ces regles n'ont rien eu a lire :", file=sys.stderr)
        for name in empty:
            print(f"  - {name}", file=sys.stderr)
        return 1

    if failures:
        print(f"check_qml_designer_compat : {len(failures)} manquement(s).\n", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1

    print(f"check_qml_designer_compat : OK ({sum(counts.values())} element(s), 9 regles).")
    for name, count in counts.items():
        print(f"  {name} : {count}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
