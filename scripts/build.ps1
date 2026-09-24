<#
.SYNOPSIS
    Configure, construit et teste JustAnotherRpgGame dans un environnement MSVC correct.

.DESCRIPTION
    Les générateurs Ninja et Makefiles n'établissent pas l'environnement MSVC : ils héritent des
    variables LIB / INCLUDE du terminal appelant. Une invite « Developer PowerShell » démarre en
    x86, ce qui fait échouer l'édition de liens d'une cible x64. Ce script supprime le problème en
    entrant lui-même dans l'environnement x64 avant d'appeler CMake — il fonctionne donc depuis
    n'importe quel terminal, y compris un PowerShell nu.

    L'installation de Visual Studio est localisée par vswhere.exe, présent à un emplacement fixe
    sur tout poste où Visual Studio est installé : aucun chemin propre à une machine n'est codé en
    dur.

.PARAMETER Preset
    Preset CMake à utiliser : « ninja » (défaut), « vs », « ninja-release » ou « vs-release ».
    Les deux derniers construisent en Release, où `NDEBUG` est défini : c'est la seule façon de
    vérifier ce qui DISPARAÎT d'un binaire livré -- l'outillage adossé à `core::DEVELOPER_BUILD`.
    Une garantie qu'on ne peut pas construire est une intention.

.PARAMETER Test
    Exécuter CTest après la construction.

.PARAMETER Label
    Ne lancer que les tests d'un étage : « unitaire », « integration » ou « systeme » (labels CTest
    de Source/Test/CMakeLists.txt). Implique -Test. Pour une boucle de développement rapide ; le
    cycle complet reste à lancer avant de pousser.

.PARAMETER Clean
    Supprimer le répertoire de build avant de configurer.

.PARAMETER Target
    Cible precise a construire au lieu de tout (ex. « JustAnotherRpgGame_qmllint », engendree par
    qt_add_qml_module et qui verifie tous les .qml du module).

.PARAMETER QtPath
    Chemin d'une installation Qt à utiliser, si la détection automatique ne la trouve pas
    (ex. D:\Qt\6.11.2\msvc2022_64).

.EXAMPLE
    pwsh scripts/build.ps1
    Configure et construit avec le preset ninja.

.EXAMPLE
    pwsh scripts/build.ps1 -Preset vs -Test -Clean
    Reconstruit de zéro avec le générateur Visual Studio, puis lance les tests.
#>
[CmdletBinding()]
param(
    [ValidateSet('ninja', 'vs', 'ninja-release', 'vs-release')]
    [string]$Preset = 'ninja',

    [switch]$Test,

    [ValidateSet('unitaire', 'integration', 'systeme')]
    [string]$Label,

    [switch]$Clean,

    [string]$QtPath,

    # Cible precise a construire au lieu de tout. Sert notamment au controle QML :
    #   scripts/build.ps1 -Target JustAnotherRpgGame_qmllint
    # Passer par ce script et non par `cmake` directement n'est pas une preference : `cmake` seul
    # herite d'un terminal sans environnement MSVC, et echoue sur un <array> introuvable.
    [string]$Target
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent $PSScriptRoot

function Enter-X64Environment {
    # Déjà en x64 (invite développeur x64, ou script déjà appelé) : rien à faire.
    if ($env:VSCMD_ARG_TGT_ARCH -eq 'x64') {
        Write-Host 'Environnement MSVC x64 déjà actif.' -ForegroundColor DarkGray
        return
    }

    $installerDir = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer'
    $vswhere = Join-Path $installerDir 'vswhere.exe'
    if (-not (Test-Path $vswhere)) {
        throw "vswhere.exe introuvable ($vswhere). Visual Studio (ou les Build Tools) doit être installé."
    }

    # Le module DevShell invoque vswhere sans chemin absolu : sans cet ajout au PATH, il émet une
    # erreur « commande non reconnue » avant de se rabattre sur un autre mécanisme.
    if ($env:PATH -notlike "*$installerDir*") {
        $env:PATH = "$installerDir;$env:PATH"
    }

    $vsPath = & $vswhere -latest -prerelease -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath
    if (-not $vsPath) {
        throw 'Aucune installation Visual Studio avec les outils C++ x64 trouvée (composant Microsoft.VisualStudio.Component.VC.Tools.x86.x64).'
    }

    $devShell = Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
    if (-not (Test-Path $devShell)) {
        throw "Module DevShell introuvable ($devShell)."
    }

    Import-Module $devShell
    # -SkipAutomaticLocation : conserve le répertoire courant, que le script gère lui-même.
    Enter-VsDevShell -VsInstallPath $vsPath `
        -DevCmdArguments '-arch=amd64 -host_arch=amd64' `
        -SkipAutomaticLocation | Out-Null

    if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
        throw "L'environnement de développement n'a pas basculé en x64 (VSCMD_ARG_TGT_ARCH=$env:VSCMD_ARG_TGT_ARCH)."
    }
    Write-Host "Environnement MSVC x64 établi ($vsPath)." -ForegroundColor DarkGray
}

<#
.SYNOPSIS
    Vérifie que Ninja garde encore les dépendances d'en-têtes de ce répertoire de build.

.DESCRIPTION
    Ninja n'apprend les en-têtes qu'un fichier inclut qu'en le compilant, et les garde dans
    « .ninja_deps ». Si ce journal est perdu — recompaction refusée, construction interrompue,
    disque plein —, une modification d'en-tête ne recompile plus ses consommateurs : des objets
    compilés contre deux versions d'une même structure se lient alors ensemble, et le binaire
    corrompt sa pile au premier appel, sans que rien ne l'annonce. C'est une journée perdue ;
    autant la voir venir.

    Le signe : des objets déjà construits (« .ninja_log ») sans journal de dépendances à côté.
#>
function Assert-NinjaDepsIntact {
    param([string]$BuildDir)

    $log = Join-Path $BuildDir '.ninja_log'
    $deps = Join-Path $BuildDir '.ninja_deps'
    if (-not (Test-Path $log)) {
        return  # rien n'a encore été construit ici : la prochaine compilation écrira les deux.
    }
    $depsSize = if (Test-Path $deps) { (Get-Item $deps).Length } else { 0 }
    if ($depsSize -gt 0) {
        return
    }
    throw @"
Ninja a perdu les dépendances d'en-têtes de $BuildDir (.ninja_deps absent ou vide).
Une construction incrémentale y mêlerait des objets compilés contre des en-têtes différents : le
binaire planterait, ou corromprait sa pile sans rien dire. Relancer avec -Clean :
    powershell -File scripts/build.ps1 -Preset $Preset -Clean
"@
}

function Invoke-Step {
    param([string]$Label, [scriptblock]$Action)

    Write-Host "==> $Label" -ForegroundColor Cyan
    & $Action
    if ($LASTEXITCODE -ne 0) {
        throw "$Label : échec (code $LASTEXITCODE)."
    }
}

Enter-X64Environment

if ($QtPath) {
    if (-not (Test-Path $QtPath)) {
        throw "Chemin Qt inexistant : $QtPath"
    }
    # Repris par find_package(Qt6) ; complète la détection automatique de Source/HMI/CMakeLists.txt.
    $env:CMAKE_PREFIX_PATH = $QtPath
}

# Les presets de configuration et de construction ne portent pas toujours le même nom : Visual
# Studio est multi-configuration, donc « vs-release » se construit dans le répertoire de « vs ».
$configurePreset = if ($Preset -eq 'vs-release') { 'vs' } else { $Preset }
$buildDir = Join-Path $repoRoot "build\$configurePreset"
if ($Clean -and (Test-Path $buildDir)) {
    Invoke-Step "Nettoyage de $buildDir" { Remove-Item -Recurse -Force $buildDir; $global:LASTEXITCODE = 0 }
}

# Un répertoire de build dont Ninja a perdu les dépendances produit des binaires incohérents :
# mieux vaut refuser que construire à faux (voir Assert-NinjaDepsIntact).
if ($configurePreset -like 'ninja*') {
    Assert-NinjaDepsIntact -BuildDir $buildDir
}

Push-Location $repoRoot
try {
    # Les images des kits ne sont pas suivies par Git (LOT-108) : les installer avant CMake, qui
    # globe UI/ et Maps/ à la configuration et refuse un kit absent.
    Invoke-Step 'Kits d''assets' { py -3 scripts/fetch_assets.py }
    Invoke-Step 'Configuration (CMake)' { cmake --preset $configurePreset }
    if ($Target) {
        Invoke-Step "Construction ($Target)" { cmake --build --preset $Preset --target $Target }
    } else {
        Invoke-Step 'Construction'      { cmake --build --preset $Preset }
    }
    if ($Label) {
        # Pas `$Label` dans le bloc : Invoke-Step a un paramètre du même nom, qui le masquerait.
        $ctestLabel = $Label
        Invoke-Step "Tests (CTest, $Label)" { ctest --preset $Preset -L $ctestLabel }
    } elseif ($Test) {
        Invoke-Step 'Tests (CTest)'     { ctest --preset $Preset }
    }
}
finally {
    Pop-Location
}

# Emplacement de l'exécutable : le générateur Visual Studio est multi-configuration (sous-dossier
# par configuration), Ninja ne l'est pas.
$exe = switch ($Preset) {
    'vs'         { "$buildDir\bin\Debug\JustAnotherRpgGame.exe" }
    'vs-release' { "$buildDir\bin\Release\JustAnotherRpgGame.exe" }
    default      { "$buildDir\bin\JustAnotherRpgGame.exe" }
}
if (Test-Path $exe) {
    Write-Host "`nExécutable : $exe" -ForegroundColor Green
}
else {
    Write-Warning "Aucun exécutable produit ($exe). Qt6 est probablement introuvable : relancer avec -QtPath <chemin>."
}
