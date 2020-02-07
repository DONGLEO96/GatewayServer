#include "InputBuff.h"
#include"sys/ioctl.h"
#include"BuffPool.h"
#include<unistd.h>
#include<assert.h>
InputBuff::InputBuff()
{
}


InputBuff::~InputBuff()
{
}

int InputBuff::readData(int fd)
{
	int readAbleBytes;
	if (ioctl(fd, FIONREAD, &readAbleBytes) == -1)
	{
		fprintf(stderr, "ioctl error\n");
		return -1;
	}
	if (_buf == NULL)
	{
		_buf = BuffPool::getInstance()->alloc_buf(readAbleBytes);
		if (_buf == NULL)
		{
			fprintf(stderr, "No fit buf for alloc\n");
			return -1;
		}
	}
	else
	{
		if (_buf->head != 0)
		{
			_buf->adjust();
		}
		if (static_cast<int>(_buf->capacity - _buf->length) < readAbleBytes)
		{
			//alloc a bigger IObuff;
			IOBuff* newBuf = BuffPool::getInstance()->alloc_buf(readAbleBytes + _buf->length);
			if (newBuf == NULL)
			{
				fprintf(stderr, "NO fit buf for alloc\n");
				return -1;
			}

			newBuf->copy(_buf);

			BuffPool::getInstance()->revert(_buf);
			
			_buf = newBuf;
		}
		
	}

	//read bytes
	ssize_t already_read = 0;
	//-------为什么这么写???---------
	do 
	{
		if (readAbleBytes == 0)
		{
			
			already_read = read(fd, _buf->data + _buf->length, size_4k);
		}
		else
		{
			already_read = read(fd, _buf->data + _buf->length, readAbleBytes);
		}
	} while (already_read == -1 && errno == EINTR);
	if (already_read > 0)
	{
		if (readAbleBytes != 0)
			assert(already_read == readAbleBytes);
		_buf->length += already_read;
	}

	return (int)already_read;
	
	//-------为什么这么写???---------


}

const char * InputBuff::data() //返回将要读取的数据
{
	if (_buf != NULL)
	{
		return _buf->data+_buf->head;
	}
	else
	{
		return NULL;
	}
}

void InputBuff::adjust()
{
	if (_buf != NULL)
	{
		_buf->adjust();
	}
}
