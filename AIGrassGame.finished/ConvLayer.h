#pragma once
#include "layer.h"

//卷积层：利用二维卷积运算，处理复杂的范围伤害判定
class ConvLayer : public Layer {
public:
    jvzhen kernel; // 卷积核 (各种技能的形状与伤害权重矩阵)

    ConvLayer(jvzhen k) : Layer("Convolution"), kernel(k.hang, k.lie) {
        kernel = k;
    }

    //forward函数（两个for循环进行卷积运算）
    jvzhen forward(jvzhen input) override
    {
        jvzhen result(input.hang, input.lie);
        int offsetH = kernel.hang / 2;
        int offsetW = kernel.lie / 2;

        for (int i = offsetH; i < input.hang - offsetH; i++) {
            for (int j = offsetW; j < input.lie - offsetW; j++) {
                double sum = 0;

                for (int ki = 0; ki < kernel.hang; ki++) {
                    for (int kj = 0; kj < kernel.lie; kj++) {
                        sum += input.data[i - offsetH + ki][j - offsetW + kj] * kernel.data[ki][kj];
                    }
                }
                result.data[i][j] = sum;
            }
        }
        return result;
    }
};
