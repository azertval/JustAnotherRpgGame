"""Portrait pixel art d'un PNJ par l'API (§2.2). Usage : py -3.13 portrait.py <slug> <tour>
Prompt et références : prompts.py (les mêmes que chatgpt.py). Deux candidats dans <slug>/token/tour<K>/."""
import base64
import json
import datetime
import hashlib
import sys
import os
from openai import OpenAI
from prompts import ROOT, TAILLES, etape
MODEL = os.environ.get("NPC_MODEL", "gpt-image-2")
slug, k = sys.argv[1], int(sys.argv[2]); out = ROOT / slug / "token" / f"tour{k}"; out.mkdir(parents=True, exist_ok=True)
prompt, refs = etape("portrait", slug); refs = [p for _, p in refs]
r = OpenAI().images.edit(model=MODEL, image=[open(p, "rb") for p in refs], prompt=prompt,
                         n=2, size=TAILLES["portrait"], quality="high", output_format="png")
for i, img in enumerate(r.data, 1):
    (out / f"candidat{i}.png").write_bytes(base64.b64decode(img.b64_json))
usage = getattr(r, "usage", None)
(out / "journal.json").write_text(json.dumps({
    "date": datetime.datetime.now().isoformat(timespec="seconds"), "model": MODEL, "prompt": prompt,
    "refs": {p.as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in refs},
    "n": 2, "size": TAILLES["portrait"], "quality": "high", "usage": usage.model_dump() if usage else None},
    indent=2, ensure_ascii=False), encoding="utf-8")
print("ok", out, "usage:", usage)
