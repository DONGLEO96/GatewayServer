#include "ReactorBuff.h"
#include<assert.h>
#include<algorithm>
#include"BuffPool.h"
using namespace std;
ReactorBuff::ReactorBuff():_buf(NULL)
{
}
ReactorBuff::~ReactorBuff()
{
	clear();
}

const size_t ReactorBuff::length()
{
	if (_buf!=NULL)
		return _buf->length;
	else
		return 0;
}

void ReactorBuff::pop(int len)
{
	assert(_buf != NULL);
	_buf->pop(min(len, (int)_buf->length));
	if (_buf->length == 0)
	{
		BuffPool::getInstance()->revert(_buf);
		_buf = NULL;
	}

}

void ReactorBuff::clear()
{
	if (_buf!=NULL)
	{
		BuffPool::getInstance()->revert(_buf);
		_buf = NULL;
	}
}

