#pragma once
#include "Layer.h"

// 怪物 AI 层：负责让怪物向玩家坐标靠拢
class MonsterAILayer : public Layer {
public:
	MonsterAILayer() : Layer("MonsterAI") {}

	jvzhen forward(jvzhen input) override {
		jvzhen next_map(input.hang, input.lie);
		int px = -1, py = -1;

		// 1. 寻找玩家 (9.0)
		for (int i = 0; i < input.hang; i++) {
			for (int j = 0; j < input.lie; j++) {
				if (input.data[i][j] == 999.0) { px = i; py = j; break; }
			}
		}

		// 2. 遍历地图移动怪物
		for (int i = 0; i < input.hang; i++) {
			for (int j = 0; j < input.lie; j++) {
				double val = input.data[i][j];

				if (val == 999.0) {
					next_map.data[i][j] = 999.0; // 玩家保持原样
				}
				else if (val > 0 && val < 999.0) { // 发现怪物
					int ni = i, nj = j;
					// 向量追踪逻辑：计算下一步坐标
					if (px > i) ni++; else if (px < i) ni--;
					if (py > j) nj++; else if (py < j) nj--;

					// 如果目标点没被占领，就移动
					if (next_map.data[ni][nj] == 0) next_map.data[ni][nj] = val;
					else next_map.data[i][j] = val; // 被挡住了就原地待着
				}
			}
		}
		return next_map;
	}
};