"""Atelier des PNJ (LOT-91), étape I : intègre un modèle fini dans le projet et le coche dans l'epic.
Usage : py -3.13 integre.py <slug> [<slug> ...]
Pour chaque PNJ :
  - TRAVAIL/<slug>/final/bandes/*.png (sortie de compose.py, sans erreur) -> Source/Elements/Assets/Npc/<slug>/ ;
  - un .anim.json par bande, depuis la table du jeu d'animations (epic, « Le jeu d'animations ») ;
  - pnj/<slug>/portrait.png (le portrait pixel art, 5e référence de la planche) -> Assets/Npc/<slug>/portrait.png ;
  - le slug ajouté à « npcs » de Assets/Npc/manifest.json (« replaces » n'est pas touché) ;
  - la case du PNJ cochée dans epic.md.
Pas de vérification en jeu : on fait confiance à la méthode (décision de sortie du PoC)."""
import json
import re
import shutil
import sys
from PIL import Image
from prompts import DEPOT, PNJ, TRAVAIL

PROJET = DEPOT.parents[3]
NPC = PROJET / "Source" / "Elements" / "Assets" / "Npc"
EPIC = DEPOT.parent / "epic.md"
# animation : (images, largeur de cellule, durée par image, boucle)
JEU = {"idle": (6, 48, 0.15, True), "walk": (8, 48, 0.10, True), "hit": (4, 48, 0.08, False),
       "death": (6, 96, 0.12, False), "attack": (8, 96, 0.08, False), "cast": (8, 96, 0.10, False)}

for slug in sys.argv[1:]:
    final = TRAVAIL / slug / "final"
    rapport = json.loads((final / "planche.json").read_text(encoding="utf-8"))
    if rapport["erreurs"]:
        raise SystemExit(f"{slug} : final/planche.json porte des erreurs, compose.py doit rendre 0 d'abord")
    portrait = PNJ / slug / "portrait.png"
    if not portrait.exists():
        raise SystemExit(f"{slug} : {portrait} absent")
    dossier = NPC / slug; dossier.mkdir(parents=True, exist_ok=True)
    for anim, (n, largeur, duree, boucle) in JEU.items():
        bande = final / "bandes" / f"{anim}.png"
        if Image.open(bande).size != (n * largeur, 64):
            raise SystemExit(f"{slug} : {bande.name} fait {Image.open(bande).size}, attendu {(n * largeur, 64)}")
        shutil.copy(bande, dossier / bande.name)
        clip = {"version": 1, "frameWidth": largeur, "frameHeight": 64,
                "clips": {anim: {"frames": list(range(n)), "frameDuration": duree, "loop": boucle}}}
        (dossier / f"{anim}.anim.json").write_text(json.dumps(clip, indent=2) + "\n", encoding="utf-8", newline="\n")
    shutil.copy(portrait, dossier / "portrait.png")
    manifeste = json.loads((NPC / "manifest.json").read_text(encoding="utf-8"))
    if slug not in manifeste["npcs"]:
        manifeste["npcs"].append(slug)
    (NPC / "manifest.json").write_text(json.dumps(manifeste, indent=2, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    texte = EPIC.read_text(encoding="utf-8")
    coche, n = re.subn(rf"^- \[ \] (.*`{re.escape(slug)}`)", r"- [x] \1", texte, flags=re.M)
    if n == 0 and not re.search(rf"^- \[x\] .*`{re.escape(slug)}`", texte, re.M):
        print(f"{slug} : AVERTISSEMENT, pas de ligne « - [ ] … `{slug}` » dans epic.md")
    EPIC.write_text(coche, encoding="utf-8", newline="\n")
    print(f"{slug} : intégré dans {dossier.relative_to(PROJET)}")
