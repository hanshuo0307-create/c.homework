#pragma once
#include "Layer.h"
#include "jvzhen.h"
using namespace std;

//卷积运算子类
// ==========================================
class ConvLayer : public Layer {
public:
	int kSize;//卷积核边长
	jvzhen kernel; // 卷积核矩阵

	ConvLayer(int s, jvzhen k) : Layer("Convolution"), kernel(k.hang, k.lie)
	{
		kSize = s;
		kernel = k;
	}

	jvzhen forward(jvzhen input) override
	{
		//计算输出矩阵的尺寸
		int outH = input.hang - kSize + 1;
		int outW = input.lie - kSize + 1;
		jvzhen result(outH, outW);//创建结果矩阵
		//四层for循环进行卷积运算
		for (int i = 0; i < outH; i++) {
			for (int j = 0; j < outW; j++) {
				double sum = 0;
				for (int ki = 0; ki < kSize; ki++) {
					for (int kj = 0; kj < kSize; kj++) {
						sum = sum + input.data[i + ki][j + kj] * kernel.data[ki][kj];
					}
				}
				result.data[i][j] = sum;
			}
		}
		return result;
	}
};