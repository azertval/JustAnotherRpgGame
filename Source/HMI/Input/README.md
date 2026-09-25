# HMI/Input/

Acquisition des entrées et traduction en **actions logiques**.

- `InputState` : état de la **manette** par relevé, avec fronts **pressé / maintenu / relâché**
  (`EX-CTRL-011`), échantillonné une fois par relevé. Indépendant de toute fenêtre (aucun
  `<Windows.h>`), donc testable en isolation. Porte aussi `Key`, la touche par code virtuel Win32
  que les raccourcis de l'éditeur enregistrent ; le clavier du jeu passe par les événements Qt.
- `GamepadPoller` : sondage XInput de la manette, versé dans un `InputState`.
- `GamepadButton` : bouton (ou direction) manette logique, indépendant de toute touche clavier.
- `ButtonRepeat` : répétition d'un bouton tenu — un pas à l'appui, puis un pas régulier après un
  délai.
- `QtKeyMap` : traduction d'un code `Qt::Key` en `hmi::Key` (code virtuel Win32), et l'inverse.

Réf. specs : `EX-CTRL-001`…`EX-CTRL-021`.
