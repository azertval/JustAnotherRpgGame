# Core · Data

Tests unitaires — **12 cas** (5 critiques, 5 majeurs, 2 mineurs). [Retour à la synthèse](README.md).

## test_json_document.cpp

### JsonDocumentPosition.ConvertitUnDecalageEnLigneEtColonne

*Majeur · Unitaire · Brique de lecture JSON* — `Source/Test/Unit/Core/Data/test_json_document.cpp:35`

Une position d'erreur est exprimee en ligne et colonne.

**Étapes**

1. Convertir le decalage du premier octet de la 3e ligne d'un texte connu.

**Résultat attendu**

- Vérifie que `position.line` vaut `3`.
- Vérifie que `position.column` vaut `1`.

### JsonDocumentPosition.RendUnePositionInconnueHorsDuTexte

*Mineur · Unitaire · Brique de lecture JSON* — `Source/Test/Unit/Core/Data/test_json_document.cpp:54`

Un decalage aberrant ne produit pas de position fausse.

**Étapes**

1. Convertir un decalage superieur a la taille du texte.

**Résultat attendu**

- Vérifie que `position.line` vaut `0`.
- Vérifie que `position.column` vaut `0`.

### JsonDocument.LitUnObjetBienForme

*Critique · Unitaire · Brique de lecture JSON* — `Source/Test/Unit/Core/Data/test_json_document.cpp:74`

Un document JSON valide se lit sans erreur.

**Étapes**

1. Lire une chaine JSON contenant une version et un champ.

**Résultat attendu**

- Vérifie que `doc.ok()` est vrai.
- Vérifie que `doc.version` vaut `1`.
- Vérifie que `doc.root.at("a").get<int>()` vaut `2`.

### JsonDocument.VersionAbsenteVautUn

*Majeur · Unitaire · Brique de lecture JSON* — `Source/Test/Unit/Core/Data/test_json_document.cpp:90`

Un champ de version absent vaut 1.

**Étapes**

1. Lire un document sans champ de version, avec une version geree de 3.

**Résultat attendu**

- Vérifie que `doc.ok()` est vrai.
- Vérifie que `doc.version` vaut `1`.

### JsonDocument.RefuseUneVersionPlusRecenteQueLeLecteur

*Critique · Unitaire · Brique de lecture JSON* — `Source/Test/Unit/Core/Data/test_json_document.cpp:106`

Une version de format non geree est refusee, pas lue au mieux.

**Étapes**

1. Lire un document de version 4 avec une version geree de 3.

**Résultat attendu**

- Vérifie que `doc.ok()` est faux.
- Vérifie que `doc.error` vaut `core::JsonReadError::UnsupportedVersion`.
- Vérifie que `doc.message.find('4')` diffère de `std::string::npos`.
- Vérifie que `doc.message.find('3')` diffère de `std::string::npos`.

### JsonDocument.RefuseUneRacineQuiNEstPasUnObjet

*Majeur · Unitaire · Brique de lecture JSON* — `Source/Test/Unit/Core/Data/test_json_document.cpp:126`

Un document dont la racine est un tableau est refuse.

**Étapes**

1. Lire un document dont la racine est un tableau.

**Résultat attendu**

- Vérifie que `doc.ok()` est faux.
- Vérifie que `doc.error` vaut `core::JsonReadError::ParseError`.

### JsonDocument.SitueLErreurDeSyntaxeALaLigne

*Critique · Unitaire · Brique de lecture JSON* — `Source/Test/Unit/Core/Data/test_json_document.cpp:142`

Une erreur de syntaxe nomme le fichier et la ligne.

**Étapes**

1. Lire un document dont la 3e ligne porte une virgule en trop, en nommant l'origine.

**Résultat attendu**

- Vérifie que `doc.ok()` est faux.
- Vérifie que `doc.error` vaut `core::JsonReadError::ParseError`.
- Vérifie que `doc.position.line` vaut `3`.
- Vérifie que `doc.message.find("essai.json:3")` diffère de `std::string::npos`.

### JsonDocument.NeLevePasSurUnNombreHorsDePortee

*Critique · Unitaire · Brique de lecture JSON* — `Source/Test/Unit/Core/Data/test_json_document.cpp:163`

Un nombre JSON trop grand pour etre represente ne fait pas lever la lecture.

**Étapes**

1. Lire un objet dont une valeur vaut 1e400.
2. Lire un tableau dont l'unique nombre porte un exposant de quatorze chiffres (entree trouvee par le fuzzing de nuit).

**Résultat attendu**

- `ASSERT_NO_THROW(doc = core::readJsonObject(R"({"octets": 723e404})", 1, "essai.json"))`
- Vérifie que `doc.error` vaut `core::JsonReadError::ParseError`.
- Vérifie que `doc.message.find("essai.json")` diffère de `std::string::npos`.
- `ASSERT_NO_THROW(doc = core::readJsonObject("[444444444444444E44444444444449]", 1))`
- Vérifie que `doc.error` vaut `core::JsonReadError::ParseError`.

### JsonDocument.NeLevePasSurUnFichierAbsent

*Critique · Unitaire · Brique de lecture JSON* — `Source/Test/Unit/Core/Data/test_json_document.cpp:185`

Un fichier absent est un echec decrit, pas une exception.

**Étapes**

1. Lire un fichier qui n'existe pas.

**Résultat attendu**

- Vérifie que `doc.ok()` est faux.
- Vérifie que `doc.error` vaut `core::JsonReadError::FileNotFound`.

### JsonDocument.NommeLeFichierDansSesMessages

*Majeur · Unitaire · Brique de lecture JSON* — `Source/Test/Unit/Core/Data/test_json_document.cpp:202`

Un echec de lecture de fichier nomme ce fichier.

**Étapes**

1. Lire une fixture JSON tronquee depuis le disque.

**Résultat attendu**

- Vérifie que `doc.ok()` est faux.
- Vérifie que `doc.message.find("tronque.json")` diffère de `std::string::npos`.

### FixtureJson.ProduitLaCategorieAnnoncee

*Majeur · Unitaire · Brique de lecture JSON* — `Source/Test/Unit/Core/Data/test_json_document.cpp:242`

Chaque fixture JSON produit la categorie d'echec attendue.

**Étapes**

1. Lire chacune des six fixtures de Fixtures/Json.
2. Comparer la categorie d'echec obtenue a celle annoncee par le nom du fichier.

**Résultat attendu**

- Vérifie que `doc.error` vaut `attente.attendu`.
- Vérifie que `doc.message.empty()` est faux.

### FixtureJsonCouverture.AucuneFixtureOrpheline

*Mineur · Unitaire · Brique de lecture JSON* — `Source/Test/Unit/Core/Data/test_json_document.cpp:284`

Aucune fixture n'est presente sans etre lue par un test.

**Étapes**

1. Parcourir le dossier de fixtures.
2. Verifier que chaque fichier figure dans la liste des cas instancies.

**Résultat attendu**

- Vérifie que `std::filesystem::is_directory(FIXTURES)` est vrai.
- Vérifie que `std::find(couvertes.begin(), couvertes.end(), nom)` diffère de `couvertes.end()`.
