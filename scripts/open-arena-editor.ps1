<# Opens the arena in the native editor, saving directly into Source/Elements. #>
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$arenaRoot = Split-Path -Parent $PSScriptRoot
$arenaEditor = Join-Path $arenaRoot 'build/ninja/bin/LevelEditor.exe'
if (-not (Test-Path -LiteralPath $arenaEditor)) {
    throw 'Build LevelEditor first with scripts/build.ps1 -Target LevelEditor.'
}
$arenaData = Join-Path $arenaRoot 'Source/Elements'
Start-Process -FilePath $arenaEditor -WorkingDirectory (Split-Path -Parent $arenaEditor) `
    -WindowStyle Hidden -ArgumentList "--data", "`"$arenaData`"", '--map=capital/arena-of-brave'
