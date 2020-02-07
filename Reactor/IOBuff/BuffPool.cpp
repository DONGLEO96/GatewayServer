#include "BuffPool.h"

using namespace std;
BuffPool* BuffPool::_instance = NULL;

BuffPool::BuffPool():_totalmem(0)
{
	IOBuff* pre;

	//initial 4K;
	_bufpool[size_4k] = new IOBuff(size_4k);
	if (_bufpool[size_4k] == NULL)
	{
		fprintf(stderr, "new IOBuff 4K error\n");
		exit(1);
	}
	pre = _bufpool[size_4k];
	for (int i = 1; i < 5000; ++i)
	{
		pre->next = new IOBuff(size_4k);
		if (pre->next == NULL)
		{
			fprintf(stderr, "new IOBuff 4K error\n");
			exit(1);
		}
		pre = pre->next;
	}
	_totalmem += 4 * 5000;


	//initial 16K;
	_bufpool[size_16k] = new IOBuff(size_16k);
	if (_bufpool[size_16k] == NULL)
	{
		fprintf(stderr, "new IOBuff 16K error\n");
		exit(1);
	}
	pre = _bufpool[size_16k];
	for (int i = 1; i < 1000; ++i)
	{
		pre->next = new IOBuff(size_16k);
		if (pre->next == NULL)
		{
			fprintf(stderr, "new IOBuff 16K error\n");
			exit(1);
		}
		pre = pre->next;
	}
	_totalmem += 16 * 1000;


	//initial 64K;
	_bufpool[size_64k] = new IOBuff(size_64k);
	if (_bufpool[size_64k] == NULL)
	{
		fprintf(stderr, "new IOBuff 64K error\n");
		exit(1);
	}
	pre = _bufpool[size_64k];
	for (int i = 1; i < 500; ++i)
	{
		pre->next = new IOBuff(size_64k);
		if (pre->next == NULL)
		{
			fprintf(stderr, "new IOBuff 64K error\n");
			exit(1);
		}
		pre = pre->next;
	}
	_totalmem += 64 * 500;


	//initial 256K;
	_bufpool[size_256k] = new IOBuff(size_256k);
	if (_bufpool[size_256k] == NULL)
	{
		fprintf(stderr, "new IOBuff 256K error\n");
		exit(1);
	}
	pre = _bufpool[size_256k];
	for (int i = 1; i < 200; ++i)
	{
		pre->next = new IOBuff(size_256k);
		if (pre->next == NULL)
		{
			fprintf(stderr, "new IOBuff 256K error\n");
			exit(1);
		}
		pre = pre->next;
	}
	_totalmem += 256 * 200;


	//initial 1M;
	_bufpool[size_1M] = new IOBuff(size_1M);
	if (_bufpool[size_1M] == NULL)
	{
		fprintf(stderr, "new IOBuff 1M error\n");
		exit(1);
	}
	pre = _bufpool[size_1M];
	for (int i = 1; i < 50; ++i)
	{
		pre->next = new IOBuff(size_1M);
		if (pre->next == NULL)
		{
			fprintf(stderr, "new IOBuff 1M error\n");
			exit(1);
		}
		pre = pre->next;
	}
	_totalmem += 1024 * 50;


	//initial 4M;
	_bufpool[size_4M] = new IOBuff(size_4M);
	if (_bufpool[size_4M] == NULL)
	{
		fprintf(stderr, "new IOBuff 4M error\n");
		exit(1);
	}
	pre = _bufpool[size_4M];
	for (int i = 1; i < 20; ++i)
	{
		pre->next = new IOBuff(size_4M);
		if (pre->next == NULL)
		{
			fprintf(stderr, "new IOBuff 4M error\n");
			exit(1);
		}
		pre = pre->next;
	}
	_totalmem += 4096 * 20;


	//initial 8M;
	_bufpool[size_8M] = new IOBuff(size_8M);
	if (_bufpool[size_8M] == NULL)
	{
		fprintf(stderr, "new IOBuff 8M error\n");
		exit(1);
	}
	pre = _bufpool[size_8M];
	for (int i = 1; i < 10; ++i)
	{
		pre->next = new IOBuff(size_8M);
		if (pre->next == NULL)
		{
			fprintf(stderr, "new IOBuff 8M error\n");
			exit(1);
		}
		pre = pre->next;
	}
	_totalmem += 8192 * 10;



}
BuffPool::~BuffPool()
{
}

IOBuff * BuffPool::alloc_buf(size_t size)
{
	int index;
	if (size <= size_4k)
	{
		index = size_4k;
	}
	else if (size <= size_16k)
	{
		index = size_16k;
	}
	else if (size <= size_64k)
	{
		index = size_64k;
	}
	else if (size <= size_256k)
	{
		index = size_256k;
	}
	else if (size <= size_1M)
	{
		index = size_1M;
	}
	else if (size <= size_4M)
	{
		index = size_4M;
	}
	else if (size <= size_8M)
	{
		index = size_8M;
	}
	else
		return NULL;

	lock_guard<mutex> lock(_mutex);
	if (_bufpool[index] == NULL)
	{
		if (_totalmem + index / 1024 >= MAXLIMIT) 
		{
			fprintf(stderr, "use too many memory\n");
			//内存用得太多了
			exit(1);
		}
		IOBuff* buf = new IOBuff(index);
		if (buf == NULL)
		{
			fprintf(stderr, "new iobuf error\n");
			exit(1);
		}
		_totalmem += index / 1024;
		return buf;
	}

	//record buf count 
	bufCount[index] += 1;

	IOBuff* target = _bufpool[index];
	_bufpool[index] = target->next;
	target->next = NULL;

	return target;
	


	return NULL;
}

void BuffPool::revert(IOBuff * buf)
{
	if (buf == NULL)
		return;
	int index = (int)(buf->capacity);
	if (_bufpool.find(index) == _bufpool.end())
	{
		fprintf(stderr, "revert error buf\n");
		exit(1);
	}
	buf->clear();
	lock_guard<mutex> lock(_mutex);

	//record buf count 
	bufCount[index] -= 1;

	if (_bufpool[index] == NULL)
	{
		_bufpool[index] = buf;
	}
	else
	{
		buf->next = _bufpool[index]->next;
		_bufpool[index] = buf;
	}
}
