#pragma once
#include"ReactorBuff.h"

class OutputBuff:public ReactorBuff
{
public:
	OutputBuff();
	~OutputBuff();

	int sendData(const char* data, int size);
	int writeFd(int fd);
};

