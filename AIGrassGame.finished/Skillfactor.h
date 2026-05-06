#pragma once
#include"jvzhen.h"

// 技能盒子类
class skillbox 
{
public:
	// 十字斩 (Crosscut)
	static jvzhen crosscut()
	{
		jvzhen k(3, 3);
		for (int i = 0; i < 3; i++)
		{
			k.data[1][i] = 1.5;
			k.data[i][1] = 1.5;
		}
		return k;
	}
	
	// X字斩 (X-Slash)
	static jvzhen xslash()
	{
		jvzhen k(3, 3);
		for (int i = 0; i < 3; i++)
		{
			k.data[i][i] = 1.5;
			k.data[i][2 - i] = 1.5;
		}
		return k;
	}
	
	// 裂风 (Windcrack) - 【大招：已进行数值平衡削弱】
	static jvzhen windcrack()
	{
		jvzhen k(5, 5);
		k.data[2][2] = 0.5; // 【修改：中心伤害 2.0 -> 0.8】
		
		// 外圈边缘
		for (int i = 0; i < 5; i++)
		{
			k.data[0][i] = 0.1; // 【修改：外圈伤害 0.5 -> 0.2】
			k.data[4][i] = 0.1;
			k.data[i][0] = 0.1;
			k.data[i][4] = 0.1;
		}
		
		// 内圈
		for (int j = 1; j < 4; j++)
		{
			k.data[1][j] = 0.2; // 【修改：内圈伤害 1.0 -> 0.4】
			k.data[3][j] = 0.2;
			k.data[j][1] = 0.2;
			k.data[j][3] = 0.2;
		}
		return k;
	}
};
