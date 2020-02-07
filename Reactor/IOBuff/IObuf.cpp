#include "IObuf.h"
#include<assert.h>
#include<memory.h>
IOBuff::IOBuff(int size) : next(NULL),capacity(size), length(0), head(0),data(NULL)
{
	//data = new char[capacity];
	data = (char*)malloc(capacity);
	assert(data);
}

IOBuff::~IOBuff()
{
	delete data;
}

void IOBuff::clear()
{
	length = 0;
	head = 0;
}

void IOBuff::adjust()
{
	if (head == 0)
		return;
	if(length!=0)
		memmove(data, data + head, length);
	head = 0;
	return;
}

void IOBuff::copy(const IOBuff* rhs)
{
	memcpy(data, rhs->data + rhs->head, rhs->length);
	head = 0;
	length = rhs->length;
}


//void IOBuff::copy(IOBuff && rhs)
//{
//	data = rhs.data;
//	length = rhs.length;
//	head = rhs.head;
//
//	rhs.data = NULL;
//	rhs.head = 0;
//	rhs.length = 0;
//}

void IOBuff::pop(int len)
{
	length -= len;
	head += len;
}
