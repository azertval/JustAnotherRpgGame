# Règles de PSScriptAnalyzer pour les scripts PowerShell du dépôt (refonte de la CI, phase 4).
# Lu par scripts/check_powershell.py, en CI (job lint-exigences) comme sur le poste.
@{
    Severity     = @('Error', 'Warning')

    # Write-Host est ici le bon outil : ces scripts parlent à un terminal (tableaux colorés de
    # setup_dev.ps1, étapes de build.ps1) et ne renvoient rien dans le pipeline. La règle vise les
    # fonctions de module dont la sortie doit rester capturable.
    ExcludeRules = @('PSAvoidUsingWriteHost')

    Rules        = @{
        # Le poste exécute Windows PowerShell 5.1 (build.ps1, setup_dev.ps1) et le runner pwsh 7 :
        # une syntaxe propre à la 7 (`&&`, `??`, ternaire) casse le poste sans que la CI le voie.
        PSUseCompatibleSyntax = @{
            Enable         = $true
            TargetVersions = @('5.1', '7.4')
        }
    }
}
