#!/usr/bin/env python3
"""Build the LOT-105 review gallery from installed manifests (no image changes)."""

import json
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "Tools/AssetsHD/Regions/central-empire/capital/Common/apercus/index.html"
ASSETS = ROOT / "Source/Elements/Assets/Regions/central-empire"


def main():
    entries = []
    for folder, label in [("capital/Common/Scene", "Capitale"), ("Common/Scene", "Empire")]:
        directory = ASSETS / folder
        data = json.loads((directory / "manifest.json").read_text(encoding="utf-8"))
        for entry in data["textures"].values():
            path = directory / entry["file"]
            entries.append(
                dict(
                    entry,
                    name=path.stem,
                    kit=label,
                    url=Path(os.path.relpath(path, OUT.parent)).as_posix(),
                    bytes=path.stat().st_size,
                )
            )
    template = """<!doctype html><html lang="fr"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>LOT-105 · Capitale</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#161c1e;color:#efe6d2;font:16px system-ui}header,main{max-width:1500px;margin:auto;padding:28px}header{border-bottom:1px solid #5c4a2a}h1{font:42px Georgia;margin:8px 0}p{color:#c5c3ba;line-height:1.6}.eyebrow{color:#c9a45c;letter-spacing:.18em;font-size:12px}button,select,input{font:inherit}button,select{background:#293034;color:#efe6d2;border:1px solid #5c4a2a;border-radius:6px;padding:9px}button{cursor:pointer}.controls{display:flex;gap:18px;align-items:center;flex-wrap:wrap;margin:20px 0}.scene{overflow:auto;background:#242b2d;border-radius:12px}canvas{display:block;width:100%;min-width:800px}#cards{display:grid;grid-template-columns:repeat(auto-fill,minmax(250px,1fr));gap:16px}article{background:#242b2d;border:1px solid #41413a;border-radius:10px;overflow:hidden}article img{display:block;width:100%;height:260px;object-fit:contain;padding:16px;background:repeating-conic-gradient(#293134 0% 25%,#242b2d 0% 50%) 50%/24px 24px}article div{padding:14px}h3{font-size:14px;overflow-wrap:anywhere;margin:0 0 8px}.meta{font-size:12px;color:#b8b7ab}a{color:#d9c7a3}.light article img,.light .scene{background:#d6d3c8}h2{font:28px Georgia;margin-top:38px}
</style><header><div class="eyebrow">LOT-105 · KIT COMMUN</div><h1>Les rues de la Capitale</h1>
<p id="summary"></p><p>Révision 4 · Sources redessinées et nettoyées. Angles construits sur cases entières, surfaces communes et sols à joints continus, grille 256 × 159 px.</p></header>
<main><p><a href="validation.html">Ouvrir la map de validation complète — 38 assets, grille et sélection</a></p><h2>Une rue de douze cases de long</h2><p>Composition avec le seul kit, sur trois cases de large. Les images sont placées avec les ancres des manifestes.</p>
<div class="controls"><label>Sol <select id="floor"><option value="floor-paving">Pavés</option><option value="floor-flagstone">Dalles</option></select></label><label>Direction <select id="direction"><option value="u">U ↘</option><option value="v">V ↙</option></select></label><label><input id="grid" type="checkbox" checked> Grille</label><button id="background">Fond clair / sombre</button></div>
<div class="scene"><canvas id="street" width="2200" height="1640"></canvas></div>
<h2>Les 38 pièces sur leur emprise</h2><p>Le contour turquoise indique les cases occupées. Le point doré indique le sommet nord, utilisé pour placer la pièce. Cliquer pour ouvrir le PNG.</p><div class="controls"><label>Famille <select id="family"><option value="">Toutes</option><option>01</option><option>02</option><option>03</option><option>05</option><option>07</option><option>08</option></select></label></div><section id="cards"></section></main>
<script>
const assets=__DATA__,byName=Object.fromEntries(assets.map(a=>[a.name,a]));
document.querySelector('#summary').textContent=`${assets.length} pièces · ${(assets.reduce((n,a)=>n+a.bytes,0)/1048576).toFixed(2)} Mio · Révision 4 · PNG transparents`;
const cards=document.querySelector('#cards');function gallery(){cards.replaceChildren();for(const a of assets){if(family.value&&a.family!==family.value)continue;const card=document.createElement('article');const link=document.createElement('a');link.href=a.url;link.target='_blank';const im=document.createElement('img');im.src=a.url+'?v='+a.source.sha256.slice(0,12);im.alt=a.name;link.append(im);const info=document.createElement('div');const title=document.createElement('h3');title.textContent=a.name;const meta=document.createElement('span');meta.className='meta';meta.textContent=`${a.kit} · ${a.family} · ${a.footprint.join(' × ')} cases · ${a.size.join(' × ')} px`;info.append(title,meta);card.append(link,info);cards.append(card)}}
const family=document.querySelector('#family');family.onchange=gallery;gallery();
const images={};const ctx=document.querySelector('#street').getContext('2d');const project=(u,v)=>[(document.querySelector('#direction').value==='v'?1680:520)+(u-v)*128,350+(u+v)*79.5];
function piece(name,u,v){const a=byName[name],im=images[name],[x,y]=project(u,v);ctx.drawImage(im,x-a.anchor[0],y-a.anchor[1]);}
function draw(){ctx.clearRect(0,0,2200,1640);const flip=document.querySelector('#direction').value==='v',C=flip?3:12,R=flip?12:3;for(let u=0;u<C;u++)for(let v=0;v<R;v++)piece(document.querySelector('#floor').value+'-0'+((u+v)%3+1),u,v);
const dir=flip?'v':'u',placed=[];for(let k=0;k<12;k+=2)placed.push([flip?0:k,flip?k:0,k===4?'wall-limestone-banner-'+dir:k%4?'wall-limestone-window-'+dir:'wall-limestone-'+dir]);
for(const [k,n] of [[0,'plant-cypress'],[2,'prop-bench-'+dir],[3,'prop-lamppost'],[5,'prop-planter'],[7,'prop-stall-empty'],[8,'prop-barrel'],[9,'prop-crate'],[10,'prop-lamppost'],[11,'prop-banner-lion']])placed.push([flip?2:k,flip?k:2,n]);
placed.sort((a,b)=>(a[0]+a[1])-(b[0]+b[1]));for(const [u,v,n]of placed)piece(n,u,v);
if(document.querySelector('#grid').checked){ctx.strokeStyle='#58d7d7';ctx.lineWidth=1;for(let u=0;u<=C;u++){ctx.beginPath();ctx.moveTo(...project(u,0));ctx.lineTo(...project(u,R));ctx.stroke()}for(let v=0;v<=R;v++){ctx.beginPath();ctx.moveTo(...project(0,v));ctx.lineTo(...project(C,v));ctx.stroke()}}}
Promise.all(assets.map(a=>new Promise((resolve,reject)=>{const im=new Image();im.onload=()=>{images[a.name]=im;resolve()};im.onerror=reject;im.src=a.url+'?v='+a.source.sha256.slice(0,12)}))).then(draw).catch(()=>document.querySelector('#summary').textContent='Une image est introuvable. Reconstruire la galerie.');
document.querySelector('#floor').onchange=draw;document.querySelector('#direction').onchange=draw;document.querySelector('#grid').onchange=draw;document.querySelector('#background').onclick=()=>document.body.classList.toggle('light');
// Replace isolated, auto-sized thumbnails with actual ground-plane inspections.
function footprintCards(){for(const card of cards.children){const old=card.querySelector('img');const a=byName[old.alt];const im=new Image();im.onload=()=>{const cv=document.createElement('canvas');cv.width=420;cv.height=420;cv.style.cssText='width:100%;min-width:0;height:300px;object-fit:contain';const c=cv.getContext('2d'),[cols,rows]=a.footprint;const minx=Math.min(-a.anchor[0],-rows*128),maxx=Math.max(a.size[0]-a.anchor[0],cols*128),miny=Math.min(-a.anchor[1],0),maxy=Math.max(a.size[1]-a.anchor[1],(cols+rows)*79.5);const z=Math.min(380/(maxx-minx),380/(maxy-miny));c.translate(210-(minx+maxx)*z/2,210-(miny+maxy)*z/2);c.scale(z,z);const p=(u,v)=>[(u-v)*128,(u+v)*79.5];c.drawImage(im,-a.anchor[0],-a.anchor[1]);c.strokeStyle='#58d7d7';c.lineWidth=1.5/z;c.setLineDash([5/z,4/z]);for(let u=0;u<=cols;u++){c.beginPath();c.moveTo(...p(u,0));c.lineTo(...p(u,rows));c.stroke()}for(let v=0;v<=rows;v++){c.beginPath();c.moveTo(...p(0,v));c.lineTo(...p(cols,v));c.stroke()}c.fillStyle='#f5ce71';c.beginPath();c.arc(0,0,3/z,0,7);c.fill();old.replaceWith(cv)};im.src=a.url+'?v='+a.source.sha256.slice(0,12);}}
const originalGallery=gallery;gallery=()=>{originalGallery();footprintCards()};family.onchange=gallery;gallery();
</script></html>"""
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(
        template.replace("__DATA__", json.dumps(entries, ensure_ascii=False)), encoding="utf-8"
    )
    print(f"{len(entries)} pieces: {OUT}")


if __name__ == "__main__":
    main()
