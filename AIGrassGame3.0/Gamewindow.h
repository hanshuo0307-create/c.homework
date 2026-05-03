#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QImageReader>
#include <QVector>
#include <QPixmap>
#include <QSoundEffect>
#include <QGuiApplication>
#include <QList>
#include <QMap>

#include "jvzhen.h"
#include "layer.h"
#include "ConvLayer.h"
#include "Skillfactor.h"
#include "MonsterAILayer.h"
#include "GameUtils.h"

struct FloatingHeartParticle {
    QPointF pos;
    double speedY;
    int lifetime;
    int initialLifetime;
};

class Gamewindow : public QWidget {
    Q_OBJECT
public:
    Gamewindow(QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
private slots:
    void gameLoop();
private:
    jvzhen gameMap;
    int px, py, totalKills, currentLevel;

    double playerHP, playerMaxHP, playerMP;

    double visualPx, visualPy;
    int logicFrame;

    QVector<QVector<double>> offsetR;
    QVector<QVector<double>> offsetC;

    QMap<int, bool> keys;

    int frameCount, shakeFrames;
    bool bossSpawned, isDead, isSelectingSkill, isVictorious;
    int medRespawnTimer;

    bool skillUnlocked[3];
    // --- 【全新】：技能冷却追踪器 ---
    int skillCooldown[3];
    // -----------------------------

    QTimer *timer;
    QSoundEffect *deathSound, *medTriggerSound;

    QVector<QPixmap> m1_alive, m1_dead, m2_alive, m2_dead, m3_alive, m3_dead;
    QVector<QPixmap> mb1_alive, mb1_dead, mb2_alive, mb2_dead, bb_alive, bb_dead;
    QPixmap player_lv1, player_lv2, player_lv3, player_lv4;
    QVector<QPixmap> med_ready_hearts, med_alive_cat;
    QList<FloatingHeartParticle> clickHealHearts;
};
#endif
