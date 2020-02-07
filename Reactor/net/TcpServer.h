#pragma once
#include<netinet/in.h>
#include"EventLoop.h"
#include"mutex"
#include"TcpConnection.h"
#include"Message.h"
#include"ThreadPool.h"
const int defaultMaxConn = 1024;

class TcpServer
{
public:
	TcpServer(EventLoop* event, const char * ip, uint16_t port,int maxConn=1024,int threadNum=3);
	void do_accept();
	~TcpServer();
	void handleSIGPIPE();
	void addMsgCallback(int msgid, MessageCallback cb);
	void sendTaskToPool(Task t);
	

	static void addConn(int connfd, TcpConnection* conn);
	static void removeConn(int connfd);
	static int getConnnum();
	static shared_ptr<TcpConnection> getConn(int fd);

	static void setCompleteCallback(netCallback cb);
	static void setCloseCallback(netCallback cb);

	

public:
	
	static MsgRouter router;
	static netCallback _connCompleteCallback;
	static netCallback _connCloseCallback;

	static std::vector<shared_ptr<TcpConnection>> _conns;//Connection ÄÚ´æÐ¹Â©
private:
	
	int _sockfd;
	sockaddr_in _connaddr;
	socklen_t _addrlen;
	EventLoop* _loop;
	ThreadPool* pool;

	static int _maxConns;
	static int _currConns;
	static std::mutex _connMutex;
	

};


