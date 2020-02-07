#pragma once
#include"IObuf.h"
#include<stdio.h>
class ReactorBuff
{
public:
	ReactorBuff();
	~ReactorBuff();
	const size_t length();
	void pop(int len);
	void clear();
protected:
	IOBuff* _buf;
};

