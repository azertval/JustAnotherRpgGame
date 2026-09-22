"""Un tour de la méthode standard par l'API. Usage : py -3.13 tour.py <slug> <planche|marche|idle|hit|death|attack|cast> <tour>
Prompt et références : prompts.py (les mêmes que chatgpt.py). Chaque tour est une génération complète
depuis le prompt initial : on ne corrige pas une planche par édition, on corrige le prompt (décision du 16 septembre).
« corps » et « effets » ne sont plus générés : séparés, ils divergeaient (disposition_*.md gardées pour mémoire)."""
import base64
import json
import datetime
import hashlib
import sys
import os
from openai import OpenAI
from prompts import ROOT, TAILLES, etape
MODEL = os.environ.get("NPC_MODEL", "gpt-image-2")   # tailles libres (multiples de 16) depuis gpt-image-2
slug, dispo, k = sys.argv[1], sys.argv[2], int(sys.argv[3])
tour = ROOT / slug / f"tour{k}" / dispo; tour.mkdir(parents=True, exist_ok=True)
prompt, refs = etape(dispo, slug); refs = [p for _, p in refs]
r = OpenAI().images.edit(model=MODEL, image=[open(p, "rb") for p in refs], prompt=prompt,
                         n=3, size=TAILLES[dispo], quality="high", output_format="png", background="transparent")
for i, img in enumerate(r.data, 1):
    (tour / f"candidat{i}.png").write_bytes(base64.b64decode(img.b64_json))
usage = getattr(r, "usage", None)
(tour / "journal.json").write_text(json.dumps({
    "date": datetime.datetime.now().isoformat(timespec="seconds"), "model": MODEL, "disposition": dispo, "prompt": prompt,
    "refs": {p.as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in refs},
    "n": 3, "size": TAILLES[dispo], "quality": "high", "background": "transparent",
    "usage": usage.model_dump() if usage else None}, indent=2, ensure_ascii=False), encoding="utf-8")
print("ok", tour, "usage:", usage)
