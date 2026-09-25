// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/GameViewportItem.h"

#include <rhi/qrhi.h>

#include "HMI/HmiLog.h"

namespace hmi {
namespace {

// Le peintre, côté **fil de rendu**.
//
// Il ne partage aucun état avec l'élément : tout ce dont il a besoin lui est remis par
// `synchronize()`, appelée pendant que le fil graphique est bloqué. C'est la discipline que
// `QQuickRhiItem` impose, et la contourner ne produirait pas une erreur mais une corruption
// intermittente — le pire des deux.
class GameViewportRenderer : public QQuickRhiItemRenderer {
public:
    void initialize(QRhiCommandBuffer* commandBuffer) override;
    void synchronize(QQuickRhiItem* item) override;
    void render(QRhiCommandBuffer* commandBuffer) override;

private:
    QRhi* _rhi = nullptr;
    QColor _clearColor{0xd0, 0xc0, 0xa0};
};

void GameViewportRenderer::initialize(QRhiCommandBuffer* /*commandBuffer*/) {
    // `rhi()` peut changer : Qt Quick recrée ses ressources quand la fenêtre change d'écran ou de
    // contexte graphique. Comparer plutôt que supposer, exactement comme `hmi::RhiContext` le fait
    // côté éditeur — c'est la même contrainte, et elle a déjà coûté un diagnostic.
    if (_rhi == rhi()) {
        return;
    }
    _rhi = rhi();
    HMI_LOG_INFO("Viewport du jeu : interface de rendu QRhi initialisee (" +
                 std::string(_rhi != nullptr ? _rhi->backendName() : "aucune") + ").");
}

void GameViewportRenderer::synchronize(QQuickRhiItem* item) {
    // Le SEUL instant où les deux fils peuvent se parler sans verrou : le fil graphique est bloqué.
    // Tout ce que la simulation produira devra passer par ici — pour l'instant, une couleur.
    // Le moteur ne passe que l'élément qui a créé ce rendu (createRenderer) : qobject_cast ne
    // peut échouer, et reste une vérification bon marché plutôt qu'un transtypage aveugle.
    auto* const viewport = qobject_cast<GameViewportItem*>(item);
    if (viewport == nullptr) {
        return;
    }
    _clearColor = viewport->clearColor();
}

void GameViewportRenderer::render(QRhiCommandBuffer* commandBuffer) {
    // Une passe qui ne fait qu'effacer, pour l'instant : `Source/Elements/Levels/` est vide par
    // construction, et il n'y a aucune scène à composer. La passe existe quand même, et c'est ce
    // qui se vérifie — sans elle, la texture d'appui contiendrait ce que la mémoire graphique y
    // avait laissé.
    commandBuffer->beginPass(renderTarget(), _clearColor, {1.0F, 0});
    commandBuffer->endPass();
}

}  // namespace

GameViewportItem::GameViewportItem(QQuickItem* parent) : QQuickRhiItem(parent) {}

void GameViewportItem::setClearColor(const QColor& color) {
    if (_clearColor == color) {
        return;
    }
    _clearColor = color;
    emit clearColorChanged();
    update();  // sans quoi la couleur ne parvient au fil de rendu qu'à la prochaine image demandée.
}

QQuickRhiItemRenderer* GameViewportItem::createRenderer() {
    return new GameViewportRenderer;
}

}  // namespace hmi
