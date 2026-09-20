<#
.SYNOPSIS
    Couverture de code des trois suites GoogleTest, fusionnées en un rapport Cobertura et HTML.

.DESCRIPTION
    OpenCppCoverage n'accepte qu'un exécutable par invocation : chaque suite (UnitTests,
    IntegrationTests, SystemTests) est exportée au format binaire intermédiaire, puis les trois sont
    fusionnées en un seul rapport (un rapport limité à UnitTests rendrait le
    chiffre affiché faux par construction).

    Une seule définition pour la CI (job build-test-coverage de ci.yml, qui applique le seuil) et
    pour le site qualité (docs.yml, qui publie le rapport HTML), et rejouable sur le poste
    (refonte de la chaîne d'outillage, phase 4) :

        powershell -File scripts/coverage.ps1 -BinDir build/vs/bin/Debug

    Écrit `coverage.xml` (Cobertura) et `coverage-html/` dans -OutDir.

.PARAMETER BinDir
    Dossier des exécutables de test, construits en Debug (les symboles sont nécessaires).

.PARAMETER OutDir
    Dossier où écrire les rapports. Par défaut : le répertoire courant.

.PARAMETER OpenCppCoverage
    Chemin de OpenCppCoverage.exe. Par défaut : l'emplacement d'installation de Chocolatey et winget.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string] $BinDir,
    [string] $OutDir = '.',
    [string] $OpenCppCoverage = (Join-Path $env:ProgramFiles 'OpenCppCoverage\OpenCppCoverage.exe')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path -LiteralPath $OpenCppCoverage)) {
    throw "OpenCppCoverage introuvable : $OpenCppCoverage (scripts/setup_dev.ps1 -Install)."
}
$bin = (Resolve-Path -LiteralPath $BinDir).Path
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$out = (Resolve-Path -LiteralPath $OutDir).Path

$common = @(
    '--sources', (Join-Path $repoRoot 'Source'),
    '--excluded_sources', (Join-Path $repoRoot 'Source\Test'),
    '--excluded_sources', (Join-Path $repoRoot 'External')
)
$inputs = @()
foreach ($suite in 'UnitTests', 'IntegrationTests', 'SystemTests') {
    $exe = Join-Path $bin "$suite.exe"
    if (-not (Test-Path -LiteralPath $exe)) { throw "Suite absente : $exe" }
    $intermediate = Join-Path $out "$suite.cov"
    & $OpenCppCoverage @common --export_type "binary:$intermediate" -- $exe
    if ($LASTEXITCODE -ne 0) { throw "$suite sous OpenCppCoverage : échec ($LASTEXITCODE)." }
    $inputs += '--input_coverage', $intermediate
}

& $OpenCppCoverage @inputs `
    --export_type ("cobertura:" + (Join-Path $out 'coverage.xml')) `
    --export_type ("html:" + (Join-Path $out 'coverage-html'))
if ($LASTEXITCODE -ne 0) { throw "Fusion des rapports de couverture : échec ($LASTEXITCODE)." }
Write-Host "Rapports écrits : $(Join-Path $out 'coverage.xml') et $(Join-Path $out 'coverage-html')"
