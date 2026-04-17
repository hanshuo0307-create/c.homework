#include <iostream>
#include <string>
#include "jvzhen.h"
#include "Layer.h"
#include "ConvLayer.h"
using namespace std;

//主程序
int main() {
	//创建"地图"
	jvzhen map1(5, 5);
	map1.data[2][2] = 1.0;

	//创建卷积核
	int size = 3;
	jvzhen he(size, size);
	input_he(he, 0.5); //统一赋值 0.5
	he.data[1][1] = 2.0; // 特别加强中心点

	//在该层载入卷积核
	Layer* cal = new ConvLayer(size, he);

	//计算结果
	jvzhen out = cal->forward(map1);


	cout << "卷积运算结果如下：" << endl;
	for (int i = 0; i < out.hang; i++)
	{
		for (int j = 0; j < out.lie; j++)
		{
			printf(" % .1f", out.data[i][j]);
			cout << " ";
		}
		cout << endl;
	}

	delete cal;
	system("pause");
	return 0;
}