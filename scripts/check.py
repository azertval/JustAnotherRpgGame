#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Rejoue sur le poste les contrôles du dépôt, en une commande (refonte de la CI, phase 2).

Le job `lint-exigences` de ci.yml enchaîne une quinzaine de scripts, qu'il fallait lancer un par un
pour prédire son verdict. Ce script ne tient **pas** sa propre liste : il lit les étapes de ce job
dans ci.yml et exécute chaque `run: python3 scripts/…` (et `run: python3 -m pytest …`) avec
l'interpréteur courant. Un contrôle
ajouté à la CI est donc rejoué ici sans qu'on y pense, et les deux ne peuvent pas diverger.

Il lance ensuite `pre-commit run --all-files` (les hooks de .pre-commit-config.yaml), s'il est
installé. Tous les contrôles s'exécutent même après un échec, comme en CI, et un tableau final
donne le verdict de chacun.

Ce que ce script ne rejoue pas : les builds, les tests, clang-tidy et la documentation — ils
demandent MSVC, Qt, LLVM ou Doxygen, et passent par `scripts/build.ps1`.

L'interpréteur courant doit être celui de `.venv/` (phase 4) : `uv run scripts/check.py` le garantit,
et installe au passage les versions de `uv.lock`, celles du runner.

Usage :
  uv run scripts/check.py                 tous les contrôles, puis pre-commit
  uv run scripts/check.py --sans-hooks    sans pre-commit
  uv run scripts/check.py --auto-test     vérifie seulement la lecture de ci.yml
"""
import argparse
import os
import re
import shlex
import shutil
import subprocess
import sys

CI = os.path.join('.github', 'workflows', 'ci.yml')
JOB = 'lint-exigences'
JOB_RE = re.compile(r'^  ([A-Za-z0-9_-]+):\s*$')
NAME_RE = re.compile(r'^      - name:\s*(.+?)\s*$')
# Un script du dépôt — de `scripts/` ou de l'outillage de `Planning/` —, ou pytest (les tests des
# scripts, phase 4).
RUN_RE = re.compile(
    r'^        run:\s*python3\s+((?:scripts|Planning/outils)/\S+\.py|-m\s+pytest)(.*?)\s*$')
ENV_RE = re.compile(r'^  ([A-Z0-9_]+):\s*[\'"]?([^\'"\s#]+)')


def read_ci(text):
    """(contrôles, env) : la liste (nom, commande, arguments) des étapes `python3 scripts/…` et
    `python3 -m pytest` du job JOB, et les variables d'`env:` de premier niveau. La commande est ce
    qui suit `python3` : le chemin du script, ou `-m pytest`."""
    checks, env = [], {}
    in_env = in_job = False
    name = None
    for line in text.splitlines():
        if line.startswith('env:'):
            in_env = True
            continue
        if line and not line.startswith(' '):
            in_env = False
        if in_env:
            match = ENV_RE.match(line)
            if match:
                env[match.group(1)] = match.group(2)
            continue
        job = JOB_RE.match(line)
        if job:
            in_job = job.group(1) == JOB
            continue
        if not in_job:
            continue
        step = NAME_RE.match(line)
        if step:
            name = step.group(1)
            continue
        run = RUN_RE.match(line)
        if run:
            command = ' '.join(run.group(1).split())
            checks.append((name or command, command, shlex.split(run.group(2))))
    return checks, env


def auto_test(root):
    with open(os.path.join(root, CI), encoding='utf-8') as handle:
        checks, env = read_ci(handle.read())
    # Plancher : le job en compte bien plus. Une lecture cassée qui ne trouverait rien rendrait ce
    # script vert par vacuité — la panne du LOT-78.
    assert len(checks) >= 10, 'seulement %d contrôle(s) lu(s) dans %s' % (len(checks), CI)
    for _, command, _ in checks:
        if command.endswith('.py'):
            assert os.path.isfile(os.path.join(root, command)), 'script absent : %s' % command
    assert any(command == '-m pytest' for _, command, _ in checks), 'étape pytest absente'
    assert 'UV_VERSION' in env and 'PRE_COMMIT_VERSION' in env, sorted(env)
    sample = ('env:\n  A_VERSION: 1.2\njobs:\n  autre:\n    steps:\n      - name: X\n'
              '        run: python3 scripts/x.py\n  %s:\n    steps:\n      - name: Premier\n'
              '        id: p\n        run: python3 scripts/a.py --all\n'
              '      - name: Pas python\n        run: pip install x\n'
              '      - name: Second\n        run: python3 Planning/outils/b.py\n'
              '      - name: Tests\n        run: python3 -m pytest --junitxml=x.xml\n' % JOB)
    assert read_ci(sample) == ([('Premier', 'scripts/a.py', ['--all']),
                                ('Second', 'Planning/outils/b.py', []),
                                ('Tests', '-m pytest', ['--junitxml=x.xml'])],
                               {'A_VERSION': '1.2'})
    print('OK : %d contrôle(s) lu(s) dans le job %s de %s.' % (len(checks), JOB, CI))


def pre_commit_version():
    """Version de l'exécutable `pre-commit` du PATH : c'est lui que les hooks lancent, et il vit
    hors de .venv/."""
    executable = shutil.which('pre-commit')
    if not executable:
        return None
    output = subprocess.run([executable, '--version'], capture_output=True, text=True).stdout
    match = re.search(r'(\d+\.\d+\.\d+)', output)
    return match.group(1) if match else None


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--sans-hooks', action='store_true', help='ne pas lancer pre-commit')
    parser.add_argument('--auto-test', action='store_true')
    arguments = parser.parse_args()

    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    if arguments.auto_test:
        auto_test(root)
        return 0
    os.chdir(root)

    with open(CI, encoding='utf-8') as handle:
        checks, env = read_ci(handle.read())
    if not checks:
        print('ERREUR : aucun contrôle lu dans le job %s de %s.' % (JOB, CI))
        return 1

    # Les versions ne sont pas imposées ici mais un écart est dit : un verdict local obtenu avec un
    # autre jsonschema ne prédit pas celui de la CI. Les dépendances Python sont celles de uv.lock,
    # installées dans .venv/ ; pre-commit reste un outil du poste (setup_dev.ps1).
    warnings = []
    venv = os.path.normcase(os.path.realpath(os.path.join(root, '.venv')))
    if os.path.normcase(os.path.realpath(sys.prefix)) != venv:
        warnings.append('interpréteur hors de .venv/ (%s) : les versions de uv.lock ne sont pas '
                        'garanties. Lancer « uv run scripts/check.py ».' % sys.prefix)
    expected, found = env.get('PRE_COMMIT_VERSION'), pre_commit_version()
    if found != expected:
        warnings.append('pre-commit %s installé, %s attendu (PRE_COMMIT_VERSION dans ci.yml).'
                        % (found or 'non', expected))

    results = []
    child_env = dict(os.environ, PYTHONUTF8='1')
    for name, command, args in checks:
        print('\n==> %s' % name, flush=True)
        code = subprocess.run([sys.executable, *command.split(), *args], env=child_env).returncode
        results.append((name, code == 0))

    if not arguments.sans_hooks:
        pre_commit = shutil.which('pre-commit')
        print('\n==> Hooks pre-commit (--all-files)', flush=True)
        if pre_commit:
            code = subprocess.run([pre_commit, 'run', '--all-files'], env=child_env).returncode
            results.append(('Hooks pre-commit', code == 0))
        else:
            warnings.append('pre-commit introuvable : hooks non rejoués '
                            '(scripts/setup_dev.ps1 -Install).')

    print('\n%-60s %s' % ('Contrôle', 'Verdict'))
    for name, ok in results:
        print('%-60s %s' % (name[:60], 'vert' if ok else 'ROUGE'))
    for warning in warnings:
        print('ATTENTION : ' + warning)
    failed = sum(1 for _, ok in results if not ok)
    print('\n%d contrôle(s), %d rouge(s).' % (len(results), failed))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
