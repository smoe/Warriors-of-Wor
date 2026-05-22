#include "autoplayercontroller.h"

#include <climits>

#include <QGraphicsScene>
#include <QQueue>

#include "characters/enemycharacter.h"
#include "lazorbeam.h"
#include "player.h"
#include "playfieldinfo.h"
#include "spawntile.h"
#include "warptile.h"

static const int MaxShotDistance = 7;
static const int IdealEnemyDistance = 4;
static const int BeamLookaheadTiles = 3;

AutoPlayerController::AutoPlayerController(Player *controlledPlayer,
                                           PlayfieldInfo *playfieldInfo,
                                           QList<Character*> &chars,
                                           QObject *parent) :
    QObject(parent),
    player(controlledPlayer),
    playfield(playfieldInfo),
    characters(chars),
    lastDirection(1, 0),
    fireCooldown(0),
    aggression(1),
    mobility(1),
    partnerBias(1)
{
    switch(player ? player->number() % 3 : 0) {
    case 0:
        aggression = 1;
        mobility = 1;
        partnerBias = 2;
        break;
    case 1:
        aggression = 2;
        mobility = 2;
        partnerBias = 1;
        break;
    default:
        aggression = 0;
        mobility = 0;
        partnerBias = 1;
        break;
    }

    decisionTimer.setInterval(180);
    connect(&decisionTimer, SIGNAL(timeout()), this, SLOT(decide()));
    decisionTimer.start();
}

void AutoPlayerController::decide()
{
    if(fireCooldown > 0)
        fireCooldown--;

    if(!player || !playfield || !player->isVisible())
        return;

    MapTile *tile = player->currentTile();
    if(!tile)
        return;

    if(!player->getControllable()) {
        SpawnTile *spawn = playfield->spawnPoint(player->number());
        if(spawn && !spawn->direction().isNull()) {
            MapTile *exit = playfield->tileAt(spawn->position() + spawn->direction());
            if(exit && !playfield->tileContainsEnemy(exit))
                steer(spawn->direction());
            else
                steer(QPoint());
        } else {
            steer(QPoint());
        }
        return;
    }

    Character *enemy = nearestEnemy(tile);
    QPoint shotDirection = bestShotDirection(tile);

    if(!shotDirection.isNull() && fireCooldown == 0 && player->weaponReady()) {
        emit aimInput(shotDirection);
        steer(QPoint());
        emit buttonPressed(1);
        fireCooldown = 4;
        return;
    }
    if(!shotDirection.isNull())
        emit aimInput(shotDirection);

    if(enemyBeamThreatensTile(tile) || enemyCanShootTile(tile)) {
        QPoint dir = enemy ? directionAwayFrom(tile, enemy->currentTile()) : combatDirection(tile, nullptr);
        if(!dir.isNull()) {
            steer(dir);
            return;
        }
    }

    if(enemy) {
        MapTile *enemyTile = enemy->currentTile();
        if(enemyNear(tile, 1)) {
            QPoint dir = directionAwayFrom(tile, enemyTile);
            if(!dir.isNull()) {
                steer(dir);
                return;
            }
        }

        QPoint dir = combatDirection(tile, enemyTile);
        if(!dir.isNull()) {
            steer(dir);
            return;
        }

        dir = strategicDirection(tile, enemyTile);
        if(!dir.isNull()) {
            steer(dir);
            return;
        }
    }

    QPoint strategicDir = strategicDirection(tile, nullptr);
    if(!strategicDir.isNull()) {
        steer(strategicDir);
        return;
    }

    QPoint dir = spacingDirection(tile);
    if(!dir.isNull()) {
        steer(dir);
        return;
    }

    steer(QPoint());
}

Character *AutoPlayerController::nearestEnemy(MapTile *from) const
{
    Character *nearest = nullptr;
    int bestDistance = INT_MAX;

    foreach(Character *character, characters) {
        EnemyCharacter *enemy = qobject_cast<EnemyCharacter*>(character);
        if(!enemy || !enemy->isVisible())
            continue;

        MapTile *enemyTile = enemy->currentTile();
        int distance = tileDistance(from, enemyTile);
        if(distance < bestDistance) {
            bestDistance = distance;
            nearest = enemy;
        }
    }

    return nearest;
}

Player *AutoPlayerController::nearestPlayer(MapTile *from) const
{
    Player *nearest = nullptr;
    int bestDistance = INT_MAX;

    foreach(Character *character, characters) {
        Player *candidate = qobject_cast<Player*>(character);
        if(!candidate || candidate == player || !candidate->isVisible())
            continue;

        MapTile *candidateTile = candidate->currentTile();
        int distance = tileDistance(from, candidateTile);
        if(distance < bestDistance) {
            bestDistance = distance;
            nearest = candidate;
        }
    }

    return nearest;
}

QPoint AutoPlayerController::directionToward(MapTile *from, MapTile *target) const
{
    if(!from || !target)
        return QPoint();

    const QPoint dirs[] = {
        QPoint(1, 0),
        QPoint(0, 1),
        QPoint(-1, 0),
        QPoint(0, -1)
    };

    bool visited[MAPW][MAPH] = {};
    QPoint firstStep[MAPW][MAPH];
    QQueue<MapTile*> frontier;
    TilePos start = from->position();

    if(start.x() < 0 || start.x() >= MAPW || start.y() < 0 || start.y() >= MAPH)
        return QPoint();

    visited[start.x()][start.y()] = true;
    frontier.enqueue(from);

    while(!frontier.isEmpty()) {
        MapTile *tile = frontier.dequeue();
        if(tile == target)
            return firstStep[tile->position().x()][tile->position().y()];

        for(const QPoint &dir : dirs) {
            if(!tile->canWalkTo(dir))
                continue;

            MapTile *next = tile->tileTo(dir);
            if(!isViableStep(next))
                continue;

            TilePos pos = next->position();
            if(pos.x() < 0 || pos.x() >= MAPW || pos.y() < 0 || pos.y() >= MAPH)
                continue;
            if(visited[pos.x()][pos.y()])
                continue;

            visited[pos.x()][pos.y()] = true;
            firstStep[pos.x()][pos.y()] = tile == from
                    ? dir
                    : firstStep[tile->position().x()][tile->position().y()];
            frontier.enqueue(next);
        }
    }

    return QPoint();
}

QPoint AutoPlayerController::directionAwayFrom(MapTile *from, MapTile *threat) const
{
    if(!from || !threat)
        return QPoint();

    const QPoint dirs[] = {
        QPoint(1, 0),
        QPoint(0, 1),
        QPoint(-1, 0),
        QPoint(0, -1)
    };

    QPoint bestDirection;
    int bestScore = -1;

    for(const QPoint &dir : dirs) {
        if(!from->canWalkTo(dir))
            continue;

        MapTile *next = from->tileTo(dir);
        if(!isViableStep(next))
            continue;

        int score = tileDistance(next, threat) - stepDanger(next);
        if(isWarpApproach(next))
            score -= 2;
        if(dir == lastDirection)
            score++;

        if(score > bestScore) {
            bestScore = score;
            bestDirection = dir;
        }
    }

    return bestDirection;
}

QPoint AutoPlayerController::combatDirection(MapTile *from, MapTile *target) const
{
    if(!from)
        return QPoint();

    const QPoint dirs[] = {
        QPoint(1, 0),
        QPoint(0, 1),
        QPoint(-1, 0),
        QPoint(0, -1)
    };

    QPoint bestDirection;
    int bestScore = -INT_MAX;

    for(const QPoint &dir : dirs) {
        if(!from->canWalkTo(dir))
            continue;
        MapTile *next = from->tileTo(dir);
        if(!next || !playfield->tileIsValidLocation(next))
            continue;
        if(!isViableStep(next))
            continue;

        int score = shotScore(next) * (7 + aggression * 2) + strategicScore(next, target);

        score -= stepDanger(next);
        if(isWarpApproach(next))
            score -= 5;

        if(target) {
            int distance = tileDistance(next, target);
            int idealDistance = qMax(3, IdealEnemyDistance - aggression);
            score -= qAbs(idealDistance - distance);
            if(distance < 3)
                score -= (3 - distance) * 5;
        }

        if(dir == lastDirection)
            score++;

        if(score > bestScore) {
            bestScore = score;
            bestDirection = dir;
        }
    }

    return bestDirection;
}

QPoint AutoPlayerController::strategicDirection(MapTile *from, MapTile *target) const
{
    if(!from)
        return QPoint();

    const TilePos positions[] = {
        TilePos(3, 2),
        TilePos(9, 2),
        TilePos(3, 5),
        TilePos(9, 5),
        TilePos(6, 2),
        TilePos(6, 5),
        TilePos(2, 3),
        TilePos(10, 3),
        TilePos(6, 3),
        TilePos(6, 4)
    };

    int enemies = enemyCount();
    MapTile *bestTile = nullptr;
    int bestScore = -INT_MAX;

    for(const TilePos &position : positions) {
        MapTile *candidate = playfield->tileAt(position);
        if(!candidate || !playfield->tileIsValidLocation(candidate))
            continue;
        if(candidate->content() & (MapTile::MT_CONTENT_FIRE |
                                   MapTile::MT_CONTENT_BLOCK |
                                   MapTile::MT_CONTENT_SAND))
            continue;
        if(playerOccupies(candidate))
            continue;

        int score = strategicScore(candidate, target);
        if(!target)
            score += tileDistance(from, candidate) * mobility;
        else if(enemies >= 3)
            score -= tileDistance(from, candidate) * 2;
        else
            score -= tileDistance(from, candidate) * (2 - mobility);

        if(score > bestScore) {
            bestScore = score;
            bestTile = candidate;
        }
    }

    if(!bestTile || bestTile == from)
        return QPoint();

    return directionToward(from, bestTile);
}

QPoint AutoPlayerController::spacingDirection(MapTile *from) const
{
    if(!from)
        return QPoint();

    Player *ally = nearestPlayer(from);
    if(!ally)
        return QPoint();

    MapTile *allyTile = ally->currentTile();
    int allyDistance = tileDistance(from, allyTile);
    if(allyDistance >= 3 && allyDistance <= 5 && !isWarpApproach(from))
        return QPoint();

    const QPoint dirs[] = {
        QPoint(1, 0),
        QPoint(0, 1),
        QPoint(-1, 0),
        QPoint(0, -1)
    };

    QPoint bestDirection;
    int bestScore = -INT_MAX;

    for(const QPoint &dir : dirs) {
        if(!from->canWalkTo(dir))
            continue;

        MapTile *next = from->tileTo(dir);
        if(!isViableStep(next))
            continue;

        int score = 0;
        int nextAllyDistance = tileDistance(next, allyTile);
        if(allyDistance < 3) {
            score = nextAllyDistance;
        } else {
            score = -qAbs(4 - nextAllyDistance);
        }
        if(isWarpApproach(next))
            score -= 5;
        if(dir == lastDirection)
            score++;

        if(score > bestScore) {
            bestScore = score;
            bestDirection = dir;
        }
    }

    return bestDirection;
}

QPoint AutoPlayerController::bestShotDirection(MapTile *from) const
{
    if(!from)
        return QPoint();

    QPoint bestDirection;
    int bestScore = -1;

    foreach(Character *character, characters) {
        EnemyCharacter *enemy = qobject_cast<EnemyCharacter*>(character);
        if(!enemy || !enemy->isVisible())
            continue;

        MapTile *enemyTile = enemy->currentTile();
        if(!hasClearShot(from, enemyTile))
            continue;

        QPoint dir = lineDirection(from, enemyTile);
        int distance = tileDistance(from, enemyTile);
        int score = maxShotDistance() - distance;
        if(player->aimDirection() == dir)
            score += 4;
        if(enemyCanShootTile(from))
            score += 2;

        if(score > bestScore) {
            bestScore = score;
            bestDirection = dir;
        }
    }

    return bestDirection;
}

QPoint AutoPlayerController::lineDirection(MapTile *from, MapTile *target) const
{
    if(!from || !target || from == target)
        return QPoint();

    QPoint delta = target->position() - from->position();
    if(delta.x() == 0)
        return QPoint(0, delta.y() > 0 ? 1 : -1);
    if(delta.y() == 0)
        return QPoint(delta.x() > 0 ? 1 : -1, 0);

    return QPoint();
}

bool AutoPlayerController::hasClearShot(MapTile *from, MapTile *target) const
{
    QPoint dir = lineDirection(from, target);
    if(dir.isNull())
        return false;
    if(tileDistance(from, target) > maxShotDistance())
        return false;

    MapTile *tile = from;
    while(tile && tile != target) {
        if(!tile->canWalkTo(dir))
            return false;

        tile = tile->tileTo(dir);
        if(tile && tile != target && playerOccupies(tile))
            return false;
    }

    return tile == target;
}

bool AutoPlayerController::laneIsOpen(MapTile *from, MapTile *target) const
{
    QPoint dir = lineDirection(from, target);
    if(dir.isNull())
        return false;

    MapTile *tile = from;
    while(tile && tile != target) {
        tile = tile->tileTo(dir);
    }

    return tile == target;
}

bool AutoPlayerController::isViableStep(MapTile *tile) const
{
    if(!tile || !playfield->tileIsValidLocation(tile))
        return false;

    if(tile->content() & (MapTile::MT_CONTENT_FIRE |
                          MapTile::MT_CONTENT_BLOCK |
                          MapTile::MT_CONTENT_SAND))
        return false;

    return !playfield->tileContainsEnemy(tile) && !playerOccupies(tile);
}

bool AutoPlayerController::isSafeStep(MapTile *tile) const
{
    return isViableStep(tile) && stepDanger(tile) == 0;
}

int AutoPlayerController::stepDanger(MapTile *tile) const
{
    if(!tile)
        return 1000;

    int danger = 0;
    if(enemyNear(tile, 1))
        danger += 20;
    else if(enemyNear(tile, 2))
        danger += 8;
    if(enemyCanShootTile(tile))
        danger += 18;
    if(enemyBeamThreatensTile(tile))
        danger += 28;
    return danger;
}

bool AutoPlayerController::enemyNear(MapTile *tile, int range) const
{
    if(!tile)
        return false;

    foreach(Character *character, characters) {
        EnemyCharacter *enemy = qobject_cast<EnemyCharacter*>(character);
        if(!enemy || !enemy->isVisible())
            continue;

        MapTile *enemyTile = enemy->currentTile();
        if(tileDistance(tile, enemyTile) <= range) {
            if(range <= 1)
                return tile == enemyTile || laneIsOpen(tile, enemyTile);
            return true;
        }
    }

    return false;
}

bool AutoPlayerController::enemyCanShootTile(MapTile *tile) const
{
    if(!tile)
        return false;

    foreach(Character *character, characters) {
        EnemyCharacter *enemy = qobject_cast<EnemyCharacter*>(character);
        if(!enemy || !enemy->isVisible())
            continue;

        MapTile *enemyTile = enemy->currentTile();
        QPoint dir = lineDirection(enemyTile, tile);
        if(dir.isNull() || dir != enemy->aimDirection())
            continue;

        if(laneIsOpen(enemyTile, tile))
            return true;
    }

    return false;
}

bool AutoPlayerController::enemyBeamThreatensTile(MapTile *tile) const
{
    if(!tile || !player || !player->scene())
        return false;

    QList<QGraphicsItem*> items = player->scene()->items();
    foreach(QGraphicsItem *item, items) {
        LazorBeam *beam = dynamic_cast<LazorBeam*>(item);
        if(!beam || !beam->isEnemys())
            continue;

        QPoint dir = beam->tileDirection();
        MapTile *beamTile = playfield->tileAt(beam->pos());
        for(int i = 0; beamTile && i < BeamLookaheadTiles; i++) {
            if(beamTile == tile)
                return true;
            beamTile = beamTile->tileTo(dir);
        }
    }

    return false;
}

bool AutoPlayerController::hasEnemy() const
{
    foreach(Character *character, characters) {
        EnemyCharacter *enemy = qobject_cast<EnemyCharacter*>(character);
        if(enemy && enemy->isVisible())
            return true;
    }

    return false;
}

bool AutoPlayerController::isWarpApproach(MapTile *tile) const
{
    if(!tile)
        return false;

    return qobject_cast<WarpTile*>(tile) ||
            qobject_cast<WarpTile*>(tile->w()) ||
            qobject_cast<WarpTile*>(tile->e());
}

bool AutoPlayerController::playerOccupies(MapTile *tile) const
{
    if(!tile)
        return false;

    foreach(Character *character, characters) {
        Player *candidate = qobject_cast<Player*>(character);
        if(candidate && candidate != player && candidate->isVisible() &&
                candidate->currentTile() == tile)
            return true;
    }

    return false;
}

int AutoPlayerController::enemyCount() const
{
    int count = 0;

    foreach(Character *character, characters) {
        EnemyCharacter *enemy = qobject_cast<EnemyCharacter*>(character);
        if(enemy && enemy->isVisible())
            count++;
    }

    return count;
}

int AutoPlayerController::strategicScore(MapTile *tile, MapTile *target) const
{
    if(!tile)
        return -INT_MAX / 2;

    int score = 0;
    if(isWarpApproach(tile))
        score -= 5;
    if(enemyCanShootTile(tile))
        score -= 14;
    if(enemyBeamThreatensTile(tile))
        score -= 20;
    if(enemyNear(tile, 2))
        score -= 8;

    score += shotScore(tile) * (2 + aggression);

    Player *ally = nearestPlayer(tile);
    if(ally) {
        int allyDistance = tileDistance(tile, ally->currentTile());
        score -= qAbs(4 - allyDistance) * partnerBias;
        if(allyDistance < 2)
            score -= 4 * partnerBias;
    }

    if(target) {
        int distance = tileDistance(tile, target);
        int idealDistance = qMax(3, IdealEnemyDistance - aggression);
        score -= qAbs(idealDistance - distance);
        if(distance < 3)
            score -= (3 - distance) * 4;
    }

    return score;
}

int AutoPlayerController::shotScore(MapTile *tile) const
{
    if(!tile)
        return 0;

    int bestScore = 0;

    foreach(Character *character, characters) {
        EnemyCharacter *enemy = qobject_cast<EnemyCharacter*>(character);
        if(!enemy || !enemy->isVisible())
            continue;

        MapTile *enemyTile = enemy->currentTile();
        if(!hasClearShot(tile, enemyTile))
            continue;

        int distance = tileDistance(tile, enemyTile);
        int score = maxShotDistance() - distance + 1;
        if(lineDirection(tile, enemyTile) == player->aimDirection())
            score += 2;
        if(score > bestScore)
            bestScore = score;
    }

    return bestScore;
}

int AutoPlayerController::maxShotDistance() const
{
    return MaxShotDistance + aggression;
}

int AutoPlayerController::tileDistance(MapTile *a, MapTile *b) const
{
    if(!a || !b)
        return INT_MAX;

    QPoint delta = a->position() - b->position();
    return qAbs(delta.x()) + qAbs(delta.y());
}

void AutoPlayerController::steer(QPoint dir)
{
    emit controlInput(dir);
    lastDirection = dir;
}
