#pragma once
#include <iostream>
#include <windows.h>
#include "jvzhen.h"

// 战斗结算：扣除血量
inline void apply_damage(jvzhen& map, const jvzhen& damage) {
	for (int i = 0; i < map.hang; i++) {
		for (int j = 0; j < map.lie; j++) {
			if (map.data[i][j] > 0 && map.data[i][j] < 999.0) {
				map.data[i][j] -= damage.data[i][j];
				if (map.data[i][j] < 0) map.data[i][j] = 0;
			}
		}
	}
}

// 碰撞伤害：怪物撞到玩家扣血
inline void check_collision(const jvzhen& map, int px, int py, double& hp) {
	int dx[] = { -1, 1, 0, 0 };
	int dy[] = { 0, 0, -1, 1 };

	for (int i = 0; i < 4; i++) {
		int nx = px + dx[i];
		int ny = py + dy[i];
		if (nx >= 0 && nx < map.hang && ny >= 0 && ny < map.lie) {
			double enemy_hp = map.data[nx][ny];
			if (enemy_hp > 0 && enemy_hp < 999.0) {
				// 伤害差异化：判断是小怪还是 Boss
				if (enemy_hp > 20.0) {
					hp -= 5.0; // 小 Boss 伤害高！
				}
				else {
					hp -= 0.5; // 普通小怪伤害低
				}
			}
		}
	}
}

// 画出 HUD 状态栏
inline void draw_hud(double hp, double mp) {
	printf("\n [ 血量 ]: ");
	for (int i = 0; i < 10; i++) i < (hp / 10) ? printf("#") : printf("-"); // 用 # 和 -
	printf(" %.1f", hp);

	printf("   [ 蓝量 ]: ");
	for (int i = 0; i < 10; i++) i < (mp / 10) ? printf("*") : printf("_"); // 用 * 和 _
	printf(" %.1f\n", mp);
	printf(" 移动:WASD | 攻击:J(十字) K(X字) L(旋风) U(纵向激光) I(横向激光) | Esc退出\n");
}