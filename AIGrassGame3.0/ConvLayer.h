#pragma once
#include "layer.h"

// 进阶版卷积层：支持任意长宽比的技能矩阵
class ConvLayer : public Layer {
public:
	jvzhen kernel;

	// 构造函数不再需要手动传入 size 了，直接传矩阵！
	ConvLayer(jvzhen k) : Layer("Convolution"), kernel(k.hang, k.lie)
	{
		kernel = k;
	}

	jvzhen forward(jvzhen input) override
	{
		jvzhen result(input.hang, input.lie);
		// 动态计算行和列的偏移量，完美适配 1x7 和 7x1
		int offsetH = kernel.hang / 2;
		int offsetW = kernel.lie / 2;

		for (int i = offsetH; i < input.hang - offsetH; i++) {
			for (int j = offsetW; j < input.lie - offsetW; j++) {
				double sum = 0;
				for (int ki = 0; ki < kernel.hang; ki++) {
					for (int kj = 0; kj < kernel.lie; kj++) {
						sum = sum + input.data[i - offsetH + ki][j - offsetW + kj] * kernel.data[ki][kj];
					}
				}
				result.data[i][j] = sum;
			}
		}
		return result;
	}
};