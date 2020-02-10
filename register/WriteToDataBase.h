#pragma once
#include"../util/Message.h"
#include<mysql/mysql.h>

struct WriteMessage
{
	int modid;
	int cmdid;
	unsigned int ip;
	int port;
	int op;//0:insert,1:delete
};

class WriteToDataBase
{
public:
	WriteToDataBase();
	~WriteToDataBase();
	void write(WriteMessage msg);

private:
	MYSQL dbconn;	
	void Register(int modid, int cmdid, unsigned int ip, int port);
	void Cancellation(int modid, int cmdid, unsigned int ip, int port);
};

