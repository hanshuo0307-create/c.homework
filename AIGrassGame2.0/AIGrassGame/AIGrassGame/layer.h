#pragma once
#include <string>
#include "jvzhen.h"
using namespace std;

//层基类
// ==========================================
class Layer {
public:
	string name;//层名
	Layer(string n)
	{
		name = n;
	}

	virtual ~Layer() {}

	// 虚函数：交给子类实现具体算法
	virtual jvzhen forward(jvzhen input) = 0;
};