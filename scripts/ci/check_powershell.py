#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Lint des scripts PowerShell du dépôt par PSScriptAnalyzer (refonte de la CI, phase 4).

`build.ps1`, `setup_dev.ps1` et les scripts de release portent une partie de la chaîne, et rien ne
les relisait. PSScriptAnalyzer (Microsoft) attrape un paramètre mal passé, un `ShouldProcess` non
déclaré, et — règle `PSUseCompatibleSyntax` — une syntaxe de pwsh 7 qui casserait le poste sous
Windows PowerShell 5.1. Règles : `scripts/ci/PSScriptAnalyzerSettings.psd1`.

La version du module est `PSSCRIPTANALYZER_VERSION` du bloc `env:` de ci.yml, et nulle part
ailleurs : ce script refuse une autre version installée plutôt que de rendre un verdict qui ne
prédit pas celui de la CI. `scripts/setup_dev.ps1 -Install` l'installe sur le poste.

Usage :
  python scripts/ci/check_powershell.py            tous les .ps1 suivis par git
  python scripts/ci/check_powershell.py FICHIER...
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys

CI = os.path.join('.github', 'workflows', 'ci.yml')
SETTINGS = os.path.join('scripts', 'ci', 'PSScriptAnalyzerSettings.psd1')
VERSION_RE = re.compile(r'^  PSSCRIPTANALYZER_VERSION:\s*[\'"]?([0-9.]+)', re.MULTILINE)

# Chemins et version passent par l'environnement : aucun nom de fichier n'est interpolé dans le
# code PowerShell.
ANALYZE = r'''
$ErrorActionPreference = 'Stop'
# Windows PowerShell écrit sinon dans la page de code OEM.
[Console]::OutputEncoding = [Text.Encoding]::UTF8
$module = Get-Module -ListAvailable PSScriptAnalyzer |
    Where-Object { $_.Version -eq [version]$env:JADG_PSSA_VERSION } | Select-Object -First 1
if (-not $module) {
    $found = (Get-Module -ListAvailable PSScriptAnalyzer | ForEach-Object { $_.Version }) -join ', '
    if (-not $found) { $found = 'aucune' }
    [Console]::Error.WriteLine("PSScriptAnalyzer $env:JADG_PSSA_VERSION introuvable (version(s) installée(s) : $found).")
    exit 3
}
Import-Module $module
$results = @()
foreach ($path in ($env:JADG_PSSA_FILES -split "`n" | Where-Object { $_ })) {
    $results += @(Invoke-ScriptAnalyzer -Path $path -Settings $env:JADG_PSSA_SETTINGS |
        ForEach-Object {
            [pscustomobject]@{ File = $path; Line = $_.Line; Column = $_.Column
                Rule = $_.RuleName; Severity = "$($_.Severity)"; Message = $_.Message }
        })
}
ConvertTo-Json -InputObject @($results) -Depth 3 -Compress
'''


def expected_version(ci_text):
    match = VERSION_RE.search(ci_text)
    return match.group(1) if match else None


def powershell():
    """pwsh (runner Linux, poste qui l'a) sinon Windows PowerShell."""
    return shutil.which('pwsh') or shutil.which('powershell')


def annotation(finding):
    """Ligne lisible, et annotation GitHub sur la ligne du diff quand on tourne en CI."""
    # Une règle qui porte sur le fichier entier (encodage) n'a ni ligne ni colonne.
    finding = dict(finding, Line=finding.get('Line') or 1, Column=finding.get('Column') or 1)
    text = '%s:%s:%s: %s [%s] %s' % (finding['File'], finding['Line'], finding['Column'],
                                     finding['Severity'].lower(), finding['Rule'],
                                     finding['Message'])
    if os.environ.get('GITHUB_ACTIONS') == 'true':
        level = 'error' if finding['Severity'] == 'Error' else 'warning'
        message = finding['Message'].replace('%', '%25').replace('\r', '').replace('\n', '%0A')
        text += '\n::%s file=%s,line=%s,col=%s,title=%s::%s' % (
            level, finding['File'], finding['Line'], finding['Column'], finding['Rule'], message)
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('files', nargs='*')
    arguments = parser.parse_args()

    os.chdir(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
    with open(CI, encoding='utf-8') as handle:
        version = expected_version(handle.read())
    if not version:
        print('ERREUR : PSSCRIPTANALYZER_VERSION absent du bloc env: de %s.' % CI)
        return 1

    files = arguments.files
    if not files:
        listing = subprocess.run(['git', 'ls-files', '-z', '*.ps1', '*.psm1', '*.psd1'],
                                 capture_output=True, check=True)
        files = [p for p in listing.stdout.decode('utf-8').split('\0') if p]
    if not files:
        print('ERREUR : aucun script PowerShell trouvé ; la recherche est cassée.')
        return 1

    shell = powershell()
    if not shell:
        print('ERREUR : ni pwsh ni powershell dans le PATH.')
        return 1
    env = dict(os.environ, JADG_PSSA_VERSION=version, JADG_PSSA_SETTINGS=SETTINGS,
               JADG_PSSA_FILES='\n'.join(files))
    completed = subprocess.run([shell, '-NoProfile', '-NonInteractive', '-Command', ANALYZE],
                               capture_output=True, text=True, encoding='utf-8', env=env)
    if completed.returncode != 0:
        print(completed.stderr.strip() or completed.stdout.strip())
        if completed.returncode == 3:
            print('Installer la version de la CI : scripts/setup_dev.ps1 -Install')
        return 1

    output = completed.stdout.strip()
    findings = json.loads(output) if output else []
    for finding in findings:
        print(annotation(finding))
    if findings:
        print('\n%d écart(s) PSScriptAnalyzer %s sur %d fichier(s).'
              % (len(findings), version, len(files)))
        return 1
    print('OK : %d script(s) PowerShell sans écart (PSScriptAnalyzer %s).' % (len(files), version))
    return 0


if __name__ == '__main__':
    sys.exit(main())
