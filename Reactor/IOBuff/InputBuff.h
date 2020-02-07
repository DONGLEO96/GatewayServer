#pragma once
#include"ReactorBuff.h"

class InputBuff:public ReactorBuff
{
public:
	InputBuff();
	~InputBuff();
	int readData(int fd);
	const char* data() ;
	void adjust();
};

