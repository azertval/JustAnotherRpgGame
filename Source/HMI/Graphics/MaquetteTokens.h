// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "Core/Resources/AssetMarker.h"
#include "HMI/Graphics/MaquettePalette.h"

/**
 * @file HMI/Graphics/MaquetteTokens.h
 * @brief Les **jetons** du rendu de maquette : un rond de couleur, avec sa lettre, à la place
 *        d'une figurine qui n'existe pas (`LOT-128`, décision D2).
 *
 * ## Une image engendrée, pas un rendu de texte
 *
 * Il n'existe **aucun** rendu de texte en scène côté jeu : `hmi::RenderLayer::UI` ne porte que des
 * rectangles, et le `LOT-88` a supprimé les polices pixel. Plutôt que d'en introduire un pour
 * trente-six caractères, le jeton entier — disque et lettre — est **peint en code pur** ici, comme
 * `core::assetMarker` peint le marqueur d'un asset manquant, puis téléversé comme n'importe quelle
 * texture.
 *
 * Le bénéfice est la **parité par construction** : le jeu (QRhi) et l'éditeur (`QPainter`) ne
 * montrent pas deux dessins qui se ressemblent, ils montrent la même image, et il n'y a qu'un
 * chemin à vérifier.
 *
 * ## Adressé comme une planche
 *
 * Un jeton se demande par un **chemin**, `Token/<nature>/<lettre>.png`, exactement comme une pièce
 * de lieu ou une bande de figurine (`hmi::ScenePieceTextures`). Les deux rendus n'ont donc rien de
 * neuf à apprendre : ils voient un chemin de plus dans `hmi::worldTexturePaths`, et savent qu'un
 * chemin de jeton se peint au lieu de se charger.
 */

namespace hmi {

/**
 * @brief Ce qu'un jeton dit de son entité — sa **couleur** (décision D3).
 *
 * La couleur se déduit de ce que le format dit déjà : aucune propriété n'est ajoutée pour elle.
 * `core::MapEntity` n'a pas de notion d'hostilité, et la condition de quête n'existera qu'au
 * `LOT-116` ; le jaune se règle donc, pour l'instant, sur « ce PNJ porte un dialogue ».
 */
enum class MaquetteTokenKind {
    /// Le joueur : l'entrée de la carte, un point d'apparition, une entrée d'arène alliée.
    Player,
    /// Un PNJ qui parle — il porte un dialogue.
    Talker,
    /// Un PNJ muet : il est là, il ne dit rien.
    Neutral,
    /// Ce qui en veut au joueur : une rencontre, une entrée d'arène adverse.
    Hostile,
    /// Ce qui se ramasse ou se lit : un coffre, un panneau.
    Object,
    /// Un portail — la sortie.
    Portal,
};

/// @brief Côté d'un jeton, en pixels d'art de planche (le losange en fait 68, `ScenePieces.h`).
inline constexpr int MAQUETTE_TOKEN_SIZE_PIXELS = 44;

/// @brief La teinte d'une nature de jeton.
[[nodiscard]] MaquetteColor maquetteTokenColor(MaquetteTokenKind kind) noexcept;

/// @brief Le mot que le chemin d'un jeton écrit pour @p kind (`player`, `talker`…).
[[nodiscard]] std::string_view maquetteTokenKindKey(MaquetteTokenKind kind) noexcept;

/**
 * @brief La lettre que porte un jeton, tirée d'un nom.
 *
 * Le **premier caractère alphanumérique** de @p name, en majuscule — `market-mother` donne `M`,
 * comme les pastilles des plans du planning. Un nom sans rien d'utilisable donne `?`.
 */
[[nodiscard]] char maquetteTokenLetter(std::string_view name) noexcept;

/// @brief Le chemin d'un jeton, tel que la composition l'adresse : `Token/<nature>/<lettre>.png`.
[[nodiscard]] std::string maquetteTokenPath(MaquetteTokenKind kind, char letter);

/// @brief Ce qu'un chemin de jeton désigne, ou rien si @p path n'en est pas un.
struct MaquetteTokenRequest {
    MaquetteTokenKind kind = MaquetteTokenKind::Neutral;
    char letter = '?';

    [[nodiscard]] friend bool operator==(const MaquetteTokenRequest&,
                                          const MaquetteTokenRequest&) noexcept = default;
};

/// @brief Lit un chemin de jeton (`Token/hostile/W.png`).
[[nodiscard]] std::optional<MaquetteTokenRequest> parseMaquetteTokenPath(std::string_view path);

/**
 * @brief Peint le jeton de @p request : un disque plein cerné, sa lettre au centre.
 *
 * Déterministe : la même demande donne toujours la même image, sur toute plateforme — c'est ce qui
 * permet au test de comparer le jeu et l'éditeur pixel à pixel.
 * @param request Nature et lettre.
 * @param size    Côté de l'image, en pixels ; 0 ou moins rend une image vide.
 */
[[nodiscard]] core::MarkerImage maquetteTokenImage(const MaquetteTokenRequest& request, int size);

/// @brief Comme ci-dessus, depuis un chemin ; image vide si @p path n'est pas un chemin de jeton.
[[nodiscard]] core::MarkerImage maquetteTokenImage(std::string_view path, int size);

/**
 * @brief Peint @p text avec la **même table de glyphes** que les jetons.
 *
 * `LevelEditor --render` tourne **sans `QApplication`** (`LOT-EDITOR-13`, décision D9) : il n'a
 * donc aucune police à sa disposition, et `QPainter::drawText` y échoue. La légende du plan de
 * principe (`--plan`) écrit par conséquent ses libellés avec les glyphes des jetons — ce qui, au
 * passage, lui donne la typographie des pastilles qu'elle légende.
 *
 * Les lettres sont **mises en majuscules** ; l'espace laisse un blanc, et tout caractère hors de
 * la table donne `?`.
 * @param text  Le libellé.
 * @param scale Pixels par pixel de glyphe (au moins 1).
 * @param color La teinte des lettres.
 * @return L'image du libellé, fond transparent ; vide si @p text l'est.
 */
[[nodiscard]] core::MarkerImage maquetteTextImage(std::string_view text, int scale,
                                                  core::MarkerColor color);

}  // namespace hmi
