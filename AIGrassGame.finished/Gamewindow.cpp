#include "Gamewindow.h"
#include <ctime>
#include <QImageReader>
#include <algorithm>
#include <cmath>

// ====== 【系统初始化与资源装载层】 ======
Gamewindow::Gamewindow(QWidget *parent) : QWidget(parent), gameMap(1, 1) {
    srand(time(NULL));
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    setFixedSize(screenRect.width(), screenRect.height());

    // 窗口无边框、置顶、背景透明化（桌宠模式）
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    // 状态机与核心变量初始化
    gameState = 0;
    selectedCharacter = 1;
    isDesktopMode = false;
    isShowingRules = false;
    hoverButton = 0;
    logicFrame = 0;
    frameCount = 0;
    shakeFrames = 0;
    victoryTimer = 0;

    auto loadGif = [](QString path, QVector<QPixmap>& frames) {
        QImageReader reader(path);
        while (reader.canRead()) {
            QImage img = reader.read();
            if (!img.isNull()) frames.append(QPixmap::fromImage(img));
        }
    };

    // 内存预热：将所有怪物动图载入内存
    loadGif(":/potato_cat.gif", m1_alive); loadGif(":/ghost.gif", m1_dead);
    loadGif(":/beecat.gif", m2_alive); loadGif(":/ghost.gif", m2_dead);
    loadGif(":/gun_cat.gif", m3_alive); loadGif(":/ghost.gif", m3_dead);
    loadGif(":/Hitler.gif", mb1_alive); loadGif(":/ghost.gif", mb1_dead);
    loadGif(":/robber.gif", mb2_alive); loadGif(":/ghost.gif", mb2_dead);
    loadGif(":/banana_alive.gif", bb_alive); loadGif(":/banana_dead.gif", bb_dead);
    loadGif(":/kfc_cat.gif", med_alive_cat);
    loadGif(":/motor_cat.gif", motor_anim);
    loadGif(":/milk_cat.gif", milk_anim);

    //UI背景贴图预加载
    startBackground = QPixmap(":/start_bg.png");
    ruleBackground = QPixmap(":/rule_bg.png");
    selectBackground = QPixmap(":/select_bg.png");
    envBackground = QPixmap(":/env_bg.png");

    trans_1 = QPixmap(":/trans_1.png");
    trans_2 = QPixmap(":/trans_2.png");
    trans_3 = QPixmap(":/trans_3.png");
    trans_4 = QPixmap(":/trans_4.png");

    deadBackground = QPixmap(":/dead_bg.png");
    victoryBackground = QPixmap(":/victory_bg.png");

    pic_skill_i = QPixmap(":/skill_i.png");
    pic_skill_j = QPixmap(":/skill_j.png");
    pic_skill_k = QPixmap(":/skill_k.png");
    pic_skill_l = QPixmap(":/skill_l.png");

    auto removeBlack = [](QPixmap pix) -> QPixmap {
        if (pix.isNull()) return pix;
        QImage img = pix.toImage().convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < img.height(); ++y) {
            QRgb *p = (QRgb *)img.scanLine(y);
            for (int x = 0; x < img.width(); ++x) {
                int r = qRed(p[x]);
                int g = qGreen(p[x]);
                int b = qBlue(p[x]);
                int a = std::max(r, std::max(g, b));
                p[x] = qRgba(r, g, b, a);
            }
        }
        img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        return QPixmap::fromImage(img);
    };
    //技能效果贴图预加载
    pic_skill_i = removeBlack(pic_skill_i);
    pic_skill_j = removeBlack(pic_skill_j);
    pic_skill_k = removeBlack(pic_skill_k);
    pic_skill_l = removeBlack(pic_skill_l);

    bg_lv1 = QPixmap(":/bg_lv1.png");
    bg_lv2 = QPixmap(":/bg_lv2.png");
    bg_lv3 = QPixmap(":/bg_lv3.png");
    bg_lv4 = QPixmap(":/bg_lv4.png");

    QImage heartSheet(":/hearts.gif");
    if (!heartSheet.isNull()) {
        int w = heartSheet.width() / 3, h = heartSheet.height() / 2;
        for (int row = 0; row < 2; row++) for (int col = 0; col < 3; col++)
                med_ready_hearts.append(QPixmap::fromImage(heartSheet.copy(col * w, row * h, w, h)));
    }

    //音频引擎
    deathSound = new QSoundEffect(this);
    deathSound->setSource(QUrl::fromLocalFile(":/cat_dead.wav"));
    deathSound->setVolume(1.0);

    medTriggerSound = new QSoundEffect(this);
    medTriggerSound->setSource(QUrl::fromLocalFile(":/cat_dead.wav"));
    medTriggerSound->setVolume(0.8);

    bgmPlayer = new QSoundEffect(this);
    bgmPlayer->setLoopCount(QSoundEffect::Infinite);
    bgmPlayer->setVolume(0.5);
    changeBGM(":/menu_music.wav");

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Gamewindow::gameLoop);
    timer->start(16);
}
void Gamewindow::changeBGM(QString path) {
    if (bgmPlayer->source() != QUrl::fromLocalFile(path)) {
        bgmPlayer->setSource(QUrl::fromLocalFile(path));
        bgmPlayer->play();
    }
}

// ====== 【游戏主心跳引擎 (每16ms触发)】 ======
void Gamewindow::gameLoop() {
    frameCount++;

    if (gameState != 3) {
        update();
        return;
    }

    if (isVictorious) {
        victoryTimer--;

        if (victoryTimer == 600) {
            changeBGM(":/victory_music.wav");
        }

        if (victoryTimer <= 0) {
            isVictorious = false;
            gameState = 0;
            changeBGM(":/menu_music.wav");
        }
        update();
        return;
    }

    if (isDead) {
        victoryTimer--;
        if (victoryTimer <= 0) {
            isDead = false;
            gameState = 0;
            changeBGM(":/menu_music.wav");
        }
        update();
        return;
    }

    // 利用补间动画(Tweening)实现在矩阵网格中的丝滑移动
    double pStep = 1.0 / 8.0;
    if (visualPx < px) { visualPx += pStep; if (visualPx > px) visualPx = px; }
    else if (visualPx > px) { visualPx -= pStep; if (visualPx < px) visualPx = px; }

    if (visualPy < py) { visualPy += pStep; if (visualPy > py) visualPy = py; }
    else if (visualPy > py) { visualPy -= pStep; if (visualPy < py) visualPy = py; }

    // 怪物阵列的平滑跟进插值
    double mStep = 1.0 / 24.0;
    for(int r=0; r<gameMap.hang; r++) {
        for(int c=0; c<gameMap.lie; c++) {
            if (offsetR[r][c] > 0) { offsetR[r][c] -= mStep; if (offsetR[r][c] < 0) offsetR[r][c] = 0; }
            else if (offsetR[r][c] < 0) { offsetR[r][c] += mStep; if (offsetR[r][c] > 0) offsetR[r][c] = 0; }

            if (offsetC[r][c] > 0) { offsetC[r][c] -= mStep; if (offsetC[r][c] < 0) offsetC[r][c] = 0; }
            else if (offsetC[r][c] < 0) { offsetC[r][c] += mStep; if (offsetC[r][c] > 0) offsetC[r][c] = 0; }
        }
    }

    for (auto it = clickHealHearts.begin(); it != clickHealHearts.end(); ) {
        it->pos.setY(it->pos.y() - it->speedY * 0.4); it->lifetime--;
        if (it->lifetime <= 0 || it->pos.y() < 0) it = clickHealHearts.erase(it); else ++it;
    }

    for (auto it = activeVFXs.begin(); it != activeVFXs.end(); ) {
        it->lifetime--;
        if (it->lifetime <= 0) it = activeVFXs.erase(it); else ++it;
    }

    // 技能 CD 冷却流逝
    for (int i = 0; i < 3; i++) {
        if (skillCooldown[i] > 0) skillCooldown[i]--;
    }

    if (frameCount % 8 == 0) {
        if (!isDead && !isVictorious) {
            int ox = px, oy = py, nx = px, ny = py;

            if (keys[Qt::Key_W] && px > boundTop) nx--;
            else if (keys[Qt::Key_S] && px < boundBottom) nx++;
            else if (keys[Qt::Key_A] && py > boundLeft) ny--;
            else if (keys[Qt::Key_D] && py < boundRight) ny++;

            if ((nx != px || ny != py) && gameMap.data[nx][ny] <= 0 && gameMap.data[nx][ny] != -77.0) {
                gameMap.data[ox][oy] = 0; px = nx; py = ny; gameMap.data[px][py] = 999.0;
            }
        }
    }

    // ====== 【全局AI与游戏规则运行层】 ======
    if (frameCount % 6 == 0) {
        logicFrame++;

        if (!isDead && !isVictorious) {
            if (playerMP < 500.0) playerMP += 2.5; // 自然回蓝

            int mCount = 0, mbCount = 0;
            for(int r=boundTop; r<=boundBottom; r++) {
                for(int c=boundLeft; c<=boundRight; c++) {
                    double v = gameMap.data[r][c];
                    if(v > 0 && v != 999.0 && v != 77.0) {
                        mCount++;
                        int type = (int)std::round((v - std::floor(v)) * 10.0);
                        if(type >= 4 && type <= 5) mbCount++;
                    }
                }
            }

            //杂兵生成控制器
            if (logicFrame % 15 == 0 && mCount < (30 + currentLevel * 10)) {
                int rx = boundTop + rand() % (boundBottom - boundTop + 1);
                int ry = boundLeft + rand() % (boundRight - boundLeft + 1);
                if (gameMap.data[rx][ry] == 0 && !(rx == kfcR && ry == kfcC)) {
                    gameMap.data[rx][ry] = (30.0 + currentLevel * 20.0) + (rand()%3+1) * 0.1;
                    offsetR[rx][ry] = 0; offsetC[rx][ry] = 0;
                }
            }

            //精英怪生成控制器 (Lv.2及以上解锁)
            if (currentLevel >= 2 && logicFrame % 80 == 0 && mbCount < ((currentLevel == 2) ? 4 : 8)) {
                int rx = boundTop + rand() % (boundBottom - boundTop + 1);
                int ry = boundLeft + rand() % (boundRight - boundLeft + 1);
                if (gameMap.data[rx][ry] == 0 && !(rx == kfcR && ry == kfcC)) {
                    gameMap.data[rx][ry] = (150.0 + currentLevel * 50.0) + (rand()%2+4)*0.1;
                    offsetR[rx][ry] = 0; offsetC[rx][ry] = 0;
                }
            }

            //香蕉猫大魔王生成器 (达成Lv.4触发)
            if (currentLevel == 4 && !bossSpawned) {
                int rx = boundTop + rand() % (boundBottom - boundTop + 1);
                int ry = boundLeft + rand() % (boundRight - boundLeft + 1);
                if (gameMap.data[rx][ry] == 0 && !(rx == kfcR && ry == kfcC)) {
                    double bossHp = (currentLevel == 4) ? 99999.0 : (currentLevel * 200.0);
                    gameMap.data[rx][ry] = bossHp + 0.6; // 小数点0.6标记为终极BOSS
                    offsetR[rx][ry] = 0; offsetC[rx][ry] = 0;
                    bossSpawned = true;
                }
            }

            //KFC血包冷却倒计时
            if (gameMap.data[kfcR][kfcC] == -77.0 && medRespawnTimer > 0) {
                medRespawnTimer--; if (medRespawnTimer == 0) gameMap.data[kfcR][kfcC] = 77.0;
            }

            //实例化MonsterAILayer计算下一步小怪走位
            if (logicFrame % 4 == 0) {
                jvzhen old = gameMap; MonsterAILayer ai; jvzhen next = ai.forward(gameMap);
                jvzhen safe(gameMap.hang, gameMap.lie); safe.data[px][py] = 999.0;
                safe.data[kfcR][kfcC] = gameMap.data[kfcR][kfcC];

                QVector<QVector<double>> nextOffR(gameMap.hang, QVector<double>(gameMap.lie, 0.0));
                QVector<QVector<double>> nextOffC(gameMap.hang, QVector<double>(gameMap.lie, 0.0));

                for(int r=0; r<gameMap.hang; r++) {
                    for(int c=0; c<gameMap.lie; c++) {
                        double val = old.data[r][c];
                        if (val > 0 && val != 999.0 && val != 77.0) {
                            bool moved = false;

                            for(int dr=-1; dr<=1 && !moved; dr++) {
                                for(int dc=-1; dc<=1 && !moved; dc++) {
                                    int i = r + dr;
                                    int j = c + dc;
                                    if (i >= boundTop && i <= boundBottom && j >= boundLeft && j <= boundRight) {
                                        if (std::abs(next.data[i][j] - val) < 0.001 && safe.data[i][j] == 0 && !(i==px && j==py)) {
                                            safe.data[i][j] = val;
                                            next.data[i][j] = 0;
                                            moved = true;

                                            nextOffR[i][j] = (r - i) + offsetR[r][c];
                                            nextOffC[i][j] = (c - j) + offsetC[r][c];
                                        }
                                    }
                                }
                            }

                            if (!moved) {
                                if (safe.data[r][c] == 0) {
                                    safe.data[r][c] = val;
                                    nextOffR[r][c] = offsetR[r][c];
                                    nextOffC[r][c] = offsetC[r][c];
                                }
                                else {
                                    bool placed = false;
                                    for(int d=1; d<=2 && !placed; d++) {
                                        for(int i=std::max(boundTop,r-d); i<=std::min(boundBottom,r+d) && !placed; i++) {
                                            for(int j=std::max(boundLeft,c-d); j<=std::min(boundRight,c+d) && !placed; j++) {
                                                if (safe.data[i][j] == 0 && !(i==px && j==py)) {
                                                    safe.data[i][j] = val;
                                                    nextOffR[i][j] = (r - i) + offsetR[r][c];
                                                    nextOffC[i][j] = (c - j) + offsetC[r][c];
                                                    placed = true;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                for(int r=0; r<gameMap.hang; r++) {
                    for(int c=0; c<gameMap.lie; c++) {
                        if(gameMap.data[r][c] < 0 && safe.data[r][c] == 0) {
                            safe.data[r][c] = gameMap.data[r][c];
                            nextOffR[r][c] = offsetR[r][c];
                            nextOffC[r][c] = offsetC[r][c];
                        }
                    }
                }
                gameMap = safe;
                offsetR = nextOffR;
                offsetC = nextOffC;
            }

            // 死亡动画(负数标记)递减消散机制
            for(int r=0; r<gameMap.hang; r++) for(int c=0; c<gameMap.lie; c++)
                    if(gameMap.data[r][c] < 0 && gameMap.data[r][c] != -77.0) {
                        gameMap.data[r][c] -= 1.0; if(gameMap.data[r][c] <= -15.0) gameMap.data[r][c] = 0;
                    }

            //受击检测
            check_collision(gameMap, px, py, playerHP);

            // 判定死亡，触发8秒失败倒计时和专属阵亡音乐
            if (playerHP <= 0) {
                isDead = true;
                victoryTimer = 480;
                if (deathSound) deathSound->play();
                changeBGM(":/wasted.wav");
            }
        }
    }
    update();
}

// ====== 【键盘交互与卷积技能系统】 ======
void Gamewindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) this->close();

    if (gameState != 3) return;
    if (!event->isAutoRepeat()) keys[event->key()] = true;
    if (isDead || isVictorious) return;

    //执行卷积叠加计算并结算击杀与等级提升
    auto triggerProcess = [&](jvzhen damage, bool applyNerf) {
        if (applyNerf) { // 为大范围技能自动乘上平衡削弱系数
            for(int r=0; r<gameMap.hang; r++)
                for(int c=0; c<gameMap.lie; c++)
                    damage.data[r][c] *= 0.35;
        }

        bool kb = false;
        totalKills += apply_damage(gameMap, damage, kb);

        //触发胜利结算
        if (kb && currentLevel == 4) {
            isVictorious = true;
            victoryTimer = 960; // 16秒倒计时 (16*60)
            changeBGM(":/banana_cry.wav");
            return;
        }

        //等级提升判定
        int old = currentLevel;
        if (totalKills >= 150) currentLevel = 4; else if (totalKills >= 80) currentLevel = 3; else if (totalKills >= 30) currentLevel = 2;

        // 升级奖励与换场清屏逻辑
        if (currentLevel > old) {
            shakeFrames = 20;
            bossSpawned = false;

            playerMaxHP += 100.0;
            playerHP = playerMaxHP;

            for(int r = 0; r < gameMap.hang; r++) {
                for(int c = 0; c < gameMap.lie; c++) {
                    if (r != px || c != py) {
                        gameMap.data[r][c] = 0.0;
                        offsetR[r][c] = 0.0;
                        offsetC[r][c] = 0.0;
                    }
                }
            }

            gameMap.data[kfcR][kfcC] = 77.0;
            medRespawnTimer = 0;

            for(int i=0; i<3; i++) skillCooldown[i] = 0;
            activeVFXs.clear();

            // 解锁新技能的连环机制
            if (currentLevel == 2) {
                skillUnlocked[0] = true;
            } else if (currentLevel == 3) {
                skillUnlocked[1] = true;
            } else if (currentLevel == 4) {
                skillUnlocked[2] = true;
                changeBGM(":/boss_epic_music.wav");
            }

            if (!isDesktopMode) {
                gameState = 4;
            }
        }
    };

    //普攻I：以玩家为中心生成九宫格伤害矩阵
    if (event->key() == Qt::Key_I) {
        jvzhen d(gameMap.hang, gameMap.lie);
        for(int r=std::max(0,px-1); r<=std::min(gameMap.hang-1,px+2); r++)
            for(int c=std::max(0,py-1); c<=std::min(gameMap.lie-1,py+2); c++) d.data[r][c] = 25.0;

        activeVFXs.append({0, px, py, 12, 12});
        triggerProcess(d, false);
    }
    //技能J/K/L：利用ConvLayer进行卷积运算
    else if (event->key()==Qt::Key_J && skillUnlocked[0] && playerMP>=25 && skillCooldown[0]<=0) {
        playerMP-=25;
        activeVFXs.append({1, px, py, 45, 45});
        triggerProcess(ConvLayer(skillbox::crosscut()).forward(gameMap), true);
        skillCooldown[0] = 500;
    }
    else if (event->key()==Qt::Key_K && skillUnlocked[1] && playerMP>=45 && skillCooldown[1]<=0) {
        playerMP-=45;
        activeVFXs.append({2, px, py, 45, 45});
        triggerProcess(ConvLayer(skillbox::xslash()).forward(gameMap), true);
        skillCooldown[1] = 500;
    }
    else if (event->key()==Qt::Key_L && skillUnlocked[2] && playerMP>=80 && skillCooldown[2]<=0) {
        playerMP-=80;
        activeVFXs.append({3, px, py, 60, 60});
        triggerProcess(ConvLayer(skillbox::windcrack()).forward(gameMap), true);
        skillCooldown[2] = 500;
    }
}

void Gamewindow::keyReleaseEvent(QKeyEvent *event) {
    if (!event->isAutoRepeat()) keys[event->key()] = false;
}

void Gamewindow::mouseMoveEvent(QMouseEvent *event) {
    hoverButton = 0;
}

// ====== 【鼠标响应与隐形UI判定层】 ======
void Gamewindow::mousePressEvent(QMouseEvent *event) {
    // 状态机 = 0：处理主页菜单点击
    if (gameState == 0) {
        if (isShowingRules) {
            if (rectCloseBtn.contains(event->pos()) || rectReturnBtn.contains(event->pos())) isShowingRules = false;
            return;
        }
        if (rectStartBtn.contains(event->pos())) {
            gameState = 1;
        }
        else if (rectRuleBtn.contains(event->pos())) isShowingRules = true;
        else if (rectExitBtn.contains(event->pos())) this->close();
        return;
    }
    // 状态机 = 1：选人菜单逻辑
    else if (gameState == 1) {
        if (rectPickLeft.contains(event->pos())) {
            selectedCharacter = 1;
            gameState = 2;
        }
        else if (rectPickRight.contains(event->pos())) {
            selectedCharacter = 2;
            gameState = 2;
        }
        return;
    }
    // 状态机 = 2：选择模式并初始化开战数据
    else if (gameState == 2) {
        if (rectEnvNKU.contains(event->pos()) || rectEnvDesktop.contains(event->pos())) {

            isDesktopMode = rectEnvDesktop.contains(event->pos());

            // 动态构建战场矩阵
            gameMap = jvzhen(height() / 50, width() / 50);
            offsetR.assign(gameMap.hang, QVector<double>(gameMap.lie, 0.0));
            offsetC.assign(gameMap.hang, QVector<double>(gameMap.lie, 0.0));
            playerHP = 100.0; playerMaxHP = 100.0; playerMP = 500.0;
            totalKills = 0; currentLevel = 1; bossSpawned = false;
            isDead = false; isVictorious = false;
            clickHealHearts.clear();
            activeVFXs.clear();

            for(int i=0; i<3; i++) skillUnlocked[i] = false;

            // 根据是否是桌宠模式，圈定活动边界
            if (isDesktopMode) {
                boundTop = 0;
                boundBottom = gameMap.hang - 1;
                boundLeft = 0;
                boundRight = gameMap.lie - 1;
                kfcR = gameMap.hang - 3;
                kfcC = gameMap.lie - 3;
            } else {
                boundTop = gameMap.hang * 0.28;
                boundBottom = gameMap.hang * 0.95;
                boundLeft = gameMap.lie * 0.04;
                boundRight = gameMap.lie * 0.96;
                kfcR = gameMap.hang * 0.82;
                kfcC = gameMap.lie * 0.88;
            }

            px = boundTop + (boundBottom - boundTop) / 2;
            py = boundLeft + (boundRight - boundLeft) / 2;
            gameMap.data[px][py] = 999.0;
            visualPx = px; visualPy = py;

            gameMap.data[kfcR][kfcC] = 77.0;
            medRespawnTimer = 0;

            changeBGM(":/battle_music.wav");

            if (isDesktopMode) {
                gameState = 3;
            } else {
                gameState = 4;
            }
        }
        return;
    }
    // 状态机 = 4：过场动画点击继续
    else if (gameState == 4) {
        gameState = 3;
        return;
    }

    if (isDead) {
        isDead = false;
        gameState = 0;
        changeBGM(":/menu_music.wav");
        return;
    }

    if (isVictorious) return;

    // 战斗状态：处理点击肯德基猫猫加血的判定
    if (QRect(kfcC*50-15, kfcR*50-15, 80, 80).contains(event->pos()) && gameMap.data[kfcR][kfcC] == 77.0) {
        double healAmount = playerMaxHP * 0.3;
        playerHP = std::min(playerMaxHP, playerHP + healAmount);
        if(medTriggerSound) medTriggerSound->play();
        gameMap.data[kfcR][kfcC] = -77.0; medRespawnTimer = 250;

        for (int i=0; i<50; i++) {
            clickHealHearts.append({
                QPointF(rand()%width(), height()*0.5 + rand()%(height()/2)),
                (double)(rand()%20+10)/10.0,
                rand()%50+50,
                100
            });
        }
        update();
    }
}

// ====== 【核心渲染】 ======
void Gamewindow::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 【渲染分支 0】：主菜单与规则界面的静态绘制
    if (gameState == 0) {
        if (isShowingRules) {
            if (!ruleBackground.isNull()) painter.drawPixmap(rect(), ruleBackground);
            else {
                painter.fillRect(rect(), QColor(0, 0, 0, 220));
                painter.setPen(Qt::white); painter.setFont(QFont("Microsoft YaHei", 24, QFont::Bold));
                painter.drawText(rect(), Qt::AlignCenter, "未找到 rule_bg.png");
            }
            int closeW = width() * 0.04; int closeH = height() * 0.07;
            int closeX = width() * 0.8; int closeY = height() * 0.065;
            rectCloseBtn = QRect(closeX, closeY, closeW, closeH);

            int retW = width() * 0.20; int retH = height() * 0.08;
            int retX = width() / 2 - retW / 2; int retY = height() * 0.89;
            rectReturnBtn = QRect(retX, retY, retW, retH);
            return;
        }

        if (!startBackground.isNull()) painter.drawPixmap(rect(), startBackground);
        else painter.fillRect(rect(), QColor(20, 10, 30));

        int btnW = width() * 0.40;
        int btnH = height() * 0.10;
        int btnX = width() / 2 - btnW / 2;

        rectStartBtn = QRect(btnX, height() * 0.49, btnW, btnH);
        rectRuleBtn  = QRect(btnX, height() * 0.65, btnW, btnH);
        rectExitBtn  = QRect(btnX, height() * 0.81, btnW, btnH);
        return;
    }

    // 【渲染分支 1】：角色选择界面及猫猫动图展示
    if (gameState == 1) {
        if (!selectBackground.isNull()) painter.drawPixmap(rect(), selectBackground);
        else painter.fillRect(rect(), QColor(40, 10, 50));

        int gifW = width() * 0.21;
        int gifH = height() * 0.20;
        int gifY = height() * 0.23;
        int leftGifX = width() * 0.21;
        int rightGifX = width() * 0.58;

        if (!motor_anim.isEmpty()) {
            painter.drawPixmap(leftGifX, gifY, gifW, gifH, motor_anim[(frameCount/4) % motor_anim.size()]);
        }
        if (!milk_anim.isEmpty()) {
            painter.drawPixmap(rightGifX, gifY, gifW, gifH, milk_anim[(frameCount/4) % milk_anim.size()]);
        }

        int pickW = width() * 0.175;
        int pickH = height() * 0.085;
        int pickY = height() * 0.875;
        int pickLeftX = width() * 0.225;
        int pickRightX = width() * 0.595;

        rectPickLeft = QRect(pickLeftX, pickY, pickW, pickH);
        rectPickRight = QRect(pickRightX, pickY, pickW, pickH);

        return;
    }

    // 【渲染分支 2】：战斗模式选择界面
    if (gameState == 2) {
        if (!envBackground.isNull()) painter.drawPixmap(rect(), envBackground);
        else {
            painter.fillRect(rect(), QColor(10, 5, 20));
            painter.setPen(Qt::white); painter.drawText(rect(), Qt::AlignCenter, "找不到 env_bg.png");
        }

        int envW = width() * 0.40;
        int envH = height() * 0.12;
        int envX = width() / 2 - envW / 2;

        rectEnvNKU = QRect(envX, height() * 0.38, envW, envH);
        rectEnvDesktop = QRect(envX, height() * 0.55, envW, envH);

        return;
    }

    // 【渲染分支 4】：剧情过渡/升级转场立绘渲染
    if (gameState == 4) {
        QPixmap* transBg = nullptr;
        if (currentLevel == 1) transBg = &trans_1;
        else if (currentLevel == 2) transBg = &trans_2;
        else if (currentLevel == 3) transBg = &trans_3;
        else if (currentLevel == 4) transBg = &trans_4;

        if (transBg && !transBg->isNull()) {
            painter.drawPixmap(rect(), *transBg);
        } else {
            painter.fillRect(rect(), QColor(10, 5, 20));
            painter.setPen(Qt::white);
            painter.setFont(QFont("Microsoft YaHei", 24, QFont::Bold));
            painter.drawText(rect(), Qt::AlignCenter, QString("找不到 trans_%1.png 资源\n请点击屏幕继续！").arg(currentLevel));
        }
        return;
    }

    // 【特效渲染】：死亡界面的血红滤镜
    if (isDead) {
        if (!deadBackground.isNull()) {
            painter.drawPixmap(rect(), deadBackground);
        } else {
            painter.fillRect(rect(), QColor(150, 0, 0, 180));
            painter.setPen(Qt::white); painter.setFont(QFont("Arial", 80, QFont::Bold));
            painter.drawText(rect(), Qt::AlignCenter, "DEFEATED...\n(CLICK TO RETURN)");
        }

        painter.setPen(QColor(255, 255, 255, 180));
        painter.setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
        int secondsLeft = victoryTimer / 60;
        painter.drawText(rect().adjusted(0, 0, 0, -20), Qt::AlignBottom | Qt::AlignHCenter,
                         QString("%1秒后自动返回首页 (点击屏幕提前返回)...").arg(secondsLeft));
        return;
    }

    // --- 【第二阶段：剩下 10 秒（<=600）时展示狂欢结算大绿屏】 ---
    if (isVictorious && victoryTimer <= 600) {
        if (!victoryBackground.isNull()) {
            painter.drawPixmap(rect(), victoryBackground);
        } else {
            painter.fillRect(rect(), QColor(0, 100, 0, 180));
            painter.setPen(Qt::white); painter.setFont(QFont("Arial", 80, QFont::Bold));
            painter.drawText(rect(), Qt::AlignCenter, "VICTORY!\nBANANA CAT SAVED!");
        }

        painter.setPen(QColor(255, 255, 255, 180));
        painter.setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
        int secondsLeft = victoryTimer / 60;
        painter.drawText(rect().adjusted(0, 0, 0, -20), Qt::AlignBottom | Qt::AlignHCenter,
                         QString("游戏通关！%1秒后自动返回首页...").arg(secondsLeft));
        return;
    }

    // 【环境渲染】：判断桌宠模式(纯透明)还是常规沉浸模式
    if (isDesktopMode) {
        painter.fillRect(rect(), QColor(0, 0, 0, 1));
    } else {
        QPixmap* currentBg = nullptr;
        if (currentLevel == 1) currentBg = &bg_lv1;
        else if (currentLevel == 2) currentBg = &bg_lv2;
        else if (currentLevel == 3) currentBg = &bg_lv3;
        else if (currentLevel == 4) currentBg = &bg_lv4;

        if (currentBg && !currentBg->isNull()) painter.drawPixmap(rect(), *currentBg);
        else painter.fillRect(rect(), QColor(25, 30, 25));
    }

    // 屏幕震动效果补偿
    if (shakeFrames > 0) { painter.translate((rand()%21)-10, (rand()%21)-10); shakeFrames--; }

    int side = 50; // 标准矩阵网格边长
    int f = logicFrame / 2;

    // 【核心战场扫描绘制】：遍历矩阵，解包数据进行动态呈现
    for (int r = boundTop; r <= boundBottom; r++) {
        for (int c = boundLeft; c <= boundRight; c++) {
            double v = gameMap.data[r][c];

            double smoothR = r + offsetR[r][c];
            double smoothC = c + offsetC[r][c];
            int x = std::round(smoothC * side);
            int y = std::round(smoothR * side);

            // 绘制主角坐标 (999.0)
            if (v == 999.0) {
                double drawX = visualPy * side;
                double drawY = visualPx * side;

                int sz = 20 + (currentLevel - 1) * 20;

                QPixmap currentAvatar;
                if (selectedCharacter == 1 && !motor_anim.isEmpty()) {
                    currentAvatar = motor_anim[(frameCount/4) % motor_anim.size()];
                } else if (selectedCharacter == 2 && !milk_anim.isEmpty()) {
                    currentAvatar = milk_anim[(frameCount/4) % milk_anim.size()];
                }

                if (!currentAvatar.isNull()) {
                    painter.drawPixmap(drawX-sz/2, drawY-sz/2, side+sz, side+sz, currentAvatar);
                }
                else {
                    painter.setBrush(QColor(0, 160, 255)); painter.setPen(Qt::NoPen);
                    painter.drawRoundedRect(drawX-sz/2+5, drawY-sz/2+5, side+sz-10, side+sz-10, 10, 10);
                    painter.setPen(Qt::white); painter.setFont(QFont("Arial", 10, QFont::Bold));
                    painter.drawText(drawX-sz/2, drawY-sz/2, side+sz, side+sz, Qt::AlignCenter, QString("Lv.%1").arg(currentLevel));
                }
            } else if (v != 0) {
                // 绘制特殊建筑：KFC回血猫站 (77.0)
                if (v == 77.0 || v == -77.0) {
                    if (!med_alive_cat.isEmpty()) painter.drawPixmap(x-10, y-10, side+50, side+50, med_alive_cat[0]);

                    if (v == 77.0) {
                        if (!med_ready_hearts.isEmpty())
                            painter.drawPixmap(x+(side-30)/2, y-40-5*sin(frameCount/18.0), 30, 30, med_ready_hearts[(frameCount/3)%med_ready_hearts.size()]);

                        painter.setFont(QFont("Microsoft YaHei", 15, QFont::Bold));
                        painter.setPen(QColor(255, 0, 100));
                        painter.drawText(x - 30, y - 60 , 170, 20, Qt::AlignCenter, "点我回血！");
                    } else {
                        painter.setFont(QFont("Microsoft YaHei", 15, QFont::Bold));
                        painter.setPen(QColor(255, 150, 50));
                        painter.drawText(x - 30, y - 60, 170, 20, Qt::AlignCenter, QString("制作血包中(%1s)").arg(medRespawnTimer/10));
                    }
                } else {
                    // 数据解包：解码怪物种类与当前血量
                    double raw_v = std::abs(v);
                    int t = (int)std::round((raw_v - std::floor(raw_v)) * 10.0);
                    t = std::max(1, std::min(6, t));

                    // 怪物模型尺寸缩放计算
                    int ex = 25;
                    if (t <= 3) ex = 30;
                    else if (t <= 5) ex = 80;
                    else if (t == 6) ex = 180;

                    int dx = x - ex/2, dy = y - ex/2, ds = side + ex;

                    // 绘制活体怪物与顶部血条
                    if (v > 0) {
                        QVector<QPixmap>* vms[] = {&m1_alive,&m2_alive,&m3_alive,&mb1_alive,&mb2_alive,&bb_alive};
                        if (!vms[t-1]->isEmpty()) {
                            painter.drawPixmap(dx, dy, ds, ds, vms[t-1]->at(f % vms[t-1]->size()));
                        } else {
                            painter.setBrush(QColor(255, 0, 255, 200)); painter.setPen(Qt::NoPen);
                            painter.drawRect(x, y, side, side);
                            painter.setPen(Qt::white); painter.setFont(QFont("Arial", 9, QFont::Bold));
                            painter.drawText(x, y, side, side, Qt::AlignCenter, QString("图%1丢失").arg(t));
                        }

                        double currentHp = std::floor(v);
                        double maxHp = currentHp;

                        if (t <= 3) maxHp = std::max(currentHp, 30.0 + currentLevel * 20.0);
                        else if (t <= 5) maxHp = std::max(currentHp, 150.0 + currentLevel * 50.0);
                        else if (t == 6) maxHp = std::max(currentHp, (currentLevel == 4) ?99999.0 : (currentLevel * 200.0));

                        double hpPercent = currentHp / maxHp;
                        if (hpPercent > 1.0) hpPercent = 1.0;

                        int barW = ds * 0.6;
                        int barH = (t == 6) ? 8 : 5;
                        int barX = dx + (ds - barW) / 2;
                        int barY = dy - 5;

                        painter.setBrush(QColor(0, 0, 0, 200));
                        painter.setPen(Qt::NoPen);
                        painter.drawRect(barX, barY, barW, barH);

                        QColor hpColor = QColor(0, 255, 0);
                        if (t >= 4 && t <= 5) hpColor = QColor(255, 150, 0);
                        else if (t == 6) hpColor = QColor(255, 50, 50);

                        painter.setBrush(hpColor);
                        painter.drawRect(barX, barY, barW * hpPercent, barH);

                        painter.setPen(Qt::white);
                        painter.setFont(QFont("Arial", 8, QFont::Bold));
                        painter.drawText(barX, barY - 12, barW, 10, Qt::AlignHCenter | Qt::AlignBottom, QString::number((int)currentHp));
                    }
                    else {
                        // 绘制死亡特效：负数标记时执行逐渐透明上升的灵魂飞升计算
                        QVector<QPixmap>* vmd[] = {&m1_dead,&m2_dead,&m3_dead,&mb1_dead,&mb2_dead,&bb_dead};
                        if (!vmd[t-1]->isEmpty()) {
                            if (t == 6) continue;
                            double progress = std::min(1.0, std::max(0.0, (raw_v - 1.0) / 14.0));
                            double alpha = 1.0 - progress;
                            painter.setOpacity(alpha);

                            int ghostSize = 25 + ex / 3;
                            int floatUp = (int)(progress * (20.0 + ex / 4.0));

                            int ghostX = x + (side - ghostSize) / 2;
                            int ghostY = y + (side - ghostSize) / 2 - floatUp;

                            painter.drawPixmap(ghostX, ghostY, ghostSize, ghostSize, vmd[t-1]->at(f % vmd[t-1]->size()));
                            painter.setOpacity(1.0);
                        }
                    }
                }
            }
        }
    }

    // 【渲染分支】: 利用 Screen (滤色) 模式叠加技能特效阵列，增强光污染质感
    painter.setCompositionMode(QPainter::CompositionMode_Screen);
    for (const auto& vfx : activeVFXs) {
        double drawX = vfx.c * side;
        double drawY = vfx.r * side;

        QPixmap* tex = nullptr;
        double gridRange = 0;
        double expand = 1.0;

        if (vfx.type == 0) {
            gridRange = 4.0; tex = &pic_skill_i;
            expand = 1.1;
        } else if (vfx.type == 1) {
            gridRange = 3.0; tex = &pic_skill_j;
            expand = 2.0;
        } else if (vfx.type == 2) {
            gridRange = 3.0; tex = &pic_skill_k;
            expand = 2.0;
        } else if (vfx.type == 3) {
            gridRange = 5.0; tex = &pic_skill_l;
            expand = 1.4;
        }

        if (tex && !tex->isNull()) {
            double drawRange = gridRange * expand;
            int sz = drawRange * side;

            double offsetX = (0.5 - drawRange / 2.0) * side;
            double offsetY = offsetX;

            double lifeRatio = (double)vfx.lifetime / vfx.maxLifetime;
            double alpha = (lifeRatio > 0.5) ? 1.0 : (lifeRatio * 2.0);

            if (vfx.type == 0) {
                alpha *= 0.8;
            }

            painter.setOpacity(alpha);
            painter.drawPixmap(drawX + offsetX, drawY + offsetY, sz, sz, *tex);
        }
    }
    painter.setOpacity(1.0);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // --- 【第一阶段：前 6 秒（>600）屏幕变暗，香蕉猫伴随哭声缓慢变大】 ---
    if (isVictorious && victoryTimer > 600) {
        int bx = -1, by = -1;
        for(int r = boundTop; r <= boundBottom; r++) {
            for(int c = boundLeft; c <= boundRight; c++) {
                double v = gameMap.data[r][c];
                if (v < 0 && v != -77.0) {
                    int t = (int)std::round((std::abs(v) - std::floor(std::abs(v))) * 10.0);
                    if (t == 6) {
                        bx = c; by = r;
                        break;
                    }
                }
            }
            if (bx != -1) break;
        }

        if (bx != -1 && !bb_dead.isEmpty()) {
            double smoothR = by + offsetR[by][bx];
            double smoothC = bx + offsetC[by][bx];
            int x = std::round(smoothC * side);
            int y = std::round(smoothR * side);

            int baseSize = side + 180;

            // 匹配 6 秒（360帧）的放大计算
            int elapsedFrames = 960 - victoryTimer;

            int dimAlpha = std::min(200, (int)(200.0 * (elapsedFrames / 360.0)));
            painter.fillRect(rect(), QColor(0, 0, 0, dimAlpha)); // 压制暗角阴影遮罩

            double expandRatio = 1.0 + 2.5 * (elapsedFrames / 360.0);
            int drawSize = baseSize * expandRatio;

            int drawX = x + side / 2 - drawSize / 2;
            int drawY = y + side / 2 - drawSize / 2;

            painter.drawPixmap(drawX, drawY, drawSize, drawSize, bb_dead[(frameCount / 8) % bb_dead.size()]);
        }
    }
    // ------------------------------------------------------------------------

    // 【粒子管线】：最上层绘制治愈爱心的上浮动画
    if (!clickHealHearts.isEmpty() && !med_ready_hearts.isEmpty()) {
        for (const auto& h : clickHealHearts) {
            double alpha = std::max(0.0, std::min(1.0, (double)h.lifetime / h.initialLifetime));
            painter.setOpacity(alpha);
            painter.drawPixmap(h.pos.x(), h.pos.y(), 35, 35, med_ready_hearts[0]);
        }
        painter.setOpacity(1.0);
    }

    // 【UI 控件层】：在画面最顶部绘制数值面板及技能冷却条
    painter.fillRect(15, 15, 450, 35, QColor(0, 0, 0, 200));
    painter.setPen(Qt::white); painter.setFont(QFont("Microsoft YaHei", 11, QFont::Bold));

    QString medStr = (gameMap.data[kfcR][kfcC] == -77.0) ? QString("冷却(%1s)").arg(medRespawnTimer/10) : "就绪(点猫)";

    QString modeStr = isDesktopMode ? "[桌宠模式]" : QString("Lv.%1").arg(currentLevel);
    painter.drawText(25, 38, QString("%1 | HP:%2/%3 | MP:%4 | 杀敌:%5 | MED:%6")
                                 .arg(modeStr).arg((int)playerHP).arg((int)playerMaxHP).arg((int)playerMP).arg(totalKills).arg(medStr));

    int startY = 80;
    int barW = 130;
    int barH = 16;
    int rightX = width() - barW - 20;

    QString skillNames[] = {"[J] Crosscut", "[K] X-Slash", "[L] Windcrack"};
    for (int i = 0; i < 3; i++) {
        if (skillUnlocked[i]) {
            int currentY = startY + i * 65;
            painter.setPen(Qt::white);
            painter.setFont(QFont("Arial", 10, QFont::Bold));

            if (skillCooldown[i] > 0) {
                double cdSec = skillCooldown[i] * 0.016;
                painter.drawText(rightX, currentY - 5, QString("%1: %2s").arg(skillNames[i]).arg(cdSec, 0, 'f', 1));

                painter.setBrush(QColor(50, 50, 50, 200));
                painter.setPen(Qt::NoPen);
                painter.drawRect(rightX, currentY, barW, barH);

                double pct = (double)skillCooldown[i] / 500.0;
                painter.setBrush(QColor(255, 100, 50, 200));
                painter.drawRect(rightX, currentY, barW * pct, barH);
            } else {
                painter.drawText(rightX, currentY - 5, QString("%1: READY").arg(skillNames[i]));

                painter.setBrush(QColor(50, 50, 50, 200));
                painter.setPen(Qt::NoPen);
                painter.drawRect(rightX, currentY, barW, barH);

                painter.setBrush(QColor(0, 200, 255, 200));
                painter.drawRect(rightX, currentY, barW, barH);
            }
        }
    }
}
