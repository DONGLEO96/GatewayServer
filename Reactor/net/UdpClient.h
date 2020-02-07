#pragma once
#include"net_conn.h"
#include"EventLoop.h"
#include"Message.h"
#include<arpa/inet.h>
#include<sys/socket.h>
class UdpClient:public net_conn
{
public:
	UdpClient(EventLoop* loop, const char* ip, unsigned short port);
	~UdpClient();

	virtual int send_message(const char* data, int datalen, int msgid);
	virtual int getFd();

	void addMsgCallback(int msgid, MessageCallback cb);
	void do_read();
private:
	int _sockfd;
	char _readbuffer[MessageMaxLength];
	char _writebuffer[MessageMaxLength];

	sockaddr_in  serveraddr;
	socklen_t serveraddrlen;

	EventLoop* _loop;

	MsgRouter _router;
};

