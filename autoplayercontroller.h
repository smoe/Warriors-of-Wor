#ifndef AUTOPLAYERCONTROLLER_H
#define AUTOPLAYERCONTROLLER_H

#include <QList>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QTimer>

class Character;
class MapTile;
class Player;
class PlayfieldInfo;

class AutoPlayerController : public QObject
{
    Q_OBJECT

public:
    explicit AutoPlayerController(Player *controlledPlayer,
                                  PlayfieldInfo *playfieldInfo,
                                  QList<Character*> &characters,
                                  QObject *parent = nullptr);

signals:
    void controlInput(QPoint dir);
    void aimInput(QPoint dir);
    void buttonPressed(int button);

private slots:
    void decide();

private:
    Character *nearestEnemy(MapTile *from) const;
    Player *nearestPlayer(MapTile *from) const;
    QPoint directionToward(MapTile *from, MapTile *target) const;
    QPoint directionAwayFrom(MapTile *from, MapTile *threat) const;
    QPoint combatDirection(MapTile *from, MapTile *target) const;
    QPoint strategicDirection(MapTile *from, MapTile *target) const;
    QPoint spacingDirection(MapTile *from) const;
    QPoint bestShotDirection(MapTile *from) const;
    QPoint lineDirection(MapTile *from, MapTile *target) const;
    bool hasClearShot(MapTile *from, MapTile *target) const;
    bool laneIsOpen(MapTile *from, MapTile *target) const;
    bool isViableStep(MapTile *tile) const;
    bool isSafeStep(MapTile *tile) const;
    int stepDanger(MapTile *tile) const;
    bool enemyNear(MapTile *tile, int range) const;
    bool enemyCanShootTile(MapTile *tile) const;
    bool enemyBeamThreatensTile(MapTile *tile) const;
    bool hasEnemy() const;
    bool isWarpApproach(MapTile *tile) const;
    bool playerOccupies(MapTile *tile) const;
    int enemyCount() const;
    int strategicScore(MapTile *tile, MapTile *target) const;
    int shotScore(MapTile *tile) const;
    int maxShotDistance() const;
    int tileDistance(MapTile *a, MapTile *b) const;
    void steer(QPoint dir);

    QPointer<Player> player;
    PlayfieldInfo *playfield;
    QList<Character*> &characters;
    QTimer decisionTimer;
    QPoint lastDirection;
    int fireCooldown;
    int aggression;
    int mobility;
    int partnerBias;
};

#endif // AUTOPLAYERCONTROLLER_H
