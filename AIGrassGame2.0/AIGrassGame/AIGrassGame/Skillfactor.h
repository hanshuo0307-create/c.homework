#pragma once
#include"jvzhen.h"

//创建技能类
class skillbox 
{
public:
	//【十字斩】十字攻击
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
	//【X字斩】对角线攻击
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
	//【旋风斩】攻击范围扩大，中心加强，边缘处有余波小伤害
	static jvzhen windcrack()
	{
		jvzhen k(5, 5);
		k.data[2][2] = 2;//中心
		//边缘
		for (int i = 0; i < 5; i++)
		{
			k.data[0][i] = 0.5;
			k.data[4][i] = 0.5;
			k.data[i][0] = 0.5;
			k.data[i][4] = 0.5;
		}
		for (int j = 1; j < 4; j++)
		{
			k.data[1][j] = 1.0;
			k.data[3][j] = 1.0;
			k.data[j][1] = 1.0;
			k.data[j][3] = 1.0;
		}
		return k;
	}
	//【垂直束】
	static jvzhen yat()
	{
		jvzhen k(7, 1);
		for (int i = 0; i < 7; i++)
		{
			k.data[i][0] = 2;
		}
		return k;
	}
	//【水平束】
	static jvzhen xat()
	{
		jvzhen k(1,7);
		for (int i = 0; i < 7; i++)
		{
			k.data[0][i] = 2;
		}
		return k;
	}
};