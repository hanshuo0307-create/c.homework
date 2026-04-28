#define _CRT_SECURE_NO_WARNINGS // 防止 VS 报错
#include <iostream>
#include <conio.h>    // 键盘交互核心
#include <ctime>      // 随机数核心
#include <windows.h>  // 控制台编码与延迟核心

// 引入你之前创建的所有头文件
#include "jvzhen.h"
#include "layer.h"
#include "ConvLayer.h"
#include "Skillfactor.h"   // 确认文件名是大写 S 还是小写
#include "MonsterAILayer.h"
#include "GameUtils.h"

using namespace std;

int main() {
	// 1. 解决乱码：强制切换控制台为 UTF-8 模式
	//system("chcp 65001");
	srand((unsigned)time(NULL));

	// --- 2. 游戏数值初始化 ---
	const int mapSize = 12;
	jvzhen gameMap(mapSize, mapSize);

	int px = 6, py = 6;      // 主角初始坐标
	gameMap.data[px][py] = 999.0; // 9.0 代表主角

	double playerHP = 100.0;    // 初始血量
	double playerMaxHP = 100.0;

	double playerMP = 100.0;    // 初始蓝量（满蓝开局）
	double playerMaxMP = 100.0; // 【核心限制】蓝量上限 100
	double mpRegen = 0.8;       // 回蓝速度

	int frameCount = 0;         // 帧计数器，用于控制刷怪频率

	cout << ">>> 割草游戏引擎启动成功！" << endl;
	Sleep(1000);

	while (true) {
		// --- 3. 渲染层 (HUD + 地图) ---
		system("cls"); // 清屏
		draw_hud(playerHP, playerMP); // 调用 GameUtils 里的显示函数

		for (int i = 0; i < gameMap.hang; i++) {
			for (int j = 0; j < gameMap.lie; j++) {
				double val = gameMap.data[i][j];
				if (val == 9.0) cout << " ▣ ";      // 画出主角
				else if (val > 0) printf("%3.1f", val); // 画出怪物血量
				else cout << " . ";                // 画出空地
			}
			cout << endl;
		}

		// --- 4. 属性更新 (自动回蓝) ---
		if (playerMP < playerMaxMP) {
			playerMP += mpRegen;
			// 【核心逻辑】强制限制蓝量不能超过 100
			if (playerMP > playerMaxMP) playerMP = playerMaxMP;
		}

		// --- 5. 随机刷怪逻辑 ---
		// 每隔 15 帧刷小怪
		if (frameCount % 15 == 0) {
			int rx = rand() % mapSize;
			int ry = (rand() % 2 == 0) ? 0 : (mapSize - 1);
			if (gameMap.data[rx][ry] == 0) gameMap.data[rx][ry] = 5.0;
		}

		// 【新加】每隔 100 帧（或者当 frameCount == 某数值时），刷一只小 Boss！
		if (frameCount % 100 == 0 && frameCount != 0) {
			int rx = rand() % mapSize;
			int ry = (rand() % 2 == 0) ? 0 : (mapSize - 1);
			if (gameMap.data[rx][ry] == 0) {
				gameMap.data[rx][ry] = 50.0; // 50血的精英怪登场！
			}
		}
		frameCount++; // 别忘了让帧数自己往上加

		// --- 6. 键盘交互层 (WASD + JKLUI) ---
		if (_kbhit()) {
			char key = _getch();
			gameMap.data[px][py] = 0; // 移动前清空原位置

			// 【走位控制】
			int nextX = px, nextY = py;
			if ((key == 'w' || key == 'W') && px > 0) nextX--;
			if ((key == 's' || key == 'S') && px < mapSize - 1) nextX++;
			if ((key == 'a' || key == 'A') && py > 0) nextY--;
			if ((key == 'd' || key == 'D') && py < mapSize - 1) nextY++;

			if (gameMap.data[nextX][nextY] == 0) {
				px = nextX;
				py = nextY;
			}

			// 【技能释放】(不需要再传 kSize 了)
			// J 键：十字斩 (消耗15)
			if ((key == 'j' || key == 'J') && playerMP >= 15.0) {
				playerMP -= 15.0;
				ConvLayer skill(skillbox::crosscut());
				apply_damage(gameMap, skill.forward(gameMap));
			}
			// K 键：X字斩 (消耗15)
			else if ((key == 'k' || key == 'K') && playerMP >= 15.0) {
				playerMP -= 15.0;
				ConvLayer skill(skillbox::xslash());
				apply_damage(gameMap, skill.forward(gameMap));
			}
			// L 键：旋风斩 (消耗45)
			else if ((key == 'l' || key == 'L') && playerMP >= 45.0) {
				playerMP -= 45.0;
				ConvLayer skill(skillbox::windcrack());
				apply_damage(gameMap, skill.forward(gameMap));
			}
			// U 键：垂直束 (消耗20)
			else if ((key == 'u' || key == 'U') && playerMP >= 20.0) {
				playerMP -= 20.0;
				ConvLayer skill(skillbox::yat());
				apply_damage(gameMap, skill.forward(gameMap));
			}
			// I 键：水平束 (消耗20)
			else if ((key == 'i' || key == 'I') && playerMP >= 20.0) {
				playerMP -= 20.0;
				ConvLayer skill(skillbox::xat());
				apply_damage(gameMap, skill.forward(gameMap));
			}

			// 退出游戏：按 Esc 键 (ASCII 码 27)
			if (key == 27) break;

			gameMap.data[px][py] = 999.0; // 更新玩家新坐标
		}

		// --- 7. 自动逻辑层 (AI 移动 + 碰撞伤害) ---
		MonsterAILayer ai;
		gameMap = ai.forward(gameMap); // 怪物向玩家迈进

		check_collision(gameMap, px, py, playerHP); // 检查怪物是否撞到玩家

		// --- 8. 死亡判定 ---
		if (playerHP <= 0) {
			system("cls");
			cout << "\n\n   ======= GAME OVER =======" << endl;
			cout << "   你被数字怪物淹没了..." << endl;
			break;
		}

		Sleep(100); // 维持游戏帧率
	}

	system("pause");
	return 0;
}