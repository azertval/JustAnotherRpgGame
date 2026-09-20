<#
.SYNOPSIS
    Ecrit SHA256SUMS pour les archives d'une release.

.DESCRIPTION
    Format de `sha256sum` (empreinte en minuscules, deux espaces, nom de fichier), pour que la
    verification tienne en une commande sur n'importe quel systeme :
    `sha256sum -c SHA256SUMS` sous Linux ou Git Bash, ou `Get-FileHash` sous Windows.
    Seul le NOM du fichier est ecrit, pas son chemin : le fichier se verifie a cote des archives
    telechargees.

.PARAMETER Path
    Archives a empreinter.

.PARAMETER OutFile
    Fichier a ecrire. Par defaut : SHA256SUMS dans le dossier courant.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string[]] $Path,
    [string] $OutFile = 'SHA256SUMS'
)

$ErrorActionPreference = 'Stop'

$lines = foreach ($file in $Path) {
    $item = Get-Item -LiteralPath $file
    $hash = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $($item.Name)"
}

# LF et sans BOM : `sha256sum -c` refuse un BOM en tete et lit mal un CR en fin de nom.
$full = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutFile)
[System.IO.File]::WriteAllText($full, (($lines -join "`n") + "`n"), (New-Object System.Text.UTF8Encoding $false))
Get-Content -LiteralPath $full | ForEach-Object { Write-Host $_ }
