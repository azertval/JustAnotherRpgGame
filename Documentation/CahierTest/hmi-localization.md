# HMI · Localization

Tests unitaires — **9 cas** (9 majeurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_localization.cpp`](#test-localizationcpp) | 9 | - | - | 9 | - |

## test_localization.cpp

### LocalizationTest.AnalyseIgnoreCommentairesEtEspaces

*Majeur · Unitaire · Localization* — `Source/Test/Unit/HMI/Localization/test_localization.cpp:35`

L'analyse ignore les lignes vides et les commentaires, et retire les espaces autour de '='.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `strings.size()` vaut `2u`.
- Vérifie que `strings.at("menu.quitter")` vaut `"Quitter"`.
- Vérifie que `strings.at("menu.titre")` vaut `"JustAnotherRpgGame"`.

### LocalizationTest.AnalyseConserveEgalDansLaValeur

*Majeur · Unitaire · Localization* — `Source/Test/Unit/HMI/Localization/test_localization.cpp:60`

Seul le premier '=' sépare ; un '=' dans la valeur est conservé.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `strings.size()` vaut `1u`.
- Vérifie que `strings.at("expression")` vaut `"a = b + c"`.

### LocalizationTest.CleExistanteResolue

*Majeur · Unitaire · Localization* — `Source/Test/Unit/HMI/Localization/test_localization.cpp:77`

Une clé existante est résolue dans la langue active.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `localization.text("menu.quitter")` vaut `"Quitter"`.
- Vérifie que `localization.activeLanguage()` vaut `"fr"`.

### LocalizationTest.CleInconnueRenvoieLaCle

*Majeur · Unitaire · Localization* — `Source/Test/Unit/HMI/Localization/test_localization.cpp:95`

Une clé inconnue partout est renvoyée telle quelle (repli déterministe, pas de plantage).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `localization.text("menu.inconnue")` vaut `"menu.inconnue"`.

### LocalizationTest.ChangementDeLangue

*Majeur · Unitaire · Localization* — `Source/Test/Unit/HMI/Localization/test_localization.cpp:114`

Changer de langue résout les valeurs de la nouvelle langue.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `localization.activeLanguage()` vaut `"en"`.
- Vérifie que `localization.text("menu.quitter")` vaut `"Quit"`.

### LocalizationTest.RepliSurLangueParDefaut

*Majeur · Unitaire · Localization* — `Source/Test/Unit/HMI/Localization/test_localization.cpp:133`

Une clé manquante dans la langue active retombe sur la langue par défaut.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `localization.text("menu.quitter")` vaut `"Quit"`.
- Vérifie que `localization.text("menu.titre")` vaut `"Jeu"`.

### LocalizationTest.LangueAbsenteEstRecuperable

*Majeur · Unitaire · Localization* — `Source/Test/Unit/HMI/Localization/test_localization.cpp:152`

Charger une langue absente échoue proprement et conserve la langue active (récupérable).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `localization.loadLanguage("xx")` est faux.
- Vérifie que `localization.activeLanguage()` vaut `"fr"`.
- Vérifie que `localization.text("menu.quitter")` vaut `"Quitter"`.

### LocalizationTest.CatalogueFrancaisLivreSeCharge

*Majeur · Unitaire · Localization* — `Source/Test/Unit/HMI/Localization/test_localization.cpp:173`

Le catalogue français livré (Source/Elements/Localization) se charge et résout ses clés.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `localization.loadDefaultLanguage("fr")` est vrai.
- Vérifie que `localization.activeLanguage()` vaut `"fr"`.
- Vérifie que `localization.text("menu.quit")` vaut `"Quitter"`.
- Vérifie que `localization.text("menu.new_game")` vaut `"Nouvelle partie"`.

### LocalizationTest.LesDeuxCataloguesDeclarentLesMemesCles

*Majeur · Unitaire · Localization* — `Source/Test/Unit/HMI/Localization/test_localization.cpp:201`

Les catalogues francais et anglais declarent les memes cles.

**Étapes**

1. Charger fr.lang et en.lang.
2. Comparer les ensembles de cles dans les deux sens.

**Résultat attendu**

- Vérifie que `french.empty()` est faux.
- Vérifie que `english.empty()` est faux.
- Vérifie que `english.count(key) > 0` est vrai.
- Vérifie que `french.count(key) > 0` est vrai.
- Vérifie que `french.size()` vaut `english.size()`.
