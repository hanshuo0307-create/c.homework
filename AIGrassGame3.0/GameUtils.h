#ifndef GAMEUTILS_H
#define GAMEUTILS_H

#include "jvzhen.h"
#include <cmath>
#include <algorithm>

inline int apply_damage(jvzhen& map, const jvzhen& damage, bool& killedBoss) {
    int killCount = 0;
    for (int r = 0; r < map.hang; r++) {
        for (int c = 0; c < map.lie; c++) {
            double val = map.data[r][c];
            if (val > 0 && val < 999.0) {
                if (val == 77.0) continue;
                int hp = (int)std::floor(val);
                int type = (int)std::round((val - hp) * 10.0);
                int dmg = (int)damage.data[r][c];
                if (dmg <= 0 && damage.data[r][c] > 0) dmg = 1;

                int remaining = hp - dmg;
                if (remaining <= 0) {
                    map.data[r][c] = -(1.0 + type * 0.1);
                    if (type == 6) killedBoss = true;
                    killCount++;
                } else {
                    map.data[r][c] = (double)remaining + (type * 0.1);
                }
            }
        }
    }
    return killCount;
}

inline void check_collision(const jvzhen& map, int px, int py, double& hp) {
    int dx[] = {-1, 1, 0, 0}, dy[] = {0, 0, -1, 1};
    for (int i = 0; i < 4; i++) {
        int nx = px + dx[i], ny = py + dy[i];
        if (nx >= 0 && nx < map.hang && ny >= 0 && ny < map.lie) {
            double target = map.data[nx][ny];
            if (target > 0 && target < 999.0) {
                if (target == 77.0 || target == -77.0) continue;
                int type = (int)std::round((target - std::floor(target)) * 10.0);
                if (type <= 3) hp -= 0.8;
                else if (type <= 5) hp -= 3.0;
                else if (type == 6) hp -= 8.0;
                if (hp < 0) hp = 0;
            }
        }
    }
}
#endif
