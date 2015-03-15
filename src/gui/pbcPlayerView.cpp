#include "pbcPlayerView.h"

#include "models/pbcPlaybook.h"
#include "QBrush"
#include "QMenu"
#include "util/pbcPositionTranslator.h"
#include "dialogs/pbcCustomRouteDialog.h"
#include <boost/unordered/unordered_map.hpp>
#include <utility>
#include "QDebug"
#include <vector>


PBCPlayerView::PBCPlayerView(PBCPlayerSP playerSP) : _playerSP(playerSP) {
    _originalPos = PBCPositionTranslator::getInstance()->translatePos(_playerSP->pos());  // NOLINT
    unsigned int playerWidth = PBCConfig::getInstance()->playerWidth();
    double playerPosX = _originalPos.get<0>() - playerWidth / 2;
    double playerPosY = _originalPos.get<1>();
    _playerEllipseSP.reset(new QGraphicsEllipseItem(playerPosX,
                                                    playerPosY,
                                                    playerWidth,
                                                    playerWidth));

    PBCColor color = _playerSP->color();
    _playerEllipseSP->setBrush(QBrush(QColor(color.r(), color.g(), color.b())));
    this->addToGroup(_playerEllipseSP.get());
    this->setFlag(QGraphicsItem::ItemIsMovable);

    if(_playerSP->route() != NULL) {
        applyRoute(_playerSP->route());
    }
}

void PBCPlayerView::applyRoute(PBCRouteSP route) {
    for(boost::shared_ptr<QGraphicsLineItem> line : _routeLines) {
        this->removeFromGroup(line.get());
    }
    _routeLines.clear();

    _playerSP->setRoute(route);
    QRectF ellipseRect = _playerEllipseSP->rect();
    double baseX = ellipseRect.x() +
            _playerEllipseSP->scenePos().x() +  // TODO(obr): check why we need this // NOLINT
            ellipseRect.width() / 2;

    double baseY = ellipseRect.y() + _playerEllipseSP->scenePos().y();
    double lastX = baseX;
    double lastY = baseY;
    for(PBCPathSP path : route->paths()) {
        PBCDPoint pos = PBCPositionTranslator::getInstance()->translatePos(path->endpoint(), PBCDPoint(baseX, baseY));  // NOLINT
        int newX = -1;
        if(baseX < PBCConfig::getInstance()->canvasWidth() / 2) {
            newX = pos.get<0>();
        } else {
            newX = 2 * baseX - pos.get<0>();
        }
        int newY = pos.get<1>();
        boost::shared_ptr<QGraphicsLineItem> lineSP(
                    new QGraphicsLineItem(lastX, lastY, newX, newY));
        PBCColor color = _playerSP->color();
        lineSP->setPen(QPen(QColor(color.r(), color.g(), color.b())));
        _routeLines.push_back(lineSP);
        lastX = newX;
        lastY = newY;
    }

    for(boost::shared_ptr<QGraphicsLineItem> line : _routeLines) {
        this->addToGroup(line.get());
    }
}


void PBCPlayerView::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    QMenu menu;
    QMenu* routeMenu = menu.addMenu(QString::fromStdString("Apply Route"));
    boost::unordered_map<QAction*, PBCRouteSP> actionMap;
    for(PBCRouteSP route : PBCPlaybook::getInstance()->routes()) {
        QAction* action = routeMenu->addAction(
                    QString::fromStdString(route->name()));
        actionMap.insert(std::make_pair(action, route));
    }

    routeMenu->addSeparator();
    QAction* action_CustomRouteCreate =
            routeMenu->addAction("Create Custom Route");
    // QAction* motion = menu.addAction(QString("Apply Motion"));
    QAction* clicked = menu.exec(event->screenPos());
    setEnabled(false);
    setEnabled(true);

    bool routeClicked = false;
    for(const auto& kv : actionMap) {
        if(clicked == kv.first) {
            routeClicked = true;
            this->applyRoute(kv.second);
        }
    }
    if(routeClicked == false) {
        if(clicked == action_CustomRouteCreate) {
            PBCCustomRouteDialog dialog;
            dialog.setWindowModality(Qt::ApplicationModal);
            PBCRouteSP createdRoute = dialog.exec();
            if(createdRoute != NULL) {
                this->applyRoute(createdRoute);
            }
        } else {
            assert(clicked == NULL);
        }
    }
}

void PBCPlayerView::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    QGraphicsItemGroup::mouseReleaseEvent(event);
    QPointF pixelDelta = this->pos();
    QPointF newPixelPos = QPointF(_originalPos.get<0>(),
                                  _originalPos.get<1>()) + pixelDelta;
    PBCDPoint newPos = PBCPositionTranslator::getInstance()->retranslatePos(PBCDPoint(newPixelPos.x(), newPixelPos.y()));  //NOLINT
    std::cout << pixelDelta.x() << ", " << pixelDelta.y() << std::endl;
    std::cout << newPos.get<0>() << ", " << newPos.get<1>() << std::endl;
    std::cout << "----------------------------------" << std::endl;
    _playerSP->setPos(newPos);
}

