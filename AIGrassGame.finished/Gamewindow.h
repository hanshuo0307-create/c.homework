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

// 【视觉特效结构体】
struct FloatingHeartParticle {
    QPointF pos;
    double speedY;
    int lifetime;
    int initialLifetime;
};

struct SkillVFX {
    int type;
    int r, c;
    int lifetime;
    int maxLifetime;
};

// 【核心游戏窗口类】
class Gamewindow : public QWidget {
    Q_OBJECT
public:
    Gamewindow(QWidget *parent = nullptr);
protected:
    // 【事件拦截器】处理屏幕渲染与键鼠交互
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
private slots:
    // 【引擎主循环】60FPS心跳函数
    void gameLoop();
private:
    // --- 【核心引擎：背景音乐播放器】 ---
    void changeBGM(QString path);
    QSoundEffect *bgmPlayer;
    QSoundEffect *deathSound, *medTriggerSound;
    // ------------------------------------

    // 【核心状态机与矩阵数据】
    jvzhen gameMap;
    int px, py, totalKills, currentLevel;

    int boundTop, boundBottom, boundLeft, boundRight;
    int kfcR, kfcC;

    double playerHP, playerMaxHP, playerMP;
    double visualPx, visualPy;
    int logicFrame;

    // 【视觉平滑补偿矩阵】
    QVector<QVector<double>> offsetR;
    QVector<QVector<double>> offsetC;
    QMap<int, bool> keys;

    int gameState; // 状态机流转
    int selectedCharacter;
    bool isDesktopMode;

    bool isShowingRules;
    int hoverButton;

    // 【UI 碰撞体】
    QRect rectStartBtn, rectRuleBtn, rectExitBtn;
    QRect rectCloseBtn, rectReturnBtn;
    QRect rectPickLeft, rectPickRight;
    QRect rectEnvNKU, rectEnvDesktop;

    // 【渲染管线：背景资源】
    QPixmap startBackground;
    QPixmap ruleBackground;
    QPixmap selectBackground;
    QPixmap envBackground;

    QPixmap trans_1, trans_2, trans_3, trans_4;
    QPixmap deadBackground;
    QPixmap victoryBackground;

    QPixmap pic_skill_i, pic_skill_j, pic_skill_k, pic_skill_l;
    QList<SkillVFX> activeVFXs;

    QPixmap bg_lv1, bg_lv2, bg_lv3, bg_lv4;

    int frameCount, shakeFrames;
    bool bossSpawned, isDead, isVictorious;
    int medRespawnTimer;

    int victoryTimer; // 电影级转场倒计时核心变量

    bool skillUnlocked[3];
    int skillCooldown[3];

    QTimer *timer;

    // 【渲染管线：动图帧序列缓存】(原封不动保留！)
    QVector<QPixmap> m1_alive, m1_dead, m2_alive, m2_dead, m3_alive, m3_dead;
    QVector<QPixmap> mb1_alive, mb1_dead, mb2_alive, mb2_dead, bb_alive, bb_dead;

    QVector<QPixmap> motor_anim, milk_anim;
    QVector<QPixmap> med_ready_hearts, med_alive_cat;
    QList<FloatingHeartParticle> clickHealHearts;
};
#endif
