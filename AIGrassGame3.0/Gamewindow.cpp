#include "Gamewindow.h"
#include <ctime>
#include <QImageReader>
#include <algorithm>
#include <cmath>

Gamewindow::Gamewindow(QWidget *parent) : QWidget(parent), gameMap(1, 1) {
    srand(time(NULL));
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    setFixedSize(screenRect.width(), screenRect.height());
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    gameMap = jvzhen(screenRect.height() / 50, screenRect.width() / 50);
    px = gameMap.hang / 2; py = gameMap.lie / 2;
    gameMap.data[px][py] = 999.0;

    visualPx = px; visualPy = py;
    logicFrame = 0;

    offsetR.assign(gameMap.hang, QVector<double>(gameMap.lie, 0.0));
    offsetC.assign(gameMap.hang, QVector<double>(gameMap.lie, 0.0));

    playerMaxHP = 100.0;
    playerHP = playerMaxHP;
    playerMP = 500.0;

    frameCount = 0; shakeFrames = 0;
    totalKills = 0; currentLevel = 1; bossSpawned = false; isDead = false;
    isSelectingSkill = false; isVictorious = false;
    for(int i=0; i<3; i++) {
        skillUnlocked[i] = false;
        skillCooldown[i] = 0;
    }

    medRespawnTimer = 0;
    gameMap.data[gameMap.hang-4][gameMap.lie-4] = 77.0;

    auto loadGif = [](QString path, QVector<QPixmap>& frames) {
        QImageReader reader(path);
        while (reader.canRead()) {
            QImage img = reader.read();
            if (!img.isNull()) frames.append(QPixmap::fromImage(img));
        }
    };

    loadGif(":/potato_cat.gif", m1_alive); loadGif(":/ghost.gif", m1_dead);
    loadGif(":/beecat.gif", m2_alive); loadGif(":/ghost.gif", m2_dead);
    loadGif(":/gun_cat.gif", m3_alive); loadGif(":/ghost.gif", m3_dead);
    loadGif(":/Hitler.gif", mb1_alive); loadGif(":/ghost.gif", mb1_dead);
    loadGif(":/robber.gif", mb2_alive); loadGif(":/ghost.gif", mb2_dead);
    loadGif(":/banana_alive.gif", bb_alive); loadGif(":/banana_dead.gif", bb_dead);

    player_lv1 = QPixmap(":/police_cat.gif"); player_lv2 = QPixmap(":/police_cat.gif");
    player_lv3 = QPixmap(":/police_cat.gif"); player_lv4 = QPixmap(":/police_cat.gif");

    loadGif(":/kfc_cat.gif", med_alive_cat);

    QImage heartSheet(":/hearts.gif");
    if (!heartSheet.isNull()) {
        int w = heartSheet.width() / 3, h = heartSheet.height() / 2;
        for (int row = 0; row < 2; row++) for (int col = 0; col < 3; col++)
                med_ready_hearts.append(QPixmap::fromImage(heartSheet.copy(col * w, row * h, w, h)));
    }

    deathSound = new QSoundEffect(this);
    deathSound->setSource(QUrl::fromLocalFile(":/cat_dead.wav"));
    deathSound->setVolume(1.0);

    medTriggerSound = new QSoundEffect(this);
    medTriggerSound->setSource(QUrl::fromLocalFile(":/cat_dead.wav"));
    medTriggerSound->setVolume(0.8);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Gamewindow::gameLoop);
    timer->start(16);
}

void Gamewindow::gameLoop() {
    double pStep = 1.0 / 8.0;
    if (visualPx < px) { visualPx += pStep; if (visualPx > px) visualPx = px; }
    else if (visualPx > px) { visualPx -= pStep; if (visualPx < px) visualPx = px; }

    if (visualPy < py) { visualPy += pStep; if (visualPy > py) visualPy = py; }
    else if (visualPy > py) { visualPy -= pStep; if (visualPy < py) visualPy = py; }

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

    for (int i = 0; i < 3; i++) {
        if (skillCooldown[i] > 0) skillCooldown[i]--;
    }

    frameCount++;

    if (frameCount % 8 == 0) {
        if (!isDead && !isVictorious && !isSelectingSkill) {
            int ox = px, oy = py, nx = px, ny = py;

            if (keys[Qt::Key_W] && px > 0) nx--;
            else if (keys[Qt::Key_S] && px < gameMap.hang - 1) nx++;
            else if (keys[Qt::Key_A] && py > 0) ny--;
            else if (keys[Qt::Key_D] && py < gameMap.lie - 1) ny++;

            if ((nx != px || ny != py) && gameMap.data[nx][ny] <= 0 && gameMap.data[nx][ny] != -77.0) {
                gameMap.data[ox][oy] = 0; px = nx; py = ny; gameMap.data[px][py] = 999.0;
            }
        }
    }

    if (frameCount % 6 == 0) {
        logicFrame++;

        if (!isDead && !isVictorious && !isSelectingSkill) {
            if (playerMP < 500.0) playerMP += 2.5;

            int mCount = 0, mbCount = 0;
            for(int r=0; r<gameMap.hang; r++) {
                for(int c=0; c<gameMap.lie; c++) {
                    double v = gameMap.data[r][c];
                    if(v > 0 && v < 999 && v != 77.0) {
                        mCount++;
                        int type = (int)std::round((v - std::floor(v)) * 10.0);
                        if(type >= 4 && type <= 5) mbCount++;
                    }
                }
            }

            if (logicFrame % 15 == 0 && mCount < (30 + currentLevel * 10)) {
                int rx = rand() % gameMap.hang, ry = rand() % gameMap.lie;
                if (gameMap.data[rx][ry] == 0) {
                    gameMap.data[rx][ry] = (30.0 + currentLevel * 20.0) + (rand()%3+1) * 0.1;
                    offsetR[rx][ry] = 0; offsetC[rx][ry] = 0;
                }
            }

            int maxMb = (currentLevel == 2) ? 4 : (currentLevel >= 3 ? 8 : 0);
            if (currentLevel >= 2 && logicFrame % 80 == 0 && mbCount < maxMb) {
                int rx = rand() % gameMap.hang, ry = rand() % gameMap.lie;
                if (gameMap.data[rx][ry] == 0) {
                    gameMap.data[rx][ry] = (150.0 + currentLevel * 50.0) + (rand()%2+4)*0.1;
                    offsetR[rx][ry] = 0; offsetC[rx][ry] = 0;
                }
            }

            if (currentLevel == 4 && !bossSpawned) {
                int rx = rand() % gameMap.hang, ry = rand() % gameMap.lie;
                if (gameMap.data[rx][ry] == 0) {
                    double bossHp = (currentLevel == 4) ? 950.0 : (currentLevel * 200.0);
                    gameMap.data[rx][ry] = bossHp + 0.6;
                    offsetR[rx][ry] = 0; offsetC[rx][ry] = 0;
                    bossSpawned = true;
                }
            }

            int medR = gameMap.hang - 4, medC = gameMap.lie - 4;
            if (gameMap.data[medR][medC] == -77.0 && medRespawnTimer > 0) {
                medRespawnTimer--; if (medRespawnTimer == 0) gameMap.data[medR][medC] = 77.0;
            }

            if (logicFrame % 4 == 0) {
                jvzhen old = gameMap; MonsterAILayer ai; jvzhen next = ai.forward(gameMap);
                jvzhen safe(gameMap.hang, gameMap.lie); safe.data[px][py] = 999.0;
                safe.data[medR][medC] = gameMap.data[medR][medC];

                QVector<QVector<double>> nextOffR(gameMap.hang, QVector<double>(gameMap.lie, 0.0));
                QVector<QVector<double>> nextOffC(gameMap.hang, QVector<double>(gameMap.lie, 0.0));

                for(int r=0; r<gameMap.hang; r++) {
                    for(int c=0; c<gameMap.lie; c++) {
                        double val = old.data[r][c];
                        if (val > 0 && val < 999.0 && val != 77.0) {
                            bool moved = false;

                            for(int dr=-1; dr<=1 && !moved; dr++) {
                                for(int dc=-1; dc<=1 && !moved; dc++) {
                                    int i = r + dr;
                                    int j = c + dc;
                                    if (i >= 0 && i < gameMap.hang && j >= 0 && j < gameMap.lie) {
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
                                        for(int i=std::max(0,r-d); i<=std::min(gameMap.hang-1,r+d) && !placed; i++) {
                                            for(int j=std::max(0,c-d); j<=std::min(gameMap.lie-1,c+d) && !placed; j++) {
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

            for(int r=0; r<gameMap.hang; r++) for(int c=0; c<gameMap.lie; c++)
                    if(gameMap.data[r][c] < 0 && gameMap.data[r][c] != -77.0) {
                        gameMap.data[r][c] -= 1.0; if(gameMap.data[r][c] <= -15.0) gameMap.data[r][c] = 0;
                    }

            check_collision(gameMap, px, py, playerHP);
            if (playerHP <= 0) isDead = true;
        }
    }

    update();
}

void Gamewindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) this->close();

    if (!event->isAutoRepeat()) {
        keys[event->key()] = true;
    }

    if (isDead || isVictorious) return;

    // --- 【修复的核心 Bug 1】：直接按键绑定对应技能！ ---
    if (isSelectingSkill) {
        int idx = -1;
        if (event->key() == Qt::Key_J && !skillUnlocked[0]) idx = 0;
        else if (event->key() == Qt::Key_K && !skillUnlocked[1]) idx = 1;
        else if (event->key() == Qt::Key_L && !skillUnlocked[2]) idx = 2;

        if (idx != -1) {
            skillUnlocked[idx] = true;
            isSelectingSkill = false;
        }
        return;
    }

    auto triggerProcess = [&](jvzhen damage, bool applyNerf) {
        if (applyNerf) {
            for(int r=0; r<gameMap.hang; r++)
                for(int c=0; c<gameMap.lie; c++)
                    damage.data[r][c] *= 0.35;
        }

        bool kb = false;
        totalKills += apply_damage(gameMap, damage, kb);

        if (kb && currentLevel == 4) { isVictorious = true; return; }

        int old = currentLevel;
        if (totalKills >= 150) currentLevel = 4; else if (totalKills >= 80) currentLevel = 3; else if (totalKills >= 30) currentLevel = 2;

        if (currentLevel > old) {
            shakeFrames = 20;
            bossSpawned = false;

            playerMaxHP += 100.0;
            playerHP = playerMaxHP;

            int medR = gameMap.hang - 4;
            int medC = gameMap.lie - 4;
            gameMap.data[medR][medC] = 77.0;
            medRespawnTimer = 0;

            for(int i=0; i<3; i++) skillCooldown[i] = 0;

            // --- 【修复的核心 Bug 2】：第 4 关自动解锁剩下技能，不再进入选技能黑屏！ ---
            if (currentLevel < 4) {
                isSelectingSkill = true;
            } else {
                for(int i=0; i<3; i++) skillUnlocked[i] = true;
                isSelectingSkill = false;
            }
        }
    };

    if (event->key() == Qt::Key_I) {
        jvzhen d(gameMap.hang, gameMap.lie);
        for(int r=std::max(0,px-1); r<=std::min(gameMap.hang-1,px+2); r++)
            for(int c=std::max(0,py-1); c<=std::min(gameMap.lie-1,py+2); c++) d.data[r][c] = 25.0;
        triggerProcess(d, false);
    }
    else if (event->key()==Qt::Key_J && skillUnlocked[0] && playerMP>=25 && skillCooldown[0]<=0) {
        playerMP-=25; triggerProcess(ConvLayer(skillbox::crosscut()).forward(gameMap), true); skillCooldown[0] = 500;
    }
    else if (event->key()==Qt::Key_K && skillUnlocked[1] && playerMP>=45 && skillCooldown[1]<=0) {
        playerMP-=45; triggerProcess(ConvLayer(skillbox::xslash()).forward(gameMap), true); skillCooldown[1] = 500;
    }
    else if (event->key()==Qt::Key_L && skillUnlocked[2] && playerMP>=80 && skillCooldown[2]<=0) {
        playerMP-=80; triggerProcess(ConvLayer(skillbox::windcrack()).forward(gameMap), true); skillCooldown[2] = 500;
    }
}

void Gamewindow::keyReleaseEvent(QKeyEvent *event) {
    if (!event->isAutoRepeat()) {
        keys[event->key()] = false;
    }
}

void Gamewindow::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (shakeFrames > 0) { painter.translate((rand()%21)-10, (rand()%21)-10); shakeFrames--; }

    painter.fillRect(15, 15, 580, 35, QColor(0, 0, 0, 200));
    painter.setPen(Qt::white); painter.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
    QString medStr = (gameMap.data[gameMap.hang-4][gameMap.lie-4] == -77.0) ? QString("冷却(%1s)").arg(medRespawnTimer/10) : "就绪(点猫)";

    painter.drawText(25, 38, QString("HP:%1/%2  MP:%3  |  杀敌:%4  |  Lv.%5  |  MED:%6")
                                 .arg((int)playerHP).arg((int)playerMaxHP).arg((int)playerMP).arg(totalKills).arg(currentLevel).arg(medStr));

    // --- 【修复UI文本】：明确提示玩家按下对应字母键 ---
    if (isSelectingSkill) {
        painter.fillRect(rect(), QColor(0, 0, 0, 220));
        painter.setPen(Qt::yellow); painter.setFont(QFont("Arial", 40, QFont::Bold));
        painter.drawText(rect(), Qt::AlignTop | Qt::AlignHCenter, "\nCHOOSE YOUR SKILL");
        painter.setFont(QFont("Arial", 20));

        QString text = "Press the corresponding key to unlock:\n\n";
        if (!skillUnlocked[0]) text += "[J]: Crosscut  (Cost: 25 MP)\n\n";
        if (!skillUnlocked[1]) text += "[K]: X-Slash   (Cost: 45 MP)\n\n";
        if (!skillUnlocked[2]) text += "[L]: Windcrack (Cost: 80 MP)\n\n";

        painter.drawText(rect(), Qt::AlignCenter, text);
        return;
    }

    int side = 50;
    int f = logicFrame / 2;

    for (int r = 0; r < gameMap.hang; r++) {
        for (int c = 0; c < gameMap.lie; c++) {
            double v = gameMap.data[r][c];

            double smoothR = r + offsetR[r][c];
            double smoothC = c + offsetC[r][c];
            int x = std::round(smoothC * side);
            int y = std::round(smoothR * side);

            if (v == 999.0) {
                double drawX = visualPy * side;
                double drawY = visualPx * side;

                int sz = 20 + (currentLevel - 1) * 20;

                QPixmap* av = nullptr;
                if(currentLevel == 1) av = &player_lv1;
                else if(currentLevel == 2) av = &player_lv2;
                else if(currentLevel == 3) av = &player_lv3;
                else av = &player_lv4;

                if (av && !av->isNull()) painter.drawPixmap(drawX-sz/2, drawY-sz/2, side+sz, side+sz, *av);
                else {
                    painter.setBrush(QColor(0, 160, 255)); painter.setPen(Qt::NoPen);
                    painter.drawRoundedRect(drawX-sz/2+5, drawY-sz/2+5, side+sz-10, side+sz-10, 10, 10);
                    painter.setPen(Qt::white); painter.setFont(QFont("Arial", 10, QFont::Bold));
                    painter.drawText(drawX-sz/2, drawY-sz/2, side+sz, side+sz, Qt::AlignCenter, QString("Lv.%1").arg(currentLevel));
                }
            } else if (v != 0) {
                if (v == 77.0 || v == -77.0) {
                    if (!med_alive_cat.isEmpty()) painter.drawPixmap(x-15, y-15, side+30, side+30, med_alive_cat[0]);

                    if (v == 77.0) {
                        if (!med_ready_hearts.isEmpty())
                            painter.drawPixmap(x+(side-30)/2, y-40-5*sin(frameCount/18.0), 30, 30, med_ready_hearts[(frameCount/3)%med_ready_hearts.size()]);

                        painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
                        painter.setPen(QColor(0, 255, 100));
                        painter.drawText(x - 60, y - 50, 170, 20, Qt::AlignCenter, "点我，全图加血！");
                    } else {
                        int currentAlpha = std::min(200, 200 * medRespawnTimer / 250);
                        painter.setBrush(QBrush(QColor(80, 80, 80, currentAlpha)));
                        painter.setPen(Qt::NoPen);
                        painter.drawEllipse(x - 5, y - 5, side + 10, side + 10);

                        painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
                        painter.setPen(QColor(200, 200, 200));
                        painter.drawText(x - 60, y - 25, 170, 20, Qt::AlignCenter, QString("努力准备中(%1s)").arg(medRespawnTimer/10));
                    }
                } else {
                    double raw_v = std::abs(v);
                    int t = (int)std::round((raw_v - std::floor(raw_v)) * 10.0);
                    t = std::max(1, std::min(6, t));

                    int ex = 25;
                    if (t <= 3) ex = 30;
                    else if (t <= 5) ex = 80;
                    else if (t == 6) ex = 180;

                    int dx = x - ex/2, dy = y - ex/2, ds = side + ex;

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
                        else if (t == 6) maxHp = std::max(currentHp, (currentLevel == 4) ? 950.0 : (currentLevel * 200.0));

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
                        QVector<QPixmap>* vmd[] = {&m1_dead,&m2_dead,&m3_dead,&mb1_dead,&mb2_dead,&bb_dead};
                        if (!vmd[t-1]->isEmpty()) {
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

    if (!clickHealHearts.isEmpty() && !med_ready_hearts.isEmpty()) {
        for (const auto& h : clickHealHearts) {
            double alpha = std::max(0.0, std::min(1.0, (double)h.lifetime / h.initialLifetime));
            painter.setOpacity(alpha);
            painter.drawPixmap(h.pos.x(), h.pos.y(), 35, 35, med_ready_hearts[0]);
        }
        painter.setOpacity(1.0);
    }

    // --- 右侧冷却进度条 ---
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

    if (isVictorious) {
        painter.fillRect(rect(), QColor(0, 100, 0, 180));
        painter.setPen(Qt::white); painter.setFont(QFont("Arial", 80, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, "VICTORY!\nBANANA CAT SAVED!");
    }
    if (isDead) {
        painter.fillRect(rect(), QColor(150, 0, 0, 180));
        painter.setPen(Qt::white); painter.setFont(QFont("Arial", 80, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, "DEFEATED...\n(ESC TO QUIT)");
    }
}

void Gamewindow::mousePressEvent(QMouseEvent *event) {
    if (isDead || isVictorious || isSelectingSkill) return;

    int medR = gameMap.hang-4, medC = gameMap.lie-4;
    if (QRect(medC*50-15, medR*50-15, 80, 80).contains(event->pos()) && gameMap.data[medR][medC] == 77.0) {

        double healAmount = playerMaxHP * 0.3;
        playerHP = std::min(playerMaxHP, playerHP + healAmount);

        if(medTriggerSound) medTriggerSound->play();
        gameMap.data[medR][medC] = -77.0; medRespawnTimer = 250;

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
