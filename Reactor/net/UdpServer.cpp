#include "UdpServer.h"
#include<string.h>
#include<signal.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<iostream>
UdpServer::UdpServer(EventLoop * loop, const char * ip, unsigned short port)
{
	handleSIGPIPE();

	_sockfd = socket(AF_INET,SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
	if (_sockfd == -1)
	{
		fprintf(stderr, "create udp socket error\n");
		exit(1);
	}

	int on = 1;
	setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &on, sizeof(on));

	sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_aton(ip, &server_addr.sin_addr);
	server_addr.sin_port = htons(port);

	if (bind(_sockfd, (sockaddr*)&server_addr, sizeof(sockaddr)) < 0)
	{
		fprintf(stderr, "bind socker error\n");
		exit(1);
	}

	_loop = loop;

	memset(&_clientAddr, 0, sizeof(_clientAddr));
	_clientAddrLen = sizeof(_clientAddrLen);

	printf("UdpServer on %s:%u is Running...\n", ip, port);

	_loop->addEvent(_sockfd, std::bind(&UdpServer::do_read, this), EPOLLIN);



}

UdpServer::~UdpServer()
{
	_loop->delEvent(_sockfd);
	close(_sockfd);
}

int UdpServer::send_message(const char * data, int datalen, int msgid)
{
	if (datalen > MessageMaxLength)
	{
		fprintf(stderr, "too data to send\n");
		return -1;
	}

	msg_head head;
	head.msglen = datalen;
	head.msgid = msgid;

	string msg;
	msg.append((char*)&head, MessageHeadLength);
	msg.append(data, datalen);
	
	
	printf("server send_message:%s,msglen=%d,msgid=%d\n", data, head.msglen, msgid);
	
	
	int ret = sendto(_sockfd, msg.data(), msg.size(), 0, (sockaddr*)&_clientAddr, _clientAddrLen);

	if (ret == -1)
	{
		fprintf(stderr, "sendto error\n");
		return -1;
	}

	return ret;

}

int UdpServer::getFd()
{
	return _sockfd;
}

void UdpServer::handleSIGPIPE()
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

void UdpServer::addMsgCallback(int msgid, MessageCallback cb)
{
	_router.registerMsgCallback(msgid, cb);
}

void UdpServer::do_read()
{
	while (true)
	{
		int ret = recvfrom(_sockfd, _readbuffer, sizeof(_readbuffer), 0, (sockaddr*)&_clientAddr, &_clientAddrLen);
		if (ret == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
			else if (errno == EAGAIN)
			{
				break;
			}
			else
			{
				fprintf(stderr, "recvfrom error\n");
				break;
			}
		}

		msg_head head;
		//cout << _readbuffer << endl;
		memcpy(&head, _readbuffer, MessageHeadLength);
		if (head.msglen > MessageMaxLength || head.msglen < 0 || head.msglen + MessageHeadLength != ret)
		{
			fprintf(stderr, "do read error,msgid=%d,msglen=%d,ret=%d\n", head.msgid, head.msglen, ret);
			continue;
		}

		_router.call(head.msgid, head.msglen, _readbuffer + MessageHeadLength, this);

	}

}
