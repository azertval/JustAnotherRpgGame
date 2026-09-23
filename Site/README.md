# La charte du site publié

Le site publié sur `gh-pages` a quatre parties, engendrées par trois outils différents :

| Adresse | Ce qu'elle porte | Qui l'engendre |
|---|---|---|
| `/` | Les pages : accueil, guide, spécifications, cahier de test | [`Documentation/outils/build_docs_site.py`](../Documentation/outils/build_docs_site.py) |
| `/reference/` | La référence de code, annexe du guide | Doxygen, par [`scripts/docs/build_docs.py`](../scripts/docs/build_docs.py) |
| `/planning/` | La planification : versions, lots, référentiels | [`Planning/outils/build_planning_site.py`](../Planning/outils/build_planning_site.py) |
| `/qualite/` | Couverture de code et mesures de performance | [`scripts/docs/build_quality_site.py`](../scripts/docs/build_quality_site.py) |

Quatre parties, mais **un seul site** : ce dossier tient ce qu'ils ont en commun — la palette, les
fontes et la barre d'en-tête. Aucun des trois ne choisit une couleur ; chacun emprunte un jeton.
C'est la seule règle, et elle se vérifie à l'œil : une couleur écrite ailleurs qu'ici est un écart.

## Les fichiers

| Fichier | Ce qu'il fait | Qui le charge |
|---|---|---|
| `tokens.css` | Les jetons : couleurs (clair et sombre), fontes, ombre. Ne décrit aucun élément. | les trois |
| `topbar.css` | La barre d'en-tête commune : marque, sections, les trois parties du site. | les trois |
| `theme.css` | L'habillage des pages écrites ici (titres, tables, cartouches, badges, graphe, figures, encadrés, cartes de cas de test). Importe les deux précédents. | documentation, planification, qualité |
| `reference.css` | Le branchement des ~140 variables CSS de Doxygen sur les jetons. | référence de code |
| `header.html` | Le gabarit d'en-tête de Doxygen, qui porte la barre. | référence de code |
| `site.js` | Les filtres des tables (recherche, listes déroulantes). | documentation, planification |
| `docs.js` | La recherche de la barre d'en-tête et le filtre des cas de test. | documentation |

La référence de code ne charge **pas** `theme.css` : ses pages ont leur propre structure, que des
règles d'élément (`table`, `h2`, `a`) défigureraient. Elle prend les mêmes jetons et la même barre,
et le reste passe par ses variables. C'est là la limite du partage — les jetons et la barre sont
communs, les habillages non.

## La palette

Tirée de la planche de référence d'Arenarea : ivoire, sable, bronze foncé, bourgogne, or vieilli,
vert feuillage, eau sourde. Le thème sombre suit la préférence du système, et n'est écrit qu'une
fois, dans `tokens.css` : nulle part le visiteur n'a de bouton à chercher, et nulle part on n'a
deux endroits à relire.

## Voir le résultat sans publier

```
python scripts/docs/build_docs.py                                          # la référence de code
python Documentation/outils/build_docs_site.py --out build/site \
    --tagfile Documentation/generated/html/reference.tag              # les pages (remplace build/site)
python Planning/outils/build_planning_site.py --out build/site/planning --docs-url ../
python scripts/docs/build_quality_site.py --site build/site               # la page qualité
```

La référence se pose ensuite sous `build/site/reference/` (copie de `Documentation/generated/html/`).

Chacun écrit dans son dossier ; seuls les liens d'une partie à l'autre (la barre
d'en-tête) demandent que les parties soient assemblées, ce que fait
[`docs.yml`](../.github/workflows/docs.yml) à la publication.

## Monter Doxygen de version

`header.html` est le gabarit livré par `doxygen -w html`, augmenté de la seule barre. La version
est épinglée (`ci.yml`, `docs.yml`) : à la prochaine montée, regénérer le gabarit et y réinsérer la
barre, plutôt que de le rafistoler. `reference.css` tient de son côté la liste des variables de
Doxygen ; un nom qui disparaît d'une version à l'autre ne casse rien, il ne sert plus.
