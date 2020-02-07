#include "HostDetailedInfo.h"
#include<time.h>



HostDetailedInfo::HostDetailedInfo(int ip, int port, int vsucc) :ip(ip), port(port), virtualsucc(vsucc),
virtualerr(0), realsucc(0), realerr(0), continuesucc(0), continueerr(0), overload(false)
{
}

HostDetailedInfo::~HostDetailedInfo()
{
}

void HostDetailedInfo::SetOverLoad(int initerr)
{
	virtualsucc = 0;
	virtualerr = initerr;
	realsucc = 0;
	realerr = 0;
	continuesucc = 0;
	continueerr = 0;
	overload = true;
	overloadts = static_cast<unsigned int>(time(NULL));
}

void HostDetailedInfo::SetIdle(int initsucc)
{
	virtualsucc = initsucc;
	virtualerr = 0;
	realsucc = 0;
	realerr = 0;
	continuesucc = 0;
	continueerr = 0;
	overload = false;
	idlets = static_cast<unsigned int>(time(NULL));
}

bool HostDetailedInfo::CheckWindow(double windowrate)
{
	if (realsucc + realerr == 0)
	{
		return false;
	}
	if ((double)realerr / (realerr + realsucc) >= windowrate)
	{
		return true;
	}
	return false;

}
