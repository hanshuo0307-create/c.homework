#pragma once
#include "layer.h"

//怪物AI层
class MonsterAILayer : public Layer {
public:
    MonsterAILayer() : Layer("MonsterAI") {}

    //forward函数计算下一帧所有怪物的行动轨迹
    jvzhen forward(jvzhen input) override {
        jvzhen next_map(input.hang, input.lie);
        int px = -1, py = -1;

        //锁定玩家的绝对坐标 (999.0)
        for (int i = 0; i < input.hang; i++) {
            for (int j = 0; j < input.lie; j++) {
                if (input.data[i][j] == 999.0) { px = i; py = j; break; }
            }
        }

        //遍历所有怪物，计算趋附向量与碰撞
        for (int i = 0; i < input.hang; i++) {
            for (int j = 0; j < input.lie; j++) {
                double val = input.data[i][j];

                if (val == 999.0) {
                    next_map.data[i][j] = 999.0; // 玩家位置保留
                }
                else if (val > 0 && val != 999.0) {
                    // 贪心寻路：向玩家坐标靠拢
                    int ni = i, nj = j;
                    if (px > i) ni++; else if (px < i) ni--;
                    if (py > j) nj++; else if (py < j) nj--;

                    // 体积碰撞排斥：目标格子为空才能移动，否则原地待命
                    if (next_map.data[ni][nj] == 0) next_map.data[ni][nj] = val;
                    else next_map.data[i][j] = val;
                }
            }
        }
        return next_map;
    }
};
