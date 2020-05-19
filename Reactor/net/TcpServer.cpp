#include "TcpServer.h"
#include<memory.h>
#include<signal.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<errno.h>
#include"../IOBuff/InputBuff.h"
#include"../IOBuff/OutputBuff.h"
#include"../IOBuff/BuffPool.h"
#include<iostream>

int TcpServer::_maxConns = 0;
int TcpServer::_currConns = 0;
std::mutex TcpServer::_connMutex;
std::vector<shared_ptr<TcpConnection>> TcpServer::_conns;
MsgRouter TcpServer::router;
netCallback TcpServer::_connCompleteCallback = NULL;
netCallback TcpServer::_connCloseCallback = NULL;



void TcpServer::addConn(int connfd, TcpConnection* conn)
{
	std::lock_guard<std::mutex> lock(_connMutex);
	TcpServer::_conns[connfd].reset(conn);
	_currConns += 1;
	//conn->settie(_conns[connfd]);

}

void TcpServer::removeConn(int connfd)
{
	std::lock_guard<std::mutex> lock(_connMutex);
	TcpServer::_conns[connfd].reset();
	_currConns -= 1;
}


int TcpServer::getConnnum()
{
	std::lock_guard<std::mutex> lock(_connMutex);
	return _currConns;
}

shared_ptr<TcpConnection> TcpServer::getConn(int fd)
{
	//不用加锁，每个线程只获取自己管理的链接，相互不影响
	return _conns[fd];
}

void TcpServer::setCompleteCallback(netCallback cb)
{
	_connCompleteCallback = cb;
}

void TcpServer::setCloseCallback(netCallback cb)
{
	_connCloseCallback = cb;
}

struct message
{
	char data[size_4k];
	size_t len;
};
void serverWritcCallBack(EventLoop* loop, int fd, message msg);
void serverReadCallBack(EventLoop* loop, int fd);

void serverWritcCallBack(EventLoop* loop, int fd, message msg)
{
	OutputBuff obuf;
	obuf.sendData(msg.data, static_cast<int>(msg.len));
	while (obuf.length())
	{
		int write_ret = obuf.writeFd(fd);
		if (write_ret == -1)
		{
			fprintf(stderr, "write connfd error\n");
			return;
		}
		else if (write_ret == 0)
		{
			break;
		}
	}

	loop->delEvent(fd, EPOLLOUT);
	loop->addEvent(fd, std::bind(serverReadCallBack, loop, fd), EPOLLIN);
}
void serverReadCallBack(EventLoop* loop, int fd)
{
	message msg;
	int ret = 0;
	InputBuff ibuf;
	ret = ibuf.readData(fd);
	if (ret == -1)
	{
		fprintf(stderr, "ibuf read_data error\n");
		loop->delEvent(fd);
		close(fd);
		return;
	}
	if (ret == 0)
	{
		loop->delEvent(fd);
		close(fd);
		return;
	}

	cout << "ibuf.length()=" << ibuf.length() << endl;

	msg.len = ibuf.length();

	bzero(msg.data, msg.len);

	memcpy(msg.data, ibuf.data(), msg.len);

	ibuf.pop(static_cast<int>(msg.len));
	ibuf.adjust();

	loop->delEvent(fd, EPOLLIN);
	loop->addEvent(fd, std::bind(serverWritcCallBack, loop, fd, msg), EPOLLOUT);
}

TcpServer::TcpServer(EventLoop* loop,const char * ip, uint16_t port, int maxConn, int threadNum):_loop(loop)
{
	memset(&_connaddr, 0, sizeof(_connaddr));
	handleSIGPIPE();
	_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
	if (_sockfd == -1)
	{
		fprintf(stderr, "tcpserver::socker error\n");
		exit(1);
	}

	//initial ip address
	sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_aton(ip, &server_addr.sin_addr);
	server_addr.sin_port = htons(port);

	int op = 1;
	if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) < 0)
	{
		fprintf(stderr, "set socket REUSEADDR error\n");
		exit(1);
	}

	if (bind(_sockfd, (sockaddr*)&server_addr, sizeof(server_addr))<0)
	{
		fprintf(stderr, "bind socker error\n");
		exit(1);
	}

	if (listen(_sockfd, SOMAXCONN) == -1)
	{
		fprintf(stderr, "listen socket error\n");
		exit(1);
	}
	if (_loop == NULL)
	{
		fprintf(stderr, "No event loop\n");
		exit(1);
	}

	_maxConns = maxConn;

	_conns.resize(_maxConns+64, 0);//0,1,2是被标准IO使用的，3没发现被用到哪儿去了，4是监听套接字，多预留几个位置
	
	
	int threadnum = threadNum;
	if (threadnum > 0)
	{
		pool = new ThreadPool(threadnum);
		if (pool == NULL)
		{
			fprintf(stderr, "new threadpool error\n");
			exit(1);
		}
	}

	_loop->addEvent(_sockfd, std::bind(&TcpServer::do_accept, this), EPOLLIN);

}

void TcpServer::do_accept()
{
	printf("begin accept\n");
	int connfd;
	while (1)
	{
		
		connfd = accept4(_sockfd, (sockaddr*)&_connaddr, &_addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
		if (connfd == -1)
		{
			if (errno == EINTR)
			{
				fprintf(stderr, "accept EINTR\n");
				continue;
			}
			else if (errno == EMFILE)
			{
				fprintf(stderr, "accept EMFILE\n");
				//优雅关闭
			}
			else if (errno == EAGAIN)
			{
				fprintf(stderr, "accept EAGAIN\n");
				break;
			}
			else
			{
				fprintf(stderr, "accept error\n");
				exit(1);
			}
		}
		else//accept success
		{
			int cur_conns = getConnnum();
			if (cur_conns >= _maxConns)
			{
				fprintf(stderr, "to many conns,max=%d\n", _maxConns);
				close(cur_conns);
				break;
			}
			else
			{


				if (pool == NULL)
				{
					//ThreadQueue<TaskMsg>* queue = pool->getThread();
					//TaskMsg task;
					//task.type = NEW_CONN;
					//task.connfd = connfd;
					//queue->addTask(task);

					TcpConnection* conn = new TcpConnection(connfd, _loop);

					if (conn == NULL)
					{
						fprintf(stderr, "new TcpConn error\n");
						exit(1);
					}
					TcpServer::addConn(connfd, conn);
					printf("get new connection success\n");
				}
				else
				{
					ThreadQueue<TaskMsg>* queue = pool->getThread();
					TaskMsg task;
					task.type = NEW_CONN;
					task.connfd = connfd;

					queue->addTask(task);
					break;//可以考虑不用跳出，accept接收到EAGAIN时会跳出
				}
			}
		}
	}
}

TcpServer::~TcpServer()
{
	_loop->delEvent(_sockfd);
	close(_sockfd);
}

void TcpServer::handleSIGPIPE()
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = 0;
	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sa, NULL))
	{
		fprintf(stderr, "signal ignore SIGPIPE error\n");
		exit(1);
	}
	
}

void TcpServer::addMsgCallback(int msgid, MessageCallback cb)
{
	router.registerMsgCallback(msgid, cb);
}

void TcpServer::sendTaskToPool(Task t)
{
	this->pool->sendTask(t);
}


