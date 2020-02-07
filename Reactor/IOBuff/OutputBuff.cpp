#include "OutputBuff.h"
#include"BuffPool.h"
#include<memory.h>
#include<unistd.h>
OutputBuff::OutputBuff()
{
}


OutputBuff::~OutputBuff()
{
}

int OutputBuff::sendData(const char * data, int size)
{
	if (_buf == NULL)
	{
		_buf = BuffPool::getInstance()->alloc_buf(size);
		if (_buf == NULL)
		{
			fprintf(stderr, "no fit buf for alloc\n");
			return -1;
		}
	}
	else
	{
		if (_buf->head != 0)
		{
			_buf->adjust();
		}
		if (static_cast<int>(_buf->capacity - _buf->length) < size)
		{
			IOBuff* newBuf = BuffPool::getInstance()->alloc_buf(size + _buf->length);
			if (newBuf == NULL)
			{
				fprintf(stderr, "No fit buf for alloc\n");
				return -1; 
			}

			newBuf->copy(_buf);
			BuffPool::getInstance()->revert(_buf);

			_buf = newBuf;
		}
	}

	memcpy(_buf->data + _buf->length, data, size);
	_buf->length += size;

	return 0;
}

int OutputBuff::writeFd(int fd)
{
	if (_buf == NULL)
		return -1;
	ssize_t already_write = 0;

	do
	{
		already_write = write(fd, _buf->data+_buf->head, _buf->length);
	} while (already_write == -1 && errno == EINTR);
	if (already_write > 0)
	{
		_buf->pop(static_cast<int>(already_write));
		_buf->adjust();
	}
	if (already_write == -1 && errno == EAGAIN)
	{
		already_write = 0;
	}
	return static_cast<int>(already_write);

	return 0;
}
