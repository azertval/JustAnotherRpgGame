# HMI · Audio

Tests unitaires — **3 cas** (1 critique, 1 majeur, 1 mineur). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_audio_engine.cpp`](#test-audio-enginecpp) | 3 | - | 1 | 1 | 1 |

## test_audio_engine.cpp

### AudioEngine.MutedEngineAcceptsPreloadAndPlayWithoutError

*Critique · Unitaire · Moteur audio* — `Source/Test/Unit/HMI/Audio/test_audio_engine.cpp:15`

Un moteur audio muet accepte precharge et lecture sans erreur.

**Étapes**

1. Construire un AudioEngine force en etat muet (ForceMuted::Yes).
2. Precharger un echantillon sur un chemin inexistant.
3. Jouer cet echantillon, puis un identifiant jamais precharge.

**Résultat attendu**

- Vérifie que `engine.muted()` est vrai.

### AudioEngine.VolumeIsClampedToUnitRange

*Majeur · Unitaire · Moteur audio* — `Source/Test/Unit/HMI/Audio/test_audio_engine.cpp:37`

Le volume du moteur audio est borne a [0, 1].

**Étapes**

1. Regler un volume negatif, puis un volume superieur a 1, puis une valeur intermediaire.

**Résultat attendu**

- Vérifie que `engine.volume()` vaut `0.0f` (comparaison flottante).
- Vérifie que `engine.volume()` vaut `1.0f` (comparaison flottante).
- Vérifie que `engine.volume()` vaut `0.42f` (comparaison flottante).

### AudioEngine.DefaultVolumeIsFull

*Mineur · Unitaire · Moteur audio* — `Source/Test/Unit/HMI/Audio/test_audio_engine.cpp:61`

Le volume par defaut du moteur audio est au maximum.

**Étapes**

1. Construire un AudioEngine sans regler de volume.

**Résultat attendu**

- Vérifie que `engine.volume()` vaut `1.0f` (comparaison flottante).
