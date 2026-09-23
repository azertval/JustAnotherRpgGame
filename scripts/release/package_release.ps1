<#
.SYNOPSIS
    Empaquette un dossier d'executable en deux archives : le jeu, et ses symboles.

.DESCRIPTION
    Le dossier de sortie du build (bin/<Config>) contient deja tout ce que le jeu demande :
    l'executable, les DLL Qt et le plugin de plateforme deposes par windeployqt, le runtime du
    compilateur et les dossiers d'assets. Il contient aussi les .pdb du linker.

    Les .pdb ne servent pas a jouer, mais sans le .pdb de la version EXACTE un minidump de plantage
    est illisible : ils partent donc dans une archive a part, `<Name>-symbols.zip`, publiee a cote
    du jeu, plutot que d'etre jetes ou d'alourdir l'archive que telecharge un joueur. Les .ilk
    (base du linker incremental) ne servent a rien hors du poste qui les a produits : ecartes.

    Les dossiers qu'ECRIT le jeu en s'executant -- `Logs/` et `Crashes/` (minidumps, phase 4) -- sont
    ecartes aussi : un lancement depuis le dossier de build (test de fumee, capture) les y laisse, et
    l'archive livrerait alors les journaux et les plantages du poste qui l'a construite.

    Lire un minidump d'une version livree : decompresser `<Name>-symbols.zip` a cote du jeu de la
    MEME version, ouvrir le `.dmp` de `Crashes/` dans Visual Studio (Fichier > Ouvrir), puis
    "Deboguer en mode natif uniquement". Le nom du dump porte la version qui l'a ecrit.

    Le script n'ecrit que dans OutDir et dans un dossier temporaire qu'il supprime ; le dossier de
    build n'est jamais modifie. Utilise par .github/workflows/release.yml, rejouable en local :
    `pwsh scripts/release/package_release.ps1 -BinDir build/vs/bin/Debug -Name JustAnotherRpgGame-debug`.

.PARAMETER BinDir
    Dossier de l'executable a empaqueter (ex. build-release/bin/Debug).

.PARAMETER Name
    Nom de base des archives, sans extension.

.PARAMETER OutDir
    Dossier ou deposer les archives (cree s'il manque). Par defaut : le dossier courant.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string] $BinDir,
    [Parameter(Mandatory)] [string] $Name,
    [string] $OutDir = '.'
)

$ErrorActionPreference = 'Stop'

$source = (Resolve-Path -LiteralPath $BinDir).Path
if (-not (Test-Path -LiteralPath (Join-Path $source 'JustAnotherRpgGame.exe'))) {
    # Meme panne muette que celle gardee par ci.yml : sans Qt, le build reussit sans application.
    throw "JustAnotherRpgGame.exe absent de $source : rien a empaqueter."
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$out = (Resolve-Path -LiteralPath $OutDir).Path

$stage = Join-Path ([System.IO.Path]::GetTempPath()) ("jadg-package-" + [guid]::NewGuid())
$gameStage = Join-Path $stage 'game'
$symbolsStage = Join-Path $stage 'symbols'

try {
    New-Item -ItemType Directory -Force -Path $gameStage, $symbolsStage | Out-Null
    Copy-Item -Path (Join-Path $source '*') -Destination $gameStage -Recurse -Force

    Get-ChildItem -LiteralPath $gameStage -Recurse -File -Filter '*.ilk' | Remove-Item -Force
    foreach ($runtime in 'Logs', 'Crashes') {
        $folder = Join-Path $gameStage $runtime
        if (Test-Path -LiteralPath $folder) { Remove-Item -LiteralPath $folder -Recurse -Force }
    }

    $pdbs = @(Get-ChildItem -LiteralPath $gameStage -Recurse -File -Filter '*.pdb')
    foreach ($pdb in $pdbs) {
        # Chemin relatif conserve : un .pdb de plugin reste a cote de son sous-dossier.
        $relative = $pdb.FullName.Substring($gameStage.Length).TrimStart('\', '/')
        $target = Join-Path $symbolsStage $relative
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
        Move-Item -LiteralPath $pdb.FullName -Destination $target
    }
    if ($pdbs.Count -eq 0) {
        # Une release sans symboles ne se rattrape pas apres coup : le binaire publie ne peut plus
        # etre reconstruit a l'identique. Mieux vaut echouer ici.
        throw "Aucun .pdb dans $source : les options de symboles du build sont absentes."
    }

    $gameZip = Join-Path $out "$Name.zip"
    $symbolsZip = Join-Path $out "$Name-symbols.zip"
    Compress-Archive -Path (Join-Path $gameStage '*') -DestinationPath $gameZip -Force
    Compress-Archive -Path (Join-Path $symbolsStage '*') -DestinationPath $symbolsZip -Force

    foreach ($zip in @($gameZip, $symbolsZip)) {
        $size = [math]::Round((Get-Item -LiteralPath $zip).Length / 1MB, 1)
        Write-Host "OK : $zip ($size Mio)"
    }
    Write-Host ("Symboles : " + ($pdbs.Name -join ', '))
}
finally {
    Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue
}
