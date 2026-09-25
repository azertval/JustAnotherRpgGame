// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Game/CombatCues.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace hmi {

namespace {

[[nodiscard]] core::Vector2 centerOf(core::GridPosition cell) noexcept {
    return core::Vector2{static_cast<float>(cell.column) + 0.5F,
                         static_cast<float>(cell.row) + 0.5F};
}

/// La diagonale qui regarde de @p from vers @p to ; @p previous si les deux se confondent.
[[nodiscard]] FigureFacing facingTowards(core::Vector2 from, core::Vector2 to,
                                         FigureFacing previous) noexcept {
    return figureFacingFor(core::Vector2{to.x - from.x, to.y - from.y}, previous);
}

[[nodiscard]] bool isVictimCue(CombatCueKind kind) noexcept {
    return kind == CombatCueKind::Hit || kind == CombatCueKind::Death;
}

}  // namespace

void CombatCueTrack::place(core::CombatantId actor, core::GridPosition cell, FigureFacing facing) {
    FigureMotion& figure = _figures[actor];
    figure.point = centerOf(cell);
    figure.facing = facing;
    if (!figure.dead) {
        figure.clip = figure_clips::IDLE;
        figure.clipSeconds = 0.0F;
    }
}

void CombatCueTrack::remove(core::CombatantId actor) {
    _figures.erase(actor);
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
    _queue.clear();
    _running.clear();
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
    Running running{.cue = std::move(next), .elapsed = 0.0F, .from = {}};
    if (const FigureMotion* figure = motionOf(running.cue.actor)) {
        running.from = figure->point;
    }
    const bool strike =
        running.cue.kind == CombatCueKind::Attack || running.cue.kind == CombatCueKind::Cast;
    const core::CombatantId attacker = running.cue.actor;
    _running.push_back(std::move(running));
    if (!strike) {
        return;
    }
    // Le coup porte au milieu du geste : les touches et les chutes qui le suivent immediatement
    // -- ceux d'AUTRES combattants -- demarrent a cet instant, pas apres le geste entier.
    while (!_queue.empty() && isVictimCue(_queue.front().kind) &&
           _queue.front().actor != attacker) {
        _running.push_back(Running{.cue = std::move(_queue.front()),
                                   .elapsed = -ACTION_SECONDS * IMPACT_FRACTION,
                                   .from = {}});
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
        case CombatCueKind::Walk: {
            const std::vector<core::GridPosition>& path = running.cue.path;
            const float total = static_cast<float>(path.size()) / WALK_CELLS_PER_SECOND;
            if (elapsed >= total) {
                // Arrive : tourne dans le sens du dernier pas, meme si l'image precedente n'a pas
                // eu le temps de le montrer.
                const core::Vector2 avant =
                    path.size() >= 2 ? centerOf(path[path.size() - 2]) : running.from;
                figure.point = centerOf(path.back());
                figure.facing = facingTowards(avant, figure.point, figure.facing);
                figure.clip = figure_clips::IDLE;
                figure.clipSeconds = 0.0F;
                return true;
            }
            const float progress = std::max(0.0F, elapsed) * WALK_CELLS_PER_SECOND;
            const auto segment = static_cast<std::size_t>(std::floor(progress));
            const float fraction = progress - static_cast<float>(segment);
            const core::Vector2 from =
                segment == 0 ? running.from : centerOf(path[segment - 1]);
            const core::Vector2 to = centerOf(path[std::min(segment, path.size() - 1)]);
            figure.point = core::Vector2{from.x + ((to.x - from.x) * fraction),
                                         from.y + ((to.y - from.y) * fraction)};
            figure.facing = facingTowards(from, to, figure.facing);
            figure.clip = figure_clips::WALK;
            figure.clipSeconds = std::max(0.0F, elapsed);
            return false;
        }
        case CombatCueKind::Attack:
        case CombatCueKind::Cast: {
            if (running.cue.target.has_value()) {
                figure.facing = facingTowards(figure.point, centerOf(*running.cue.target),
                                              figure.facing);
            }
            if (elapsed >= ACTION_SECONDS) {
                figure.clip = figure_clips::IDLE;
                figure.clipSeconds = 0.0F;
                return true;
            }
            figure.clip = running.cue.kind == CombatCueKind::Attack ? figure_clips::ATTACK
                                                                    : figure_clips::CAST;
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
            if (elapsed >= ACTION_SECONDS) {
                figure.clip = figure_clips::IDLE;
                figure.clipSeconds = 0.0F;
                return true;
            }
            figure.clip = figure_clips::HIT;
            figure.clipSeconds = elapsed;
            return false;
        }
        case CombatCueKind::Death: {
            if (elapsed < 0.0F) {
                return false;
            }
            figure.dead = true;
            figure.clip = figure_clips::DEATH;
            // La bande de mort ne revient jamais au repos : ses secondes continuent de courir, et
            // le rendu la fige sur sa derniere image (`SceneTexture::loop`).
            figure.clipSeconds = elapsed;
            return elapsed >= ACTION_SECONDS;
        }
    }
    return true;
}

void CombatCueTrack::advance(float seconds) {
    // Les figurines au repos respirent : leur bande de repos avance avec le temps.
    for (auto& [actor, figure] : _figures) {
        if (figure.clip == figure_clips::IDLE) {
            figure.clipSeconds += seconds;
        } else if (figure.dead && figure.clip == figure_clips::DEATH) {
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
    for (std::size_t guard = 0; guard < 4096 && busy(); ++guard) {
        advance(1.0F);
    }
}

}  // namespace hmi
