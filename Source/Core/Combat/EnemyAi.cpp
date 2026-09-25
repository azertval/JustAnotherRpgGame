// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/EnemyAi.h"

#include <algorithm>
#include <set>
#include <string>
#include <utility>

#include "Core/Combat/BattleGrid.h"
#include "Core/Combat/Flanking.h"
#include "Core/Combat/LineOfSight.h"
#include "Core/Combat/Pathfinding.h"
#include "Core/Data/JsonDocument.h"
#include "Core/Rpg/Bestiary.h"

namespace core {
namespace {

constexpr int SANS_GARDE_DE_VERSION = 0;
/// Une case vaut, au taux `approachPerTile`, un point de degat : 800 unites d'esperance.
constexpr long long UNITES_PAR_POINT = 800;

// --- Donnee -----------------------------------------------------------------------------------

[[nodiscard]] std::string lireTexte(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_string()) ? trouve->get<std::string>()
                                                          : std::string{};
}

[[nodiscard]] int lireEntier(const nlohmann::json& objet, const char* champ, int defaut) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_number_integer()) ? trouve->get<int>() : defaut;
}

[[nodiscard]] bool lireBooleen(const nlohmann::json& objet, const char* champ, bool defaut) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_boolean()) ? trouve->get<bool>() : defaut;
}

// --- Des --------------------------------------------------------------------------------------

/// Les faces d'un d20 qui touchent : un 1 rate toujours, un 20 et le seuil critique touchent.
[[nodiscard]] int facesQuiTouchent(int requis, int seuilCritique) noexcept {
    int faces = 0;
    for (int face = 2; face <= 20; ++face) {
        if (face >= requis || face >= seuilCritique) {
            ++faces;
        }
    }
    return faces;
}

/// Une chance d'un de sur vingt, en quatre-centiemes, selon la posture.
[[nodiscard]] int chanceSelonPosture(int faces, RollStance posture) noexcept {
    switch (posture) {
        case RollStance::Advantage:
            return CHANCE_SCALE - ((20 - faces) * (20 - faces));
        case RollStance::Disadvantage:
            return faces * faces;
        case RollStance::Normal:
            break;
    }
    return faces * 20;
}

/// Les degats moyens en demi-points (un de a f faces vaut (f + 1) / 2), et la part des des seuls.
[[nodiscard]] std::pair<long long, long long> degatsMoyens(const AttackProfile& profil) noexcept {
    long long total = 0;
    long long des = 0;
    for (const DamageClause& clause : profil.damage) {
        const long long moyenneDes =
            static_cast<long long>(clause.dice.count) * (clause.dice.faces + 1);
        total += std::max(0LL, moyenneDes + (2LL * clause.dice.modifier));
        des += moyenneDes;
    }
    return {total, des};
}

// --- Geometrie --------------------------------------------------------------------------------

[[nodiscard]] int ecart(GridPosition a, int coteA, GridPosition b, int coteB) noexcept {
    const int dx =
        std::max({0, b.column - (a.column + coteA - 1), a.column - (b.column + coteB - 1)});
    const int dy = std::max({0, b.row - (a.row + coteA - 1), a.row - (b.row + coteB - 1)});
    return std::max(dx, dy);
}

[[nodiscard]] std::string caseTexte(GridPosition cell) {
    return std::to_string(cell.column) + "," + std::to_string(cell.row);
}

/// Un combattant place, tel que l'IA le lit.
struct Present {
    CombatantId id{};
    const Combatant* combattant = nullptr;
    Footprint emprise;
    const std::vector<AttackProfile>* attaques = nullptr;
};

/// Le bonus d'une portee : l'allonge au contact, la longue portee a distance, 1 sans portee connue.
[[nodiscard]] bool porte(const AttackProfile& attaque, int distance) noexcept {
    if (attaque.kind == AttackKind::Melee) {
        return distance <= attaque.reach;
    }
    return attaque.range.has_value() ? distance <= attaque.range->maximum : distance <= 1;
}

/**
 * Ce que l'IA sait evaluer pour un combattant a un instant : les menaces sur une case, les attaques
 * d'opportunite d'un chemin, les attaques possibles depuis une case. Tout est lu de la session,
 * rien n'est ecrit.
 */
class Evaluateur {
public:
    Evaluateur(const ArenaSession& session, CombatantId acteur, const BehaviorProfile& profil)
        : _session(session), _combat(session.combat()), _grille(_combat.grid()), _profil(profil) {
        _moi.id = acteur;
        _moi.combattant = _combat.find(acteur);
        const std::optional<GridPosition> ancre = _grille.positionOf(acteur);
        if (_moi.combattant == nullptr || !ancre.has_value()) {
            return;
        }
        _moi.emprise = {.anchor = *ancre, .side = footprintSide(_moi.combattant->profile.size)};
        _moi.attaques = session.attacks(acteur);
        _valide = true;
        for (const CombatantId id : _combat.combatants()) {
            const Combatant* c = _combat.find(id);
            const std::optional<GridPosition> position = _grille.positionOf(id);
            if (c == nullptr || !position.has_value()) {
                continue;
            }
            Present present{
                .id = id,
                .combattant = c,
                .emprise = {.anchor = *position, .side = footprintSide(c->profile.size)},
                .attaques = session.attacks(id)};
            _places.push_back(present);
            if (id == acteur || c->status != CombatantStatus::Standing) {
                continue;
            }
            (c->profile.side == _moi.combattant->profile.side ? _allies : _ennemis)
                .push_back(present);
        }
        for (const Present& ennemi : _ennemis) {
            _mobilite.push_back(destinationsDe(ennemi));
        }
    }

    [[nodiscard]] bool valide() const noexcept {
        return _valide;
    }
    [[nodiscard]] const Present& moi() const noexcept {
        return _moi;
    }
    [[nodiscard]] const std::vector<Present>& ennemis() const noexcept {
        return _ennemis;
    }

    [[nodiscard]] Footprint empriseEn(GridPosition ancre) const noexcept {
        return {.anchor = ancre, .side = _moi.emprise.side};
    }

    /// Les corps qui abritent entre deux combattants : tous les places, sauf les deux, l'acteur
    /// compte a sa case supposee.
    [[nodiscard]] std::vector<Footprint> corps(CombatantId a, CombatantId b,
                                               GridPosition ancreActeur) const {
        std::vector<Footprint> liste;
        for (const Present& p : _places) {
            if (p.id == a || p.id == b) {
                continue;
            }
            liste.push_back(p.id == _moi.id ? empriseEn(ancreActeur) : p.emprise);
        }
        return liste;
    }

    /// Le nombre d'ennemis debout qui peuvent frapper l'ancre **au contact** sans bouger. Un tireur
    /// ne compte pas : sa portee couvre l'arene, et la cle anti-suicide n'interdirait plus rien
    /// d'autre que d'approcher ; il pese dans `menace`.
    [[nodiscard]] int menacesImmediates(GridPosition ancre) const {
        int total = 0;
        const Footprint ici = empriseEn(ancre);
        for (const Present& ennemi : _ennemis) {
            if (ennemi.attaques == nullptr) {
                continue;
            }
            const int distance = ecart(ennemi.emprise.anchor, ennemi.emprise.side, ancre, ici.side);
            const bool frappe = std::ranges::any_of(*ennemi.attaques, [&](const AttackProfile& a) {
                return a.kind == AttackKind::Melee && porte(a, distance);
            });
            if (frappe && hasLineOfSight(_grille, ennemi.emprise, ici)) {
                ++total;
            }
        }
        return total;
    }

    /**
     * Les degats attendus au prochain round sur l'ancre, en huit-centiemes : chaque ennemi qui peut
     * y frapper, en marchant puis au contact, ou a distance depuis sa portee plus sa vitesse, avec
     * sa meilleure attaque, contre la CA de l'acteur.
     */
    [[nodiscard]] long long menace(GridPosition ancre, bool esquive) const {
        long long total = 0;
        const Footprint ici = empriseEn(ancre);
        const RollStance posture = esquive ? RollStance::Disadvantage : RollStance::Normal;
        const int ca = _moi.combattant->profile.armorClass;
        for (std::size_t i = 0; i < _ennemis.size(); ++i) {
            const Present& ennemi = _ennemis[i];
            if (ennemi.attaques == nullptr) {
                continue;
            }
            long long pire = 0;
            for (const AttackProfile& attaque : *ennemi.attaques) {
                if (attaque.kind == AttackKind::Melee) {
                    const bool atteint =
                        std::ranges::any_of(_mobilite[i], [&](GridPosition depuis) {
                            return ecart(depuis, ennemi.emprise.side, ancre, ici.side) <=
                                   attaque.reach;
                        });
                    if (atteint) {
                        pire = std::max(pire, expectedDamage(attaque, ca, posture));
                    }
                    continue;
                }
                const int portee = attaque.range.has_value() ? attaque.range->maximum : 1;
                if (ecart(ennemi.emprise.anchor, ennemi.emprise.side, ancre, ici.side) >
                    portee + ennemi.combattant->profile.movement) {
                    continue;
                }
                // Sans abri : un tireur qui marche trouve sa case, et compter l'abri de chaque case
                // contre chaque tireur doublait le cout d'un tour pour une nuance de deux points.
                pire = std::max(pire, expectedDamage(attaque, ca, posture));
            }
            total += pire;
        }
        return total;
    }

    /// Les attaques d'opportunite que provoque le chemin jusqu'a l'ancre, en huit-centiemes.
    [[nodiscard]] long long opportunites(const ReachableArea& zone, GridPosition ancre) const {
        if (ancre == zone.origin()) {
            return 0;
        }
        const std::optional<Path> chemin = zone.pathTo(ancre);
        if (!chemin.has_value()) {
            return 0;
        }
        std::vector<GridPosition> cases{zone.origin()};
        cases.insert(cases.end(), chemin->steps.begin(), chemin->steps.end());
        long long total = 0;
        std::set<CombatantId> deja;
        const int ca = _moi.combattant->profile.armorClass;
        for (std::size_t pas = 0; pas + 1 < cases.size(); ++pas) {
            for (const Present& ennemi : _ennemis) {
                if (deja.contains(ennemi.id) || ennemi.attaques == nullptr ||
                    ennemi.combattant->economy.remaining(REACTION_RESOURCE) <= 0) {
                    continue;
                }
                const auto coup =
                    std::ranges::find(*ennemi.attaques, AttackKind::Melee, &AttackProfile::kind);
                if (coup == ennemi.attaques->end()) {
                    continue;
                }
                const int avant = ecart(cases[pas], _moi.emprise.side, ennemi.emprise.anchor,
                                        ennemi.emprise.side);
                const int apres = ecart(cases[pas + 1], _moi.emprise.side, ennemi.emprise.anchor,
                                        ennemi.emprise.side);
                if (avant <= coup->reach && apres > coup->reach &&
                    hasLineOfSight(_grille, empriseEn(cases[pas]), ennemi.emprise)) {
                    deja.insert(ennemi.id);
                    total += expectedDamage(*coup, ca, RollStance::Normal);
                }
            }
        }
        return total;
    }

    /// Une attaque evaluee depuis une ancre.
    struct Frappe {
        CombatantId cible{};
        std::size_t indice = 0;
        int requis = 0;
        RollStance posture = RollStance::Normal;
        std::vector<std::string> circonstances;
        /// L'esperance, deja ponderee par le profil, en huit-centiemes de point fois cent.
        long long valeur = 0;
    };

    /// Toutes les attaques valides depuis l'ancre, par cible puis par indice d'attaque.
    [[nodiscard]] std::vector<Frappe> frappes(GridPosition ancre) const {
        std::vector<Frappe> liste;
        if (_moi.attaques == nullptr) {
            return liste;
        }
        const Footprint ici = empriseEn(ancre);
        // Un ennemi qui voit l'acteur a une case gene le tir : une fois par ancre, pas par cible.
        std::optional<bool> auContact;
        for (const Present& cible : _ennemis) {
            const int distance = ecart(ancre, ici.side, cible.emprise.anchor, cible.emprise.side);
            const bool uneAPortee = std::ranges::any_of(
                *_moi.attaques, [&](const AttackProfile& a) { return porte(a, distance); });
            if (!uneAPortee || !hasLineOfSight(_grille, ici, cible.emprise)) {
                continue;
            }
            const std::vector<Footprint> abris = corps(_moi.id, cible.id, ancre);
            const Cover abri = coverFrom(_grille, ici, cible.emprise, abris);
            const int ca = cible.combattant->profile.armorClass + coverBonus(abri);
            const long long pourcent = pourcentContre(cible);

            for (std::size_t i = 0; i < _moi.attaques->size(); ++i) {
                const AttackProfile& attaque = (*_moi.attaques)[i];
                if (!porte(attaque, distance)) {
                    continue;
                }
                Frappe frappe{.cible = cible.id, .indice = i, .circonstances = {}};
                const RollStance posture =
                    postureDe(attaque, cible, ancre, distance, auContact, frappe);
                if (abri != Cover::None) {
                    frappe.circonstances.emplace_back(coverLabel(abri));
                }
                frappe.posture = posture;
                frappe.requis = requiredRoll(ca, attackBonusOf(attaque));
                frappe.valeur = expectedDamage(attaque, ca, frappe.posture) * pourcent;
                liste.push_back(std::move(frappe));
            }
        }
        return liste;
    }

    /// Le poids de la menace pour l'acteur : plus lourd s'il est ensanglante.
    [[nodiscard]] long long poidsMenace() const noexcept {
        return isBloodied(_moi.combattant->profile) ? _profil.threatWhenBloodied
                                                    : _profil.threatTaken;
    }

private:
    /// Le pourcentage du profil contre @p cible : les degats, plus une cible ensanglantee, les
    /// allies a son contact, et un allie ensanglante a proteger.
    [[nodiscard]] long long pourcentContre(const Present& cible) const {
        int allieAuContact = 0;
        bool allieEnsanglante = false;
        for (const Present& allie : _allies) {
            if (ecart(allie.emprise.anchor, allie.emprise.side, cible.emprise.anchor,
                      cible.emprise.side) == 1) {
                ++allieAuContact;
                allieEnsanglante = allieEnsanglante || isBloodied(allie.combattant->profile);
            }
        }
        long long pourcent = _profil.damageDealt;
        if (isBloodied(cible.combattant->profile)) {
            pourcent += _profil.bloodiedTarget;
        }
        pourcent += static_cast<long long>(_profil.focusFire) * allieAuContact;
        if (allieEnsanglante) {
            pourcent += _profil.protectBloodiedAlly;
        }
        return pourcent;
    }

    /// Les sources d'avantage et de desavantage d'une attaque depuis l'ancre, notees dans
    /// @p frappe, et la posture qu'elles donnent. @p auContact est calcule au premier tir.
    [[nodiscard]] RollStance postureDe(const AttackProfile& attaque, const Present& cible,
                                       GridPosition ancre, int distance,
                                       std::optional<bool>& auContact, Frappe& frappe) const {
        const Footprint ici = empriseEn(ancre);
        int avantages = 0;
        int desavantages = 0;
        if (attaque.kind == AttackKind::Melee && _session.bout().flanking &&
            isFlankedFrom(_combat, _moi.id, ancre, cible.id)) {
            ++avantages;
            frappe.circonstances.emplace_back("prise en tenaille");
        }
        if (attaque.kind == AttackKind::Ranged) {
            if (attaque.range.has_value() && distance > attaque.range->normal) {
                ++desavantages;
                frappe.circonstances.emplace_back("longue portee");
            }
            if (!auContact.has_value()) {
                auContact = ennemiAuContact(ancre);
            }
            if (*auContact) {
                ++desavantages;
                frappe.circonstances.emplace_back("tir au contact d'un ennemi");
            }
        }
        if (_session.isDodging(cible.id) && hasLineOfSight(_grille, cible.emprise, ici)) {
            ++desavantages;
            frappe.circonstances.emplace_back("esquive de la cible");
        }
        return rollStance(avantages, desavantages);
    }

    /// Vrai si un ennemi qui voit l'acteur se tient a une case de l'ancre.
    [[nodiscard]] bool ennemiAuContact(GridPosition ancre) const {
        const Footprint ici = empriseEn(ancre);
        return std::ranges::any_of(_ennemis, [&](const Present& e) {
            return ecart(ancre, ici.side, e.emprise.anchor, e.emprise.side) == 1 &&
                   hasLineOfSight(_grille, e.emprise, ici);
        });
    }

    /// Les ancres d'ou un ennemi peut frapper au prochain round : la sienne et celles qu'il
    /// atteint.
    [[nodiscard]] std::vector<GridPosition> destinationsDe(const Present& ennemi) const {
        std::vector<GridPosition> liste{ennemi.emprise.anchor};
        const ReachableArea zone(_grille, _combat.moverFor(ennemi.id),
                                 ennemi.combattant->profile.movement);
        const std::vector<GridPosition> autres = zone.destinations();
        liste.insert(liste.end(), autres.begin(), autres.end());
        return liste;
    }

    const ArenaSession& _session;
    const CombatState& _combat;
    const BattleGrid& _grille;
    const BehaviorProfile& _profil;
    bool _valide = false;
    Present _moi;
    std::vector<Present> _places;
    std::vector<Present> _allies;
    std::vector<Present> _ennemis;
    std::vector<std::vector<GridPosition>> _mobilite;
};

/// L'ordre d'un candidat : moins d'exces de menace, puis attaquer, puis avancer vers l'ennemi,
/// puis le score, puis le moins de deplacement. Un candidat ne remplace le meilleur que s'il le
/// bat strictement : a egalite, le premier examine reste. « Avancer » est une cle et non un
/// poids : la menace d'un round entier pese plus que quelques cases d'approche, et une IA qui ne
/// peut pas attaquer resterait hors de portee — ou reculerait — a jamais. Le deplacement departage
/// les cases equivalentes : sans lui, la premiere examinee — le coin haut-gauche — l'emportait.
struct Cle {
    int exces = 0;
    bool attaque = false;
    bool progresse = false;
    long long score = 0;
    int deplacement = 0;

    [[nodiscard]] bool meilleureQue(const Cle& autre) const noexcept {
        if (exces != autre.exces) {
            return exces < autre.exces;
        }
        if (attaque != autre.attaque) {
            return attaque;
        }
        if (progresse != autre.progresse) {
            return progresse;
        }
        if (score != autre.score) {
            return score > autre.score;
        }
        return deplacement < autre.deplacement;
    }
};

[[nodiscard]] std::string nomDe(const CombatState& combat, CombatantId id) {
    const Combatant* c = combat.find(id);
    return (c == nullptr ? std::string("?") : c->profile.name) + " #" +
           std::to_string(static_cast<std::uint32_t>(id));
}

/// Le chemin le plus court jusqu'au contact d'un ennemi : la cible d'une approche.
struct Approche {
    CombatantId cible{};
    Path chemin;
};

[[nodiscard]] std::optional<Approche> approcheLaPlusCourte(const CombatState& combat,
                                                           const Evaluateur& eval) {
    const BattleGrid& grille = combat.grid();
    const Present& moi = eval.moi();
    const Mover mobile = combat.moverFor(moi.id);
    std::optional<Approche> meilleure;
    for (const Present& ennemi : eval.ennemis()) {
        const Footprint& e = ennemi.emprise;
        for (int ligne = e.anchor.row - moi.emprise.side; ligne <= e.anchor.row + e.side; ++ligne) {
            for (int colonne = e.anchor.column - moi.emprise.side;
                 colonne <= e.anchor.column + e.side; ++colonne) {
                const GridPosition ancre{.column = colonne, .row = ligne};
                if (ecart(ancre, moi.emprise.side, e.anchor, e.side) != 1) {
                    continue;
                }
                if (ancre == moi.emprise.anchor) {
                    return Approche{.cible = ennemi.id, .chemin = {}};
                }
                if (!grille.canStand(ancre, moi.emprise.side, moi.id,
                                     moi.combattant->profile.locomotion)) {
                    continue;
                }
                std::optional<Path> chemin = findPath(grille, mobile, ancre);
                if (chemin.has_value() &&
                    (!meilleure.has_value() || chemin->cost < meilleure->chemin.cost)) {
                    meilleure = Approche{.cible = ennemi.id, .chemin = std::move(*chemin)};
                }
            }
        }
    }
    return meilleure;
}

/// Le reste du chemin d'approche depuis une ancre, en cases : exact sur le chemin, estime ailleurs
/// par la distance a la cible plus le detour que le chemin impose.
[[nodiscard]] long long resteDApproche(const CombatState& combat, const Evaluateur& eval,
                                       const Approche& approche, const ReachableArea& zone,
                                       GridPosition ancre) {
    const Present& moi = eval.moi();
    const auto sur = std::ranges::find(approche.chemin.steps, ancre);
    if (sur != approche.chemin.steps.end() || ancre == zone.origin()) {
        const int parcouru = ancre == zone.origin() ? 0 : zone.costTo(ancre).value_or(0);
        return std::max(0, approche.chemin.cost - parcouru);
    }
    const std::optional<GridPosition> ancreCible = combat.grid().positionOf(approche.cible);
    const int cote = footprintSide(combat.find(approche.cible)->profile.size);
    const int direct = ecart(zone.origin(), moi.emprise.side, *ancreCible, cote) - 1;
    const int detour = std::max(0, approche.chemin.cost - direct);
    return std::max(0, ecart(ancre, moi.emprise.side, *ancreCible, cote) - 1) + detour;
}

/// La derniere case du chemin ou l'on peut finir dans @p zone : jusqu'ou une approche va.
[[nodiscard]] std::optional<GridPosition> plusLoinSurLeChemin(const Path& chemin,
                                                              const ReachableArea& zone) {
    for (auto it = chemin.steps.rbegin(); it != chemin.steps.rend(); ++it) {
        if (zone.canEndAt(*it)) {
            return *it;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::string_view nomDePosture(RollStance posture) noexcept {
    switch (posture) {
        case RollStance::Advantage:
            return ", avantage";
        case RollStance::Disadvantage:
            return ", desavantage";
        case RollStance::Normal:
            break;
    }
    return "";
}

}  // namespace

// --- Ce que la table sait ---------------------------------------------------------------------

int attackBonusOf(const AttackProfile& profile) noexcept {
    int bonus = 0;
    for (const Modifier& modificateur : profile.modifiers) {
        bonus += modificateur.value;
    }
    return bonus;
}

int hitChance(int required, RollStance stance, int criticalThreshold) noexcept {
    return chanceSelonPosture(facesQuiTouchent(required, criticalThreshold), stance);
}

int criticalChance(int criticalThreshold, RollStance stance) noexcept {
    const int seuil = std::clamp(criticalThreshold, 2, 20);
    return chanceSelonPosture(21 - seuil, stance);
}

long long expectedDamage(const AttackProfile& profile, int armorClass, RollStance stance) noexcept {
    const int requis = requiredRoll(armorClass, attackBonusOf(profile));
    const long long touche =
        chanceSelonPosture(facesQuiTouchent(requis, profile.criticalThreshold), stance);
    const long long critique = criticalChance(profile.criticalThreshold, stance);
    const auto [moyens, des] = degatsMoyens(profile);
    return (touche * moyens) + (critique * des);
}

// --- Les profils --------------------------------------------------------------------------------

const BehaviorProfile* BehaviorCatalog::find(std::string_view id) const {
    const auto trouve = std::ranges::find(profiles, id, &BehaviorProfile::id);
    return trouve == profiles.end() ? nullptr : &*trouve;
}

BehaviorCatalog loadBehaviors(const std::filesystem::path& file) {
    BehaviorCatalog catalogue;
    const JsonDocument document = readJsonObjectFromFile(file, SANS_GARDE_DE_VERSION);
    if (!document.ok()) {
        catalogue.errors.push_back(document.message);
        return catalogue;
    }
    const std::string fichier = file.filename().string();
    const nlohmann::json& racine = document.root;
    const auto profils = racine.find("profiles");
    if (profils == racine.end() || !profils->is_array()) {
        catalogue.errors.push_back(fichier + " : aucune liste « profiles ».");
        return catalogue;
    }
    const BehaviorProfile defaut;
    for (const nlohmann::json& entree : *profils) {
        if (!entree.is_object()) {
            continue;
        }
        BehaviorProfile profil{
            .id = lireTexte(entree, "id"),
            .name = lireTexte(entree, "name"),
            .damageDealt = lireEntier(entree, "damageDealt", defaut.damageDealt),
            .bloodiedTarget = lireEntier(entree, "bloodiedTarget", defaut.bloodiedTarget),
            .focusFire = lireEntier(entree, "focusFire", defaut.focusFire),
            .protectBloodiedAlly =
                lireEntier(entree, "protectBloodiedAlly", defaut.protectBloodiedAlly),
            .threatTaken = lireEntier(entree, "threatTaken", defaut.threatTaken),
            .threatWhenBloodied =
                lireEntier(entree, "threatWhenBloodied", defaut.threatWhenBloodied),
            .opportunityTaken = lireEntier(entree, "opportunityTaken", defaut.opportunityTaken),
            .approachPerTile = lireEntier(entree, "approachPerTile", defaut.approachPerTile),
            .toleratedThreats = lireEntier(entree, "toleratedThreats", defaut.toleratedThreats),
            .opportunityMaximumRoll =
                lireEntier(entree, "opportunityMaximumRoll", defaut.opportunityMaximumRoll),
            .dodgeWhenThreatened =
                lireBooleen(entree, "dodgeWhenThreatened", defaut.dodgeWhenThreatened),
            .retreatAfterAttack =
                lireBooleen(entree, "retreatAfterAttack", defaut.retreatAfterAttack)};
        if (profil.id.empty()) {
            catalogue.errors.push_back(fichier + " : profil sans identifiant.");
            continue;
        }
        if (catalogue.find(profil.id) != nullptr) {
            catalogue.errors.push_back(fichier + " : profil « " + profil.id + " » en double.");
            continue;
        }
        catalogue.profiles.push_back(std::move(profil));
    }
    catalogue.defaultBehavior = lireTexte(racine, "default");
    if (catalogue.find(catalogue.defaultBehavior) == nullptr) {
        catalogue.errors.push_back(fichier + " : profil par defaut « " + catalogue.defaultBehavior +
                                   " » inconnu.");
    }
    const auto regles = racine.find("assignments");
    if (regles != racine.end() && regles->is_array()) {
        for (const nlohmann::json& entree : *regles) {
            if (!entree.is_object()) {
                continue;
            }
            BehaviorAssignment regle{.behavior = lireTexte(entree, "behavior"),
                                     .trait = lireTexte(entree, "trait"),
                                     .creature = lireTexte(entree, "creature"),
                                     .ranged = lireBooleen(entree, "ranged", false)};
            if (catalogue.find(regle.behavior) == nullptr) {
                catalogue.errors.push_back(fichier + " : attribution vers le profil inconnu « " +
                                           regle.behavior + " ».");
                continue;
            }
            if (regle.trait.empty() && regle.creature.empty() && !regle.ranged) {
                catalogue.errors.push_back(fichier + " : attribution sans condition vers « " +
                                           regle.behavior + " ».");
                continue;
            }
            catalogue.assignments.push_back(std::move(regle));
        }
    }
    return catalogue;
}

std::string behaviorFor(const Creature& creature, const BehaviorCatalog& catalog) {
    const CreatureAttacks attaques = attacksFor(creature);
    long long auContact = 0;
    long long aDistance = 0;
    for (const AttackProfile& attaque : attaques.attacks) {
        const long long moyens = degatsMoyens(attaque).first;
        if (attaque.kind == AttackKind::Melee) {
            auContact = std::max(auContact, moyens);
        } else if (attaque.range.has_value()) {
            aDistance = std::max(aDistance, moyens);
        }
    }
    const bool tireur = aDistance > 0 && aDistance >= auContact;
    for (const BehaviorAssignment& regle : catalog.assignments) {
        const bool trait = regle.trait.empty() ||
                           std::ranges::find(creature.traits, regle.trait, &CreatureTrait::name) !=
                               creature.traits.end();
        const bool laCreature = regle.creature.empty() || regle.creature == creature.id;
        const bool distance = !regle.ranged || tireur;
        if (trait && laCreature && distance) {
            return regle.behavior;
        }
    }
    return catalog.defaultBehavior;
}

// --- Le tour ----------------------------------------------------------------------------------

namespace {

/// Les menaces au-dela de ce que le profil tolere.
[[nodiscard]] int excesDe(const BehaviorProfile& profile, int menaces) noexcept {
    return std::max(0, menaces - profile.toleratedThreats);
}

/// Attaquer @p frappe depuis @p ancre, avec la ligne de journal qui le dit.
[[nodiscard]] TurnPlan planAttaque(const CombatState& combat, const BehaviorProfile& profile,
                                   const Present& moi, GridPosition origine, GridPosition ancre,
                                   const Evaluateur::Frappe& frappe, int menaces, long long cout) {
    TurnPlan candidat{.actor = moi.id,
                      .moveTo = ancre == origine ? std::nullopt : std::optional(ancre),
                      .action = TurnAction::Attack,
                      .target = frappe.cible,
                      .attackIndex = frappe.indice,
                      .dashTo = std::nullopt,
                      .requiredRoll = frappe.requis,
                      .stance = frappe.posture,
                      .immediateThreats = menaces,
                      .score = frappe.valeur - cout,
                      .summary = {}};
    std::string raisons;
    for (const std::string& c : frappe.circonstances) {
        raisons.append(", ").append(c);
    }
    candidat.summary = "ia " + profile.id + " " + nomDe(combat, moi.id) + " : attaque " +
                       nomDe(combat, frappe.cible) + " avec " +
                       (*moi.attaques)[frappe.indice].label + " depuis " + caseTexte(ancre) +
                       " (jet requis " + std::to_string(frappe.requis) +
                       std::string(nomDePosture(frappe.posture)) + raisons + ")";
    return candidat;
}

/// Finir le deplacement en @p ancre sans attaquer, ou tenir sa place si c'est l'origine.
[[nodiscard]] TurnPlan planTenir(const CombatState& combat, const BehaviorProfile& profile,
                                 CombatantId actor, const std::optional<Approche>& approche,
                                 GridPosition origine, GridPosition ancre, int menaces,
                                 long long score) {
    const std::optional<GridPosition> vers = ancre == origine ? std::nullopt : std::optional(ancre);
    const std::string qui = "ia " + profile.id + " " + nomDe(combat, actor);
    const std::string cible =
        approche.has_value() ? " vers " + nomDe(combat, approche->cible) : std::string();

    TurnPlan tenir{.actor = actor,
                   .moveTo = vers,
                   .action = TurnAction::Wait,
                   .target = std::nullopt,
                   .dashTo = std::nullopt,
                   .immediateThreats = menaces,
                   .score = score,
                   .summary = {}};
    tenir.summary = qui + (vers.has_value() ? " : avance en " + caseTexte(ancre) + cible
                                            : std::string(" : tient sa place"));
    return tenir;
}

/// Se precipiter : la vitesse une seconde fois, jusqu'au plus loin du chemin d'approche.
template <typename Proposer>
void proposerCourse(const CombatState& combat, const BehaviorProfile& profile,
                    const Evaluateur& eval, const ReachableArea& zone, const Approche& approche,
                    long long resteOrigine, const Proposer& proposer) {
    if (approche.chemin.steps.empty()) {
        return;
    }
    const Present& moi = eval.moi();
    const CombatantId actor = moi.id;
    const ReachableArea loin(combat.grid(), combat.moverFor(actor),
                             zone.budget() + moi.combattant->profile.movement);
    const std::optional<GridPosition> but = plusLoinSurLeChemin(approche.chemin, loin);
    if (!but.has_value()) {
        return;
    }
    const int menaces = eval.menacesImmediates(*but);
    const long long reste = std::max(0, approche.chemin.cost - loin.costTo(*but).value_or(0));
    TurnPlan course{
        .actor = actor,
        .moveTo = std::nullopt,
        .action = TurnAction::Dash,
        .target = std::nullopt,
        .dashTo = *but,
        .immediateThreats = menaces,
        .score = (-static_cast<long long>(profile.approachPerTile) * UNITES_PAR_POINT * reste) -
                 (eval.poidsMenace() * eval.menace(*but, false)) -
                 (static_cast<long long>(profile.opportunityTaken) * eval.opportunites(loin, *but)),
        .summary = {}};
    course.summary = "ia " + profile.id + " " + nomDe(combat, actor) + " : se precipite en " +
                     caseTexte(*but) + " vers " + nomDe(combat, approche.cible);
    const long long scoreCourse = course.score;
    proposer({.exces = excesDe(profile, menaces),
              .attaque = false,
              .progresse = reste < resteOrigine,
              .score = scoreCourse,
              .deplacement = loin.costTo(*but).value_or(0)},
             std::move(course));
}

/// Reculer apres avoir frappe : vers une case moins menacee, si elle bat strictement la sienne.
/// A menace egale, la moins loin : on ne file pas jusqu'au premier coin de la salle.
void reculerApresAttaque(ArenaSession& session, CombatantId actif, const BehaviorProfile& profil) {
    const CombatState& combat = session.combat();
    const Evaluateur eval(session, actif, profil);
    const std::optional<ReachableArea> zone = combat.reachableArea();
    if (!eval.valide() || !zone.has_value()) {
        return;
    }
    const long long poids = eval.poidsMenace();
    const auto cleEn = [&](GridPosition ancre) {
        return Cle{.exces = std::max(0, eval.menacesImmediates(ancre) - profil.toleratedThreats),
                   .attaque = false,
                   .progresse = false,
                   .score = (-poids * eval.menace(ancre, false)) -
                            (static_cast<long long>(profil.opportunityTaken) *
                             eval.opportunites(*zone, ancre)),
                   .deplacement = zone->costTo(ancre).value_or(0)};
    };
    Cle meilleure = cleEn(zone->origin());
    std::optional<GridPosition> recul;
    for (const GridPosition ancre : zone->destinations()) {
        const Cle cle = cleEn(ancre);
        if (cle.meilleureQue(meilleure)) {
            meilleure = cle;
            recul = ancre;
        }
    }
    if (recul.has_value()) {
        session.note("ia " + profil.id + " " + nomDe(combat, actif) + " : recule en " +
                     caseTexte(*recul));
        static_cast<void>(session.move(*recul));
    }
}

}  // namespace

TurnPlan planTurn(const ArenaSession& session, CombatantId actor, const BehaviorProfile& profile) {
    const CombatState& combat = session.combat();
    TurnPlan plan{.actor = actor,
                  .moveTo = std::nullopt,
                  .target = std::nullopt,
                  .dashTo = std::nullopt,
                  .summary = {}};
    const Evaluateur eval(session, actor, profile);
    const std::optional<ReachableArea> zone = combat.reachableArea();
    if (!eval.valide() || !zone.has_value() || combat.activeCombatant() != actor) {
        plan.summary = "ia " + profile.id + " " + nomDe(combat, actor) + " : attend";
        return plan;
    }
    const Present& moi = eval.moi();
    const bool action = moi.combattant->economy.remaining(ACTION_RESOURCE) > 0;
    const long long poidsMenace = eval.poidsMenace();

    std::vector<GridPosition> ancres{zone->origin()};
    const std::vector<GridPosition> destinations = zone->destinations();
    ancres.insert(ancres.end(), destinations.begin(), destinations.end());

    std::optional<Cle> meilleure;
    const auto proposer = [&](const Cle& cle, TurnPlan candidat) {
        if (!meilleure.has_value() || cle.meilleureQue(*meilleure)) {
            meilleure = cle;
            plan = std::move(candidat);
        }
    };
    const auto exces = [&](int menaces) { return excesDe(profile, menaces); };

    // Ce qu'une case coute, calcule une fois : les deux familles de candidats le relisent.
    struct Case {
        int menaces = 0;
        long long menace = 0;
        long long opportunites = 0;
    };
    std::vector<Case> cases;
    cases.reserve(ancres.size());
    for (const GridPosition ancre : ancres) {
        cases.push_back({.menaces = eval.menacesImmediates(ancre),
                         .menace = poidsMenace * eval.menace(ancre, false),
                         .opportunites = static_cast<long long>(profile.opportunityTaken) *
                                         eval.opportunites(*zone, ancre)});
    }

    const auto deplacementVers = [&](GridPosition ancre) {
        return zone->costTo(ancre).value_or(0);
    };

    // Attaquer : chaque case, chaque cible, chaque attaque.
    for (std::size_t indice = 0; action && indice < ancres.size(); ++indice) {
        const GridPosition ancre = ancres[indice];
        const std::vector<Evaluateur::Frappe> frappes = eval.frappes(ancre);
        if (frappes.empty()) {
            continue;
        }
        const int menaces = cases[indice].menaces;
        const long long cout = cases[indice].menace + cases[indice].opportunites;
        for (const Evaluateur::Frappe& frappe : frappes) {
            TurnPlan candidat =
                planAttaque(combat, profile, moi, zone->origin(), ancre, frappe, menaces, cout);
            const long long scoreCandidat = candidat.score;
            proposer({.exces = exces(menaces),
                      .attaque = true,
                      .progresse = true,
                      .score = scoreCandidat,
                      .deplacement = deplacementVers(ancre)},
                     std::move(candidat));
        }
    }

    // Sans attaque : s'approcher, se precipiter, esquiver, ou tenir. Un candidat « progresse »
    // s'il laisse moins de chemin jusqu'a l'ennemi que la case de depart.
    const std::optional<Approche> approche = approcheLaPlusCourte(combat, eval);
    const long long resteOrigine = approche.has_value() ? approche->chemin.cost : 0;
    for (std::size_t indice = 0; indice < ancres.size(); ++indice) {
        const GridPosition ancre = ancres[indice];
        const int menaces = cases[indice].menaces;
        const long long reste =
            approche.has_value() ? resteDApproche(combat, eval, *approche, *zone, ancre) : 0;
        const bool progresse = reste < resteOrigine;
        const int deplacement = deplacementVers(ancre);
        const long long opportunites = cases[indice].opportunites;
        const long long base =
            -static_cast<long long>(profile.approachPerTile) * UNITES_PAR_POINT * reste;

        const TurnPlan tenir = planTenir(combat, profile, actor, approche, zone->origin(), ancre,
                                         menaces, base - cases[indice].menace - opportunites);
        proposer({.exces = exces(menaces),
                  .attaque = false,
                  .progresse = progresse,
                  .score = tenir.score,
                  .deplacement = deplacement},
                 tenir);

        if (action && profile.dodgeWhenThreatened && menaces > 0) {
            TurnPlan esquive = tenir;
            esquive.action = TurnAction::Dodge;
            esquive.score = base - (poidsMenace * eval.menace(ancre, true)) - opportunites;
            esquive.summary += ", esquive";
            const long long scoreEsquive = esquive.score;
            proposer({.exces = exces(menaces),
                      .attaque = false,
                      .progresse = progresse,
                      .score = scoreEsquive,
                      .deplacement = deplacement},
                     std::move(esquive));
        }
        if (action && opportunites > 0) {
            TurnPlan desengage = tenir;
            desengage.action = TurnAction::Disengage;
            desengage.score = base - cases[indice].menace;
            desengage.summary += ", en se desengageant";
            const long long scoreDesengage = desengage.score;
            proposer({.exces = exces(menaces),
                      .attaque = false,
                      .progresse = progresse,
                      .score = scoreDesengage,
                      .deplacement = deplacement},
                     std::move(desengage));
        }
    }

    if (action && approche.has_value()) {
        proposerCourse(combat, profile, eval, *zone, *approche, resteOrigine, proposer);
    }
    return plan;
}

bool shouldTakeOpportunity(const ArenaSession& session, CombatantId reactor, CombatantId mover,
                           const BehaviorProfile& profile) {
    const std::vector<AttackProfile>* attaques = session.attacks(reactor);
    const Combatant* fuyard = session.combat().find(mover);
    if (attaques == nullptr || fuyard == nullptr) {
        return false;
    }
    const auto coup = std::ranges::find(*attaques, AttackKind::Melee, &AttackProfile::kind);
    return coup != attaques->end() &&
           requiredRoll(fuyard->profile.armorClass, attackBonusOf(*coup)) <=
               profile.opportunityMaximumRoll;
}

OpportunityPolicy aiOpportunityPolicy(const BehaviorCatalog& catalog) {
    return [&catalog](const ArenaSession& session, CombatantId reactor, CombatantId mover) {
        const std::string& id = session.behaviorOf(reactor);
        if (id.empty()) {
            return true;
        }
        const BehaviorProfile* profil = catalog.find(id);
        return profil == nullptr || shouldTakeOpportunity(session, reactor, mover, *profil);
    };
}

bool playTurn(ArenaSession& session, const BehaviorCatalog& catalog) {
    CombatState& combat = session.combat();
    const std::optional<CombatantId> actif = combat.activeCombatant();
    if (!actif.has_value() || combat.phase() != CombatPhase::TurnActive) {
        return false;
    }
    const BehaviorProfile* profil = catalog.find(session.behaviorOf(*actif));
    if (profil == nullptr) {
        return false;
    }
    const auto toujoursLui = [&] {
        return combat.phase() == CombatPhase::TurnActive && combat.activeCombatant() == actif;
    };
    const TurnPlan plan = planTurn(session, *actif, *profil);
    session.note(plan.summary);

    if (plan.action == TurnAction::Disengage) {
        static_cast<void>(session.disengage());
    }
    if (plan.moveTo.has_value() && toujoursLui()) {
        // Un deplacement refuse se dit : sans cette ligne, une IA qui « ne bouge pas » ne laisse
        // aucune trace de ce qu'elle a voulu, et l'audit ne peut pas distinguer un plan qui tient
        // sa place d'un pas que la grille a refuse (LOT-118).
        if (const MoveOutcome pas = session.move(*plan.moveTo); pas.result != MoveResult::Moved) {
            session.note("deplacement refuse vers " + std::to_string(plan.moveTo->column) + "," +
                         std::to_string(plan.moveTo->row));
        }
    }
    switch (plan.action) {
        case TurnAction::Attack:
            if (toujoursLui() && plan.target.has_value()) {
                static_cast<void>(session.attack(*plan.target, plan.attackIndex));
            }
            break;
        case TurnAction::Dash:
            if (toujoursLui() && session.dash() && plan.dashTo.has_value()) {
                static_cast<void>(session.move(*plan.dashTo));
            }
            break;
        case TurnAction::Dodge:
            if (toujoursLui()) {
                static_cast<void>(session.dodge());
            }
            break;
        case TurnAction::Disengage:
        case TurnAction::Wait:
            break;
    }

    if (plan.action == TurnAction::Attack && profil->retreatAfterAttack && toujoursLui()) {
        reculerApresAttaque(session, *actif, *profil);
    }
    if (toujoursLui()) {
        static_cast<void>(session.endTurn());
    }
    return true;
}

}  // namespace core
