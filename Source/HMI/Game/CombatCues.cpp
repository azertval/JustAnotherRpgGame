// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Game/CombatCues.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace hmi {

namespace {

// Le milieu d'une case, en fraction de case.
constexpr float CELL_CENTER = 0.5F;
// Garde de finishAll : nombre maximal de pas d'une seconde avant d'abandonner la file.
constexpr std::size_t MAX_FINISH_STEPS = 4096;

[[nodiscard]] core::Vector2 centerOf(core::GridPosition cell) noexcept {
    return core::Vector2{static_cast<float>(cell.column) + CELL_CENTER,
                         static_cast<float>(cell.row) + CELL_CENTER};
}

// La diagonale qui regarde de `from` vers `to` ; `previous` si les deux se confondent.
[[nodiscard]] FigureFacing facingTowards(core::Vector2 from, core::Vector2 to,
                                         FigureFacing previous) noexcept {
    return figureFacingFor(core::Vector2{to.x - from.x, to.y - from.y}, previous);
}

[[nodiscard]] bool isVictimCue(CombatCueKind kind) noexcept {
    return kind == CombatCueKind::Hit || kind == CombatCueKind::Death;
}

// Le suffixe d'un projectile qui vole vers la gauche de l'ecran : ses bandes sont peintes vers la
// droite, et `build_fx.py` en pose le miroir (`<effet>-left`). En isometrique, l'ecran va vers la
// droite quand la colonne croit plus que la rangee.
[[nodiscard]] std::string projectileStrip(const std::string& effect, core::Vector2 from,
                                          core::Vector2 to) {
    const float versLaDroite = (to.x - from.x) - (to.y - from.y);
    return versLaDroite < 0.0F ? effect + "-left" : effect;
}

// Un pas le long de `path`, parti de `start` depuis `elapsed` secondes ; vrai a l'arrivee.
bool applyWalk(FigureMotion& figure, const std::vector<core::GridPosition>& path,
               core::Vector2 start, float elapsed) {
    const float total = static_cast<float>(path.size()) / CombatCueTrack::WALK_CELLS_PER_SECOND;
    if (elapsed >= total) {
        // Arrive : tourne dans le sens du dernier pas, meme si l'image precedente n'a pas
        // eu le temps de le montrer.
        const core::Vector2 avant = path.size() >= 2 ? centerOf(path[path.size() - 2]) : start;
        figure.point = centerOf(path.back());
        figure.facing = facingTowards(avant, figure.point, figure.facing);
        figure.heading = figureHeadingFor(
            core::Vector2{figure.point.x - avant.x, figure.point.y - avant.y}, figure.heading);
        figure.clip = figure_clips::IDLE;
        figure.clipSeconds = 0.0F;
        return true;
    }
    const float progress = std::max(0.0F, elapsed) * CombatCueTrack::WALK_CELLS_PER_SECOND;
    const auto segment = static_cast<std::size_t>(std::floor(progress));
    const float fraction = progress - static_cast<float>(segment);
    const core::Vector2 from = segment == 0 ? start : centerOf(path[segment - 1]);
    const core::Vector2 to = centerOf(path[std::min(segment, path.size() - 1)]);
    figure.point =
        core::Vector2{from.x + ((to.x - from.x) * fraction), from.y + ((to.y - from.y) * fraction)};
    figure.facing = facingTowards(from, to, figure.facing);
    figure.heading = figureHeadingFor(core::Vector2{to.x - from.x, to.y - from.y}, figure.heading);
    figure.clip = figure_clips::WALK;
    figure.clipSeconds = std::max(0.0F, elapsed);
    return false;
}

// Les durees d'un clip declare : ce qu'il dure, et son image cle -- a defaut le milieu du geste.
[[nodiscard]] GestureTiming gestureFrom(const core::SkeletonClip& clip) noexcept {
    return GestureTiming{
        .seconds = clip.duration,
        .impact = clip.key.value_or(clip.duration * CombatCueTrack::IMPACT_FRACTION)};
}

}  // namespace

FigureTimings CombatCueTrack::stripTimings() noexcept {
    const GestureTiming gesture{.seconds = ACTION_SECONDS,
                                .impact = ACTION_SECONDS * IMPACT_FRACTION};
    return FigureTimings{.attack = gesture,
                         .ranged = gesture,
                         .cast = gesture,
                         .hit = ACTION_SECONDS,
                         .death = ACTION_SECONDS};
}

FigureTimings CombatCueTrack::timingsOf(const core::SkeletonDescription* skeleton) {
    FigureTimings timings = stripTimings();
    if (skeleton == nullptr) {
        return timings;
    }
    if (const core::SkeletonClip* const clip = skeleton->clip(figure_clips::ATTACK)) {
        timings.attack = gestureFrom(*clip);
        timings.ranged = timings.attack;
    }
    if (const core::SkeletonClip* const clip = skeleton->clip(figure_clips::RANGED)) {
        timings.ranged = gestureFrom(*clip);
    }
    if (const core::SkeletonClip* const clip = skeleton->clip(figure_clips::CAST)) {
        timings.cast = gestureFrom(*clip);
    }
    if (const core::SkeletonClip* const clip = skeleton->clip(figure_clips::HIT)) {
        timings.hit = clip->duration;
    }
    if (const core::SkeletonClip* const clip = skeleton->clip(figure_clips::DEATH)) {
        timings.death = clip->duration;
    }
    return timings;
}

void CombatCueTrack::setTimings(core::CombatantId actor, const FigureTimings& timings) {
    _timings[actor] = timings;
}

const FigureTimings& CombatCueTrack::timings(core::CombatantId actor) const {
    static const FigureTimings strips = stripTimings();
    const auto found = _timings.find(actor);
    return found != _timings.end() ? found->second : strips;
}

GestureTiming CombatCueTrack::gestureOf(const CombatCue& cue) const {
    const FigureTimings& known = timings(cue.actor);
    if (cue.kind == CombatCueKind::Cast) {
        return known.cast;
    }
    return cue.ranged ? known.ranged : known.attack;
}

void CombatCueTrack::place(core::CombatantId actor, core::GridPosition cell, FigureFacing facing) {
    FigureMotion& figure = _figures[actor];
    figure.point = centerOf(cell);
    figure.facing = facing;
    figure.heading = figureHeadingOf(facing);
    if (!figure.dead) {
        figure.clip = figure_clips::IDLE;
        figure.clipSeconds = 0.0F;
    }
}

void CombatCueTrack::remove(core::CombatantId actor) {
    _figures.erase(actor);
    _timings.erase(actor);
    std::erase_if(_running, [actor](const Running& r) { return r.cue.actor == actor; });
    std::erase_if(_queue, [actor](const CombatCue& cue) { return cue.actor == actor; });
}

void CombatCueTrack::push(CombatCue cue) {
    // Un fait sur un combattant que personne n'a pose ne se montre pas : il n'a pas de figurine.
    if (!_figures.contains(cue.actor)) {
        return;
    }
    if (cue.kind == CombatCueKind::Walk && cue.path.empty()) {
        return;
    }
    _queue.push_back(std::move(cue));
}

void CombatCueTrack::clear() noexcept {
    _figures.clear();
    _timings.clear();
    _queue.clear();
    _running.clear();
}

std::vector<EffectMotion> CombatCueTrack::effects() const {
    std::vector<EffectMotion> effets;
    for (const Running& running : _running) {
        if (running.cue.kind != CombatCueKind::Effect || running.elapsed < 0.0F ||
            !running.cue.target.has_value()) {
            continue;
        }
        const core::Vector2 cible = centerOf(*running.cue.target);
        if (running.cue.travels) {
            const float t =
                running.flight > 0.0F ? std::min(1.0F, running.elapsed / running.flight) : 1.0F;
            effets.push_back(EffectMotion{
                .effect = projectileStrip(running.cue.effect, running.from, cible),
                .point = core::Vector2{running.from.x + ((cible.x - running.from.x) * t),
                                       running.from.y + ((cible.y - running.from.y) * t)},
                .seconds = running.elapsed});
        } else {
            effets.push_back(EffectMotion{
                .effect = running.cue.effect, .point = cible, .seconds = running.elapsed});
        }
    }
    return effets;
}

const FigureMotion* CombatCueTrack::motionOf(core::CombatantId actor) const {
    const auto found = _figures.find(actor);
    return found != _figures.end() ? &found->second : nullptr;
}

void CombatCueTrack::startNext() {
    if (!_running.empty() || _queue.empty()) {
        return;
    }
    CombatCue next = std::move(_queue.front());
    _queue.pop_front();
    Running running{.cue = std::move(next), .elapsed = 0.0F, .from = {}, .flight = 0.0F};
    if (const FigureMotion* figure = motionOf(running.cue.actor)) {
        running.from = figure->point;
    }
    const bool strike =
        running.cue.kind == CombatCueKind::Attack || running.cue.kind == CombatCueKind::Cast;
    const core::CombatantId attacker = running.cue.actor;
    // L'instant ou le coup porte : l'image cle du geste de l'attaquant (LOT-1005).
    const float impact = strike ? gestureOf(running.cue).impact : ACTION_SECONDS * IMPACT_FRACTION;
    running.flight = impact;
    _running.push_back(std::move(running));
    if (!strike) {
        return;
    }
    // Le coup porte a l'image cle du geste : les touches et les chutes qui le suivent
    // immediatement -- ceux d'AUTRES combattants -- demarrent a cet instant, pas apres le geste
    // entier. Les effets l'accompagnent aussi : un projectile part avec le geste, le reste parait
    // a l'impact.
    while (!_queue.empty()) {
        CombatCue& suivant = _queue.front();
        if (suivant.kind == CombatCueKind::Effect) {
            const float depart = suivant.travels ? 0.0F : -impact;
            Running effet{
                .cue = std::move(suivant), .elapsed = depart, .from = {}, .flight = impact};
            if (const FigureMotion* figure = motionOf(effet.cue.actor)) {
                effet.from = figure->point;
            }
            _running.push_back(std::move(effet));
        } else if (isVictimCue(suivant.kind) && suivant.actor != attacker) {
            _running.push_back(
                Running{.cue = std::move(suivant), .elapsed = -impact, .from = {}, .flight = 0.0F});
        } else {
            break;
        }
        _queue.pop_front();
    }
}

bool CombatCueTrack::apply(Running& running) {
    const auto found = _figures.find(running.cue.actor);
    if (found == _figures.end()) {
        return true;  // sorti entre-temps
    }
    FigureMotion& figure = found->second;
    const float elapsed = running.elapsed;
    switch (running.cue.kind) {
        case CombatCueKind::Walk:
            return applyWalk(figure, running.cue.path, running.from, elapsed);
        case CombatCueKind::Attack:
        case CombatCueKind::Cast: {
            if (running.cue.target.has_value()) {
                const core::Vector2 cible = centerOf(*running.cue.target);
                figure.facing = facingTowards(figure.point, cible, figure.facing);
                figure.heading = figureHeadingFor(
                    core::Vector2{cible.x - figure.point.x, cible.y - figure.point.y},
                    figure.heading);
            }
            if (elapsed >= gestureOf(running.cue).seconds) {
                figure.clip = figure_clips::IDLE;
                figure.clipSeconds = 0.0F;
                return true;
            }
            if (running.cue.kind == CombatCueKind::Cast) {
                figure.clip = figure_clips::CAST;
            } else {
                figure.clip = running.cue.ranged ? figure_clips::RANGED : figure_clips::ATTACK;
            }
            figure.clipSeconds = std::max(0.0F, elapsed);
            return false;
        }
        case CombatCueKind::Hit: {
            if (elapsed < 0.0F) {
                return false;  // le coup n'a pas encore porte
            }
            if (figure.dead) {
                return true;  // un mort n'encaisse plus rien de visible
            }
            if (elapsed >= timings(running.cue.actor).hit) {
                figure.clip = figure_clips::IDLE;
                figure.clipSeconds = 0.0F;
                return true;
            }
            figure.clip = figure_clips::HIT;
            figure.clipSeconds = elapsed;
            return false;
        }
        case CombatCueKind::Effect:
            return elapsed >= (running.cue.travels ? running.flight : EFFECT_SECONDS);
        case CombatCueKind::Death: {
            if (elapsed < 0.0F) {
                return false;
            }
            figure.dead = true;
            figure.clip = figure_clips::DEATH;
            // La bande de mort ne revient jamais au repos : ses secondes continuent de courir, et
            // le rendu la fige sur sa derniere image (`SceneTexture::loop`).
            figure.clipSeconds = elapsed;
            return elapsed >= timings(running.cue.actor).death;
        }
    }
    return true;
}

void CombatCueTrack::advance(float seconds) {
    // Les figurines au repos respirent : leur bande de repos avance avec le temps.
    for (auto& [actor, figure] : _figures) {
        if (figure.clip == figure_clips::IDLE ||
            (figure.dead && figure.clip == figure_clips::DEATH)) {
            figure.clipSeconds += seconds;
        }
    }
    startNext();
    for (Running& running : _running) {
        running.elapsed += seconds;
    }
    std::erase_if(_running, [this](Running& running) { return apply(running); });
    // Un fait fini dans ce pas laisse la place au suivant sans attendre le pas d'apres.
    startNext();
}

void CombatCueTrack::finishAll() {
    // Une garde contre une file qui ne se viderait pas : chaque tour en joue au moins un.
    for (std::size_t guard = 0; guard < MAX_FINISH_STEPS && busy(); ++guard) {
        advance(1.0F);
    }
}

}  // namespace hmi
