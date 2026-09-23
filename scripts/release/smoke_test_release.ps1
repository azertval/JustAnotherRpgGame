<#
.SYNOPSIS
    Test de fumée d'une archive livrée : la décompresser ailleurs, lancer le jeu, obtenir une image.

.DESCRIPTION
    Le build vérifie que l'exécutable existe ; rien ne prouvait que l'ARCHIVE publiée se lance sur une
    machine qui n'a ni Qt ni le dossier de build. windeployqt peut oublier un plugin QML, un module,
    le runtime : le zip est alors complet en apparence et le jeu ne s'ouvre pas (refonte de la CI,
    phase 3).

    Le script décompresse l'archive dans un dossier neuf, lance `JustAnotherRpgGame.exe
    --screenshot=<png>` — le mode du jeu qui charge l'interface, rend une image et quitte seul — et
    exige :
      - un code de sortie 0 dans le délai (1 = QML non chargé ou capture refusée, 2 = délai interne) ;
      - une image PNG écrite, qui n'est pas d'une seule couleur (une fenêtre noire n'est pas un jeu).
    Puis il relance le jeu avec `--crash-test`, qui plante volontairement (phase 4), et exige :
      - une fin par violation d'accès (0xC0000005), sans boîte de dialogue qui bloquerait le runner ;
      - un minidump sous Crashes/, signature MDMP, dont le chemin est consigné dans le journal.
    C'est la seule preuve que l'archive livrée écrit bien son dump : filtre installé, DbgHelp chargé.
    En cas d'échec, la fin du journal du jeu (Logs/) est affichée.

    Utilisé par .github/workflows/release.yml avant toute publication, et par le job `smoke` de
    nightly.yml. Rejouable en local :
    `pwsh scripts/release/smoke_test_release.ps1 -Zip dist/JustAnotherRpgGame-debug.zip -Screenshot smoke.png`

.PARAMETER Zip
    L'archive du jeu produite par scripts/release/package_release.ps1.

.PARAMETER Screenshot
    Où conserver la capture (utile en artefact). Par défaut : dans le dossier temporaire, supprimée.

.PARAMETER TimeoutSeconds
    Délai au-delà duquel le processus est tué. Le jeu s'arrête seul à 15 s ; la marge couvre un
    premier lancement lent (cache de shaders, antivirus du runner).
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string] $Zip,
    [string] $Screenshot,
    [int] $TimeoutSeconds = 90
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$archive = (Resolve-Path -LiteralPath $Zip).Path
$stage = Join-Path ([System.IO.Path]::GetTempPath()) ("jadg-smoke-" + [guid]::NewGuid())
if (-not $Screenshot) { $Screenshot = Join-Path $stage 'smoke.png' }
$Screenshot = [System.IO.Path]::GetFullPath($Screenshot)

function Show-GameLog([string] $Directory) {
    $logs = @(Get-ChildItem -LiteralPath (Join-Path $Directory 'Logs') -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime)
    if ($logs.Count -eq 0) {
        Write-Host 'Aucun journal écrit dans Logs/.'
        return
    }
    Write-Host "--- Fin de $($logs[-1].Name) ---"
    Get-Content -LiteralPath $logs[-1].FullName -Tail 40 | ForEach-Object { Write-Host $_ }
}

try {
    $game = Join-Path $stage 'game'
    New-Item -ItemType Directory -Force -Path $game, (Split-Path -Parent $Screenshot) | Out-Null
    Expand-Archive -LiteralPath $archive -DestinationPath $game -Force

    $exe = Join-Path $game 'JustAnotherRpgGame.exe'
    if (-not (Test-Path -LiteralPath $exe)) { throw "JustAnotherRpgGame.exe absent de l'archive $archive." }
    if (Test-Path -LiteralPath $Screenshot) { Remove-Item -LiteralPath $Screenshot -Force }

    Write-Host "Lancement : $exe --screenshot=$Screenshot"
    $process = Start-Process -FilePath $exe -ArgumentList "--screenshot=`"$Screenshot`"" `
        -WorkingDirectory $game -PassThru
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill()
        Show-GameLog $game
        throw "Le jeu ne s'est pas arrêté en $TimeoutSeconds s."
    }
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) {
        Show-GameLog $game
        throw ("Le jeu a quitté avec le code $($process.ExitCode) " +
               "(1 : interface non chargée ou capture refusée ; 2 : délai interne).")
    }
    if (-not (Test-Path -LiteralPath $Screenshot)) {
        Show-GameLog $game
        throw "Code de sortie 0 mais aucune capture écrite : $Screenshot."
    }

    # Une capture d'une seule couleur : la fenêtre s'est ouverte mais rien n'a été rendu.
    Add-Type -AssemblyName System.Drawing
    $image = [System.Drawing.Bitmap]::FromFile($Screenshot)
    try {
        $colors = [System.Collections.Generic.HashSet[int]]::new()
        foreach ($i in 1..24) {
            foreach ($j in 1..24) {
                $x = [int](($image.Width - 1) * $i / 25)
                $y = [int](($image.Height - 1) * $j / 25)
                [void]$colors.Add($image.GetPixel($x, $y).ToArgb())
            }
        }
        $width = $image.Width
        $height = $image.Height
    }
    finally {
        $image.Dispose()
    }
    if ($colors.Count -lt 2) {
        Show-GameLog $game
        throw "Capture uniforme ($width x $height) : la fenêtre ne rend rien."
    }
    Write-Host ("OK : le jeu de $(Split-Path -Leaf $archive) se lance et rend une image " +
                "($width x $height, $($colors.Count) couleurs échantillonnées).")

    # --- Plantage volontaire : le minidump est-il écrit ? ----------------------------------------
    Write-Host "Lancement : $exe --crash-test"
    $crash = Start-Process -FilePath $exe -ArgumentList '--crash-test' -WorkingDirectory $game -PassThru
    if (-not $crash.WaitForExit($TimeoutSeconds * 1000)) {
        $crash.Kill()
        Show-GameLog $game
        throw "--crash-test : le jeu ne s'est pas arrêté en $TimeoutSeconds s (boîte de dialogue ?)."
    }
    $crash.WaitForExit()
    $accessViolation = -1073741819  # 0xC0000005, en entier signé
    if ($crash.ExitCode -ne $accessViolation) {
        Show-GameLog $game
        throw ("--crash-test : code de sortie $($crash.ExitCode) (0x{0:X8}), violation d'accès 0xC0000005 attendue." -f $crash.ExitCode)
    }
    $dumps = @(Get-ChildItem -LiteralPath (Join-Path $game 'Crashes') -Filter '*.dmp' -File -ErrorAction SilentlyContinue)
    # Exactement un : l'archive n'en livre aucun (package_release.ps1 écarte Crashes/), et le
    # lancement ci-dessus en écrit un seul.
    if ($dumps.Count -ne 1) {
        Show-GameLog $game
        throw "--crash-test : $($dumps.Count) minidump(s) dans Crashes/, un seul attendu (l'archive en livre-t-elle ?)."
    }
    $bytes = [System.IO.File]::ReadAllBytes($dumps[0].FullName)
    $signature = [System.Text.Encoding]::ASCII.GetString($bytes, 0, [Math]::Min(4, $bytes.Length))
    if ($signature -ne 'MDMP') {
        throw "--crash-test : $($dumps[0].Name) n'est pas un minidump (signature « $signature »)."
    }
    $logged = Get-ChildItem -LiteralPath (Join-Path $game 'Logs') -File |
        Select-String -SimpleMatch $dumps[0].Name -Quiet
    if (-not $logged) {
        Show-GameLog $game
        throw "--crash-test : le journal ne cite pas $($dumps[0].Name)."
    }
    Write-Host ("OK : le plantage volontaire écrit $($dumps[0].Name) " +
                "($([Math]::Round($dumps[0].Length / 1KB)) Kio), cité dans le journal.")
}
finally {
    # La capture demandée par -Screenshot vit hors de ce dossier et reste.
    if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
}
