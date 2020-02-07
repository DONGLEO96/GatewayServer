#include "UdpClient.h"


#include<string.h>
#include<unistd.h>
UdpClient::UdpClient(EventLoop * loop, const char * ip, unsigned short port)
{
	_sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
	if (_sockfd == -1)
	{
		fprintf(stderr, "create socket error\n");
		exit(1);
	}


	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	inet_aton(ip, &serveraddr.sin_addr);
	serveraddrlen = sizeof(serveraddr);
	int ret = connect(_sockfd, (sockaddr*)&serveraddr, sizeof(serveraddr));
	if (ret == -1)
	{
		fprintf(stderr, "connect server error\n");
		exit(1);
	}

	_loop = loop;
	_loop->addEvent(_sockfd, std::bind(&UdpClient::do_read, this), EPOLLIN);
	
}

UdpClient::~UdpClient()
{
	_loop->delEvent(_sockfd);
	close(_sockfd);
}

int UdpClient::send_message(const char * data, int datalen, int msgid)
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




	//memcpy(_writebuffer, msg.data(), msg.size());
	int ret = sendto(_sockfd, msg.data(), msg.size(), 0, (sockaddr*)&serveraddr, serveraddrlen);

	if (ret == -1)
	{
		fprintf(stderr, "sendto error\n");
		return -1;
	}

	return ret;
}

int UdpClient::getFd()
{
	return 0;
}

void UdpClient::addMsgCallback(int msgid, MessageCallback cb)
{
	_router.registerMsgCallback(msgid, cb);
}

void UdpClient::do_read()
{
	while (true)
	{
		int ret = recvfrom(_sockfd, _readbuffer, sizeof(_readbuffer), 0, (sockaddr*)&serveraddr, &serveraddrlen);
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

		memcpy(&head, _readbuffer, MessageHeadLength);
		if (head.msglen > MessageMaxLength || head.msglen < 0 || head.msglen + MessageHeadLength != ret)
		{
			fprintf(stderr, "do read error,msgid=%d,msglen=%d,ret=%d\n", head.msgid, head.msglen, ret);
			continue;
		}

		_router.call(head.msgid, head.msglen, _readbuffer + MessageHeadLength, this);

	}

}
