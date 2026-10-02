# HMI/Platform/

Intégration au système d'exploitation.

- `ExecutableDirectory` — dossier contenant l'exécutable en cours, pour localiser les ressources
  copiées à côté (niveaux, catalogues de langue) indépendamment du répertoire de travail.
- `CrashDump` — minidump écrit sous `Crashes/` sur toute fin anormale (exception structurée,
  `std::terminate`, appel virtuel pur, paramètre invalide de la CRT), lisible avec l'archive de
  symboles de la release. Installé par `App/Common/Bootstrap.cpp` pour les deux applications.

Réf. specs : `EX-REN-001`, `EX-REN-003`.
