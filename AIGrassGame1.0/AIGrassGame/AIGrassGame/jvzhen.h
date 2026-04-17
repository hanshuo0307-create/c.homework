#pragma once
#include <vector>
using namespace std;

//矩阵类
// ==========================================
class jvzhen {
public:
	int hang, lie; // 定义行和列变量
	vector<vector<double>> data; // 创建二维动态数组
	jvzhen(int h, int l)
	{
		hang = h; lie = l;
		data.resize(hang); // 给行分配空间
		for (int i = 0; i < hang; i++)
		{
			data[i].resize(lie); // 用for循环给每一行分配列空间
			// 初始化为0.0
			for (int j = 0; j < lie; j++) {
				data[i][j] = 0.0;
			}
		}
	}
};

//卷积核类：给卷积核特定位置自由赋值
// ==========================================
void input_he(jvzhen& k, double val)
{
	for (int i = 0; i < k.hang; i++)
	{
		for (int j = 0; j < k.lie; j++)
		{
			k.data[i][j] = val;
		}
	}
}