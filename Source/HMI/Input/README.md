# HMI/Input/

Traduction des entrées clavier de l'éditeur. Le jeu lit ses touches et la souris par les
événements Qt Quick.

- `Key` : touche enregistrée par les raccourcis de l'éditeur, identifiée par son code virtuel Win32.
- `QtKeyMap` : traduction d'un code `Qt::Key` en `hmi::Key` (code virtuel Win32), et l'inverse.

Réf. specs : `EX-CTRL-001`, `EX-CTRL-012`, `EX-CTRL-020`, `EX-CTRL-022`.
