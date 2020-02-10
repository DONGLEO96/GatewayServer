#include "WriteToDataBase.h"
#include<stdio.h>
#include<string.h>
#include<time.h>
WriteToDataBase::WriteToDataBase()
{
	mysql_library_init(0, NULL, NULL);
	//多线程初始化

	mysql_init(&dbconn);
	
	mysql_options(&dbconn, MYSQL_OPT_CONNECT_TIMEOUT, "30");
	char reconnect = 1;
	mysql_options(&dbconn, MYSQL_OPT_RECONNECT, &reconnect);


	if (!mysql_real_connect(&dbconn, "0.0.0.0", "root", "111111", "dns", 3306, NULL, 0))
	{
		fprintf(stderr, "Faied to connect mysql\n");
		exit(1);
	}

}


WriteToDataBase::~WriteToDataBase()
{
	//mysql_close(&dbconn);
}

void WriteToDataBase::write(WriteMessage msg)
{
	int op = msg.op;
	if (op == 1)
	{
		this->Cancellation(msg.modid, msg.cmdid, msg.ip, msg.port);
	}
	else
	{
		this->Register(msg.modid, msg.cmdid, msg.ip, msg.port);
	}
}

void WriteToDataBase::Register(int modid, int cmdid, unsigned int ip, int port)
{
	char sql1[1024];
	char sql2[1024];
	char sql3[1024];
	unsigned int ts = static_cast<int>(time(NULL));
	snprintf(sql1, 1024, "INSERT INTO RouteData(modid,cmdid,serverip,serverport) VALUES(%d,%d,%u,%d)", modid, cmdid, ip, port);
	snprintf(sql2, 1024, "UPDATE RouteVersion SET version=%u WHERE id=1", ts);
	snprintf(sql3, 1024, "INSERT INTO RouteChange(modid,cmdid,version) VALUES(%d,%d,%u);", modid, cmdid, ts);
	if (mysql_real_query(&dbconn, sql1, strlen(sql1)) != 0)
	{
		fprintf(stderr, "Fail to Insert Into RouteData: %s\n", mysql_error(&dbconn));
	}
	if (mysql_real_query(&dbconn, sql2, strlen(sql2)) != 0)
	{
		fprintf(stderr, "Fail to Update RouteVersion: %s\n", mysql_error(&dbconn));
	}
	if (mysql_real_query(&dbconn, sql3, strlen(sql3)) != 0)
	{
		fprintf(stderr, "Fail to Insert Into RouteChange: %s\n", mysql_error(&dbconn));
	}
}

void WriteToDataBase::Cancellation(int modid, int cmdid, unsigned int ip, int port)
{
	char sql1[1024];
	char sql2[1024];
	char sql3[1024];
	unsigned int ts = static_cast<int>(time(NULL));
	snprintf(sql1, 1024, "DELETE FROM RouteData WHERE serverip=%u AND serverport=%d", ip, port);
	snprintf(sql2, 1024, "UPDATE RouteVersion SET version=%u WHERE id=1", ts);
	snprintf(sql3, 1024, "INSERT INTO RouteChange(modid,cmdid,version) VALUES(%d,%d,%u)", modid, cmdid, ts);
	if (mysql_real_query(&dbconn, sql1, strlen(sql1)) != 0)
	{
		fprintf(stderr, "Fail to Delete From RouteData: %s\n", mysql_error(&dbconn));
	}
	if (mysql_real_query(&dbconn, sql2, strlen(sql2)) != 0)
	{
		fprintf(stderr, "Fail to Update RouteVersion: %s\n", mysql_error(&dbconn));
	}
	if (mysql_real_query(&dbconn, sql3, strlen(sql3)) != 0)
	{
		fprintf(stderr, "Fail to Insert Into RouteChange: %s\n", mysql_error(&dbconn));
	}
	
}
//Fail to Insert Into RouteData You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near 'UPDATE RouteVersion SET version=1581307841 WHERE id=1:INSERT INTO RouteChange(mo' at line 1
