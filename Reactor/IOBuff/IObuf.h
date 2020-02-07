#pragma once
#include<stdlib.h>
class IOBuff
{
public:
	IOBuff(int size);
	~IOBuff();
	void clear();
	
	void adjust();
	
	void copy(const IOBuff* rhs);



	void pop(int len);
	
	IOBuff* next;
	size_t capacity;
	size_t length;
	size_t head;//将要读取的偏移量
	char* data;
};

