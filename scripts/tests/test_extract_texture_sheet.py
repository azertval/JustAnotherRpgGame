# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""L'atelier des textures (LOT-92, T3) : la disposition se valide sans image, et une planche
synthétique -- chaque emprise peinte à plat, sur deux planches -- se découpe, s'installe et se revérifie.

Les tests d'image demandent Pillow et numpy, que la CI n'installe pas : ils sont sautés là, et joués
sur le poste qui produit les planches.
"""
import copy
import json

import pytest

import extract_texture_sheet as T


@pytest.fixture
def colisee():
    return T.charger('colisee')


def test_les_dispositions_de_l_atelier_sont_conformes():
    nombre, fautes = T.valider_tout()
    assert fautes == []
    assert nombre >= 36


def test_une_grille_se_deduit_sans_chevauchement(colisee):
    poses = T.grille(colisee)
    assert len(poses) == len(T.dessinees(colisee)) < len(colisee['cells'])
    largeur, hauteur = T.taille_planche(colisee)
    for numero in range(1, T.planches(colisee) + 1):
        boites = [p['boite'] for p in poses if p['planche'] == numero]
        assert boites, f'planche {numero} vide'
        for i, (ax0, ay0, ax1, ay1) in enumerate(boites):
            assert 0 <= ax0 < ax1 <= largeur and 0 <= ay0 < ay1 <= hauteur
            for bx0, by0, bx1, by1 in boites[i + 1:]:
                assert ax1 <= bx0 or bx1 <= ax0 or ay1 <= by0 or by1 <= ay0


def test_les_cellules_debordent_sur_une_planche_de_plus(colisee):
    colisee['sheet'] = {'size': [2560, 1440], 'scale': 4}
    poses = T.grille(colisee)
    assert T.planches(colisee) == 2
    assert [p['planche'] for p in poses] == sorted(p['planche'] for p in poses)
    colisee['sheet'] = {'size': [3840, 2160], 'scale': 4}
    assert T.planches(colisee) == 1


def test_un_sol_est_le_losange_d_iso_projection(colisee):
    sol = next(c for c in colisee['cells'] if c['class'] == 'floor')
    assert T.canevas(colisee, sol) == (68, 42)
    assert T.sommets(colisee, sol) == [(34, 0), (68, 21), (34, 42), (0, 21)]


def test_une_grande_piece_a_gauche_s_allonge_sur_l_arete_haut_gauche(colisee):
    porte = next(c for c in colisee['cells'] if c['name'] == 'gate-left')
    haut, droite, bas, gauche = T.sommets(colisee, porte)
    assert (haut[0] - gauche[0], gauche[1] - haut[1]) == (68, 42)   # deux cases vers la gauche
    assert (droite[0] - haut[0], droite[1] - haut[1]) == (34, 21)   # une vers la droite


@pytest.mark.parametrize('faute, attendu', [
    (lambda d: d['cells'].append(copy.deepcopy(d['cells'][0])), 'deux fois'),
    (lambda d: d.update(keyPrefix='ui/coliseum'), 'scene/'),
    (lambda d: d.update(location='lieu-inexistant'), "absent de l'atlas"),
    (lambda d: d['cells'][0].update({'class': 'inconnue'}), 'inconnue'),
    (lambda d: d.update(locationExcerpt=['A market with lantern-lit stalls.']), 'extrait absent'),
    (lambda d: d.update(sheet={'size': [2560, 1450], 'scale': 4}), 'multiples de 16'),
    (lambda d: d.update(sheet={'size': [4096, 2048], 'scale': 4}), 'au plus 3840'),
    (lambda d: d.update(sheet={'size': [3200, 800], 'scale': 2}), 'rapport'),
    (lambda d: d.update(sheet={'size': [768, 512], 'scale': 2}), 'pixels'),
    (lambda d: d.update(sheet={'size': [2560, 1440], 'scale': 12}), 'ne tient pas'),
])
def test_une_disposition_fautive_est_refusee(colisee, faute, attendu):
    faute(colisee)
    assert any(attendu in f for f in T.valider(colisee))


def test_le_bloc_c_nomme_chaque_cellule_de_sa_planche_dans_l_ordre(colisee):
    for numero in range(1, T.planches(colisee) + 1):
        texte = T.bloc_c(colisee, numero)
        cellules = [p['cellule'] for p in T.grille(colisee) if p['planche'] == numero]
        for rang, cellule in enumerate(cellules, 1):
            assert f"{rang}. {cellule['prompt']}" in texte
        assert f"{rang + 1}. " not in texte


def test_le_bloc_b_ne_cite_que_les_extraits(colisee):
    texte = T.bloc_b(colisee)
    assert 'Arena of Fate' in texte
    assert 'Golden Chalice Casino' not in texte
    del colisee['locationExcerpt']
    assert 'Golden Chalice Casino' in T.bloc_b(colisee)


def test_le_bloc_a_prend_le_pas_de_la_disposition(colisee):
    colisee['sheet'] = {'size': [2560, 1440], 'scale': 4}
    texte = T.bloc_a(colisee)
    assert '4 screen pixels per art pixel' in texte and '272 screen pixels wide and 168' in texte
    assert '{' not in texte and not any(l.startswith('#') for l in texte.splitlines())


# -- Avec images ---------------------------------------------------------------------------------

def _planches(disposition, trou=None):
    np = pytest.importorskip('numpy')
    Image = pytest.importorskip('PIL.Image')
    ImageDraw = pytest.importorskip('PIL.ImageDraw')
    s = T.pas(disposition)
    images = [Image.new('RGBA', T.taille_planche(disposition), (0, 0, 0, 0))
              for _ in range(T.planches(disposition))]
    for rang, pose in enumerate(T.grille(disposition)):
        cellule = pose['cellule']
        trait = ImageDraw.Draw(images[pose['planche'] - 1])
        ox, oy = pose['origine']
        couleur = (40 + 5 * rang, 120, 200 - 4 * rang, 255)
        points = [(ox + x * s, oy + y * s) for x, y in T.sommets(disposition, cellule)]
        trait.polygon(points, fill=couleur)
        hausse = disposition['classes'][cellule['class']]['rise']
        if hausse:
            w, _ = T.canevas(disposition, cellule)
            trait.rectangle([ox + 10 * s, oy, ox + (w - 10) * s, oy + hausse * s], fill=couleur)
        if cellule['name'] == trou:
            x, y = points[0]
            trait.rectangle([x - 30 * s, y, x + 30 * s, y + 30 * s], fill=(0, 0, 0, 0))
    return np, Image, images


@pytest.fixture
def depot(tmp_path, monkeypatch, colisee):
    monkeypatch.setattr(T, 'RACINE', tmp_path)
    return tmp_path


def test_des_planches_se_decoupent_s_installent_et_se_reverifient(depot, colisee, tmp_path):
    np, Image, planches = _planches(colisee)
    candidats = []
    for numero, planche in enumerate(planches, 1):
        candidats.append(tmp_path / f'candidat-{numero}.png')
        planche.save(candidats[-1])
    assert len(candidats) == 2
    assert T.decoupe('colisee', candidats) == 0

    racine = depot / colisee['installRoot']
    manifeste = json.loads((racine / 'manifest.json').read_text(encoding='utf-8'))
    assert len(manifeste['textures']) == len(colisee['cells'])
    assert [f['file'] for f in manifeste['sheets']] == ['planche-1.png', 'planche-2.png']
    mur = np.asarray(Image.open(racine / 'wall-left.png'))
    assert np.array_equal(np.asarray(Image.open(racine / 'wall-right.png')), mur[:, ::-1])
    assert manifeste['textures']['scene/coliseum/wall-right']['mirrorOf'] == 'scene/coliseum/wall-left'
    porte = manifeste['textures']['scene/coliseum/gate-right']
    assert porte['footprint'] == [2, 1]
    assert porte['anchor'][0] == porte['size'][0] - manifeste['textures']['scene/coliseum/gate-left']['anchor'][0]
    sable = manifeste['textures']['scene/coliseum/sand']
    assert (sable['size'], sable['anchor'], sable['sheet']) == ([68, 42], [34, 0], 1)
    assert Image.open(racine / 'sand.png').size == (68, 42)
    assert T.verifier('colisee') == 0

    # une texture retouchée à la main se voit
    retouche = np.asarray(Image.open(racine / 'pillar.png')).copy()
    retouche[50, 30] = [255, 0, 255, 255]
    Image.fromarray(retouche).save(racine / 'pillar.png')
    assert T.verifier('colisee') == 1


def test_un_sol_troue_est_refuse(colisee):
    _, _, planches = _planches(colisee, trou='sand-blood')
    recues = [T.mettre_au_format(colisee, p) for p in planches]
    _, _, erreurs, _ = T.decouper(colisee, recues)
    assert any(e.startswith('scene/coliseum/sand-blood : le sol couvre') for e in erreurs)


def test_il_faut_toutes_les_planches(colisee):
    _, _, planches = _planches(colisee)
    with pytest.raises(T.DispositionError, match='1 planche'):
        T.decouper(colisee, [T.mettre_au_format(colisee, planches[0])])


def test_une_planche_sans_transparence_est_refusee(colisee):
    _, _, planches = _planches(colisee)
    with pytest.raises(T.DispositionError, match='fond transparent'):
        T.mettre_au_format(colisee, planches[0].convert('RGB'))


def test_les_pieces_se_lisent_hors_des_cellules_dans_l_ordre(colisee):
    """Ce que le générateur a rendu au tour 1 : ni la place ni l'échelle du gabarit, mais le nombre
    et l'ordre. Chaque pièce est ici décalée et agrandie de moitié ; deux étincelles flottent."""
    np, Image, planches = _planches(colisee)
    s = T.pas(colisee)
    deformees = []
    for numero, planche in enumerate(planches, 1):
        poses = [p for p in T.grille(colisee) if p['planche'] == numero]
        largeur, hauteur = planche.size
        sortie = Image.new('RGBA', (largeur * 2, hauteur * 2), (0, 0, 0, 0))
        for rang, pose in enumerate(poses):
            x0, y0, x1, y1 = pose['boite']
            piece = planche.crop(pose['boite'])
            piece = piece.resize((piece.width * 3 // 2, piece.height * 3 // 2), Image.NEAREST)
            decalage = (rang % 3) * 7 * s
            sortie.alpha_composite(piece, (x0 * 3 // 2 + decalage, y0 * 3 // 2 + decalage // 2))
        # une étincelle au loin, à effacer ; une autre au ras de la première pièce, à rattacher
        sortie.alpha_composite(Image.new('RGBA', (3, 3), (255, 200, 0, 255)), (largeur * 2 - 20, hauteur * 2 - 20))
        x0, y0, x1, _ = poses[-1]['boite']
        sortie.alpha_composite(Image.new('RGBA', (3, 3), (255, 200, 0, 255)),
                               (x1 * 3 // 2 - 40, y0 * 3 // 2 - 6))
        deformees.append(sortie)
    textures, manifeste, erreurs, _ = T.decouper(
        colisee, [T.mettre_au_format(colisee, p) for p in deformees])
    assert erreurs == []
    assert set(manifeste['textures']) == {T.cle(colisee, c) for c in colisee['cells']}
    assert textures['scene/coliseum/sand'].size == (68, 42)
    assert manifeste['textures']['scene/coliseum/sand']['scale'] == pytest.approx(1 / (1.5 * s), rel=0.05)


def test_l_arete_d_une_piece_orientee_est_rouge_dans_le_gabarit(colisee):
    pytest.importorskip('numpy')
    pytest.importorskip('PIL.Image')
    mur = next(p for p in T.grille(colisee) if p['cellule']['name'] == 'wall-left')
    image = T.gabarit(colisee, mur['planche'])
    s = T.pas(colisee)
    ox, oy = mur['origine']
    (hx, hy), _, _, (gx, gy) = [(ox + x * s, oy + y * s) for x, y in T.sommets(colisee, mur['cellule'])]
    assert image.getpixel(((hx + gx) // 2, (hy + gy) // 2))[:3] == (220, 40, 40)


def test_un_miroir_doit_nommer_une_cellule_dessinee(colisee):
    miroir = next(c for c in colisee['cells'] if 'mirrorOf' in c)
    miroir['mirrorOf'] = 'inexistante'
    assert any('mirrorOf' in f for f in T.valider(colisee))


@pytest.mark.parametrize('arete, stance, dessin, retournee', [
    ('left', 'along', 'bas-gauche', False),
    ('left', 'along', 'bas-droite', True),
    ('right', 'along', 'bas-gauche', True),
    ('left', 'toward', 'haut-droite', True),
    ('left', 'toward', 'haut-gauche', False),
])
def test_une_piece_dessinee_contre_l_autre_arete_est_retournee(colisee, arete, stance, dessin, retournee):
    np = pytest.importorskip('numpy')
    cellule = {'name': 'essai', 'class': 'tall', 'edge': arete, 'stance': stance, 'prompt': 'x'}
    texture = np.zeros((100, 68, 4), dtype=np.uint8)
    if dessin.startswith('bas'):
        # un mur en biais : sa base descend vers le côté nommé
        for x in range(68):
            bas = 99 - (x * 20 // 68 if dessin == 'bas-gauche' else (67 - x) * 20 // 68)
            texture[bas - 60:bas, x] = 255
    else:
        # des gradins : la moitié haute penche vers le côté nommé
        texture[50:100, :] = 255
        texture[0:50, :34] = 255 if dessin == 'haut-gauche' else 0
        texture[0:50, 34:] = 255 if dessin == 'haut-droite' else 0
    avertissements = []
    sortie = T._orienter(colisee, cellule, texture, avertissements)
    assert np.array_equal(sortie, texture[:, ::-1]) == retournee
    assert bool(avertissements) == retournee


def test_martpart_se_commande_depuis_sa_seule_fiche():
    brut = json.loads((T.DISPOSITIONS / 'martpart.json').read_text(encoding='utf-8'))
    assert set(brut) == {'version', 'id', 'location', 'model'}   # rien de rédigé pour ce lieu
    martpart = T.charger('martpart')
    assert T.valider(martpart) == []
    assert martpart['title'] == 'Martpart'
    assert martpart['keyPrefix'] == 'scene/martpart'
    assert martpart['installRoot'] == 'Source/Elements/Assets/Scene/martpart'
    fiche = json.loads((T.LIEUX / f"{martpart['location']}.json").read_text(encoding='utf-8'))
    assert fiche['description'] in T.bloc_b(martpart)
    assert T.planches(martpart) == 1


def test_un_modele_inconnu_est_refuse():
    with pytest.raises(T.DispositionError, match='introuvable'):
        T.resoudre({'id': 'x', 'location': 'central-empire-the-capital-city-martpart', 'model': 'absent'})


def test_un_champ_du_fichier_l_emporte_sur_le_modele():
    disposition = T.resoudre({'id': 'x', 'model': 'quartier', 'keyPrefix': 'scene/autre',
                              'sheet': {'size': [3840, 2160], 'scale': 4}})
    assert disposition['keyPrefix'] == 'scene/autre'
    assert disposition['sheet']['size'] == [3840, 2160]


def test_une_piece_libre_plus_large_que_son_emprise_elargit_son_canevas():
    np = pytest.importorskip('numpy')
    pytest.importorskip('PIL')
    martpart = T.charger('martpart')
    etal = next(c for c in martpart['cells'] if c['name'] == 'feature-1')
    cw, ch = T.canevas(martpart, etal)
    art = np.full((ch, cw + 70, 4), 255, dtype=np.uint8)
    avertissements = []
    texture, ancre = T._poser(martpart, etal, art, avertissements)
    assert texture.shape[1] == cw + 70 and texture[..., 3].all()      # rien n'est rogné
    gauche = (cw + 70) // 2 - cw // 2                                # centré sur l'emprise
    assert ancre[0] == T.emprise(martpart, etal)[1] * T.DEMI_L + gauche
    assert any('canevas élargi (35 à gauche, 35 à droite)' in a for a in avertissements)
    mur = next(c for c in martpart['cells'] if c['name'] == 'wall-left')
    cw, ch = T.canevas(martpart, mur)
    texture, _ = T._poser(martpart, mur, np.full((ch, cw + 5, 4), 255, dtype=np.uint8), [])
    assert texture.shape[1] == cw                                     # une pièce pleine, rognée
