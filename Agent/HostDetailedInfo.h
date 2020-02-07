#pragma once
struct HostDetailedInfo
{
	HostDetailedInfo(int ip,int port,int vsucc);
	~HostDetailedInfo();

	void SetOverLoad(int initerr);
	void SetIdle(int initsucc);
	bool CheckWindow(double windowrate);
	int ip;
	int port;
	unsigned int virtualsucc;
	unsigned int virtualerr;
	unsigned int realsucc;
	unsigned int realerr;
	unsigned int continuesucc;
	unsigned int continueerr;
	unsigned int idlets;
	unsigned int overloadts;
  	bool overload;
};

