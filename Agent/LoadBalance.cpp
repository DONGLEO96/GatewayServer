#include "LoadBalance.h"
#include<assert.h>
#include<set>
#include<arpa/inet.h>
using namespace std;
static void GetHostFromList(HostRespose& rep, list<HostDetailedInfo*>& list)
{
	HostDetailedInfo* host = list.front();

	rep.host.ip = host->ip;
	rep.host.port = host->port;

	list.pop_front();
	list.push_back(host);
}

LoadBalance::LoadBalance(int modid, int cmdid, ThreadQueue<DnsRequestMsg>* queue, ThreadQueue<ReportStatusReq>* rqueue, Json::Value config) : _modid(modid), _cmdid(cmdid),_lastupdatets(0), dns_queue(queue),report_queue(rqueue),_config(config)
{
	_initSucc = config["loadbalance"]["initsucc"].asInt();
	_probenum = config["loadbalance"]["probenum"].asInt();
	_initerr = config["loadbalance"]["initerr"].asInt();
	_continuesucc = config["loadbalance"]["continuesucc"].asUInt();
	_continueerr = config["loadbalance"]["continueerr"].asUInt();
	succrate = config["loadbalance"]["succrate"].asDouble();
	errrate = config["loadbalance"]["errrate"].asDouble();
	_windowerrrate = config["loadbalance"]["errrate"].asDouble();
	_idletimeout = config["loadbalance"]["idletimeout"].asInt();
	_overloadtimeout = config["loadbalance"]["overloadtimeout"].asInt();
	
	if (_initSucc == 0)
	{
		_initSucc = 180;
	}
	if (_probenum == 0)
	{
		_probenum = 5;
	}
	if (_initerr == 0)
	{
		_initerr = 10;
	}
	if (_continuesucc == 0)
	{
		_continuesucc = 5;
	}
	if (_continueerr == 0)
	{
		_continueerr = 5;
	}
	if (succrate == 0)
	{
		succrate = 0.8;
	}
	if (errrate == 0)
	{
		errrate = 0.2;
	}
	if (_windowerrrate == 0)
	{
		_windowerrrate = 0.75;
	}
	if (_idletimeout == 0)
	{
		_idletimeout = 15;
	}
	if (_overloadtimeout == 0)
	{
		_overloadtimeout = 15;
	}
}

LoadBalance::~LoadBalance()
{
}

bool LoadBalance::Empty()
{
	return _hostmap.empty();
}

int LoadBalance::GetOneHost(HostRespose & rep)
{
	if (_idlelist.empty())
	{
		if (_access_cnt >= _probenum)
		{
			_access_cnt = 0;
			GetHostFromList(rep, overloadlist);
		}
		else
		{
			++_access_cnt;
			return CallResult::Overload;
		}
	}
	else
	{
		if (overloadlist.empty())
		{
			GetHostFromList(rep, _idlelist);
		}
		else
		{
			if (_access_cnt >= _probenum)
			{
				_access_cnt = 0;
				GetHostFromList(rep, overloadlist);
			}
			else
			{
				++_access_cnt;
				GetHostFromList(rep, _idlelist);
			}
		}
	}
	return CallResult::Sueecss;
}

vector<HostDetailedInfo*> LoadBalance::GetAllHost()
{
	vector<HostDetailedInfo*> res;
	for (auto it = _hostmap.begin(); it != _hostmap.end(); ++it)
	{
		HostDetailedInfo* h = it->second;
		res.push_back(h);
	}
	return res;
}

int LoadBalance::pull()
{
	DnsRequestMsg req;
	req.cmdid = _cmdid;
	req.modid = _modid;

	dns_queue->addTask(req);

	status = Pulling;
	
	return 0;
}

void LoadBalance::update(DnsResponseMsg & rep)
{
	assert(rep.hostnum != 0);

	set<unsigned long long> remotehosts;
	set<unsigned long long> needdelete;
	for (int i = 0; i < rep.hostnum; ++i)
	{
		int ip = rep.ips[i];
		int port = rep.ports[i];
		HostInfo h;
		h.ip = ip;
		h.port = port;

		unsigned long long key = (static_cast<unsigned long long>(ip) << 32) + port;

		remotehosts.insert(key);

		if (_hostmap.find(key) == _hostmap.end())
		{
			HostDetailedInfo* host = new HostDetailedInfo(h.ip, h.port, _initSucc);
			if (host == NULL)
			{
				fprintf(stderr,"new a HostDetailedInfo error\n");
				exit(1);
			}
			_hostmap[key] = host;
			
			_idlelist.push_back(host);
		}
	}

	for (auto it = _hostmap.begin(); it != _hostmap.end(); ++it)
	{
		if (remotehosts.find(it->first) == remotehosts.end())
		{
			needdelete.insert(it->first);
		}
	}

	for (auto it = needdelete.begin(); it != needdelete.end(); ++it)
	{
		unsigned long long key = *it;
		HostDetailedInfo* host = _hostmap[key];

		if (host->overload)
		{
			overloadlist.remove(host);
		}
		else
		{
			_idlelist.remove(host);
		}

		delete host;
		
		_hostmap.erase(key);
	}

	_lastupdatets = static_cast<unsigned int>(time(NULL));
	this->status = New;
}

void LoadBalance::report(int ip, int port, int retcode)
{
	unsigned long long key = (static_cast<unsigned long long>(ip) << 32) + port;

	if (_hostmap.find(key) == _hostmap.end())
	{
		return;
	}
	HostDetailedInfo* host = _hostmap[key];
	//if (retcode == CallResult::Sueecss)
	//{
	//	host->virtualsucc += 1;
	//	host->realsucc += 1;

	//	host->continuesucc += 1;
	//	host->continueerr = 0;
	//}
	//else
	//{
	//	host->realerr += 1;
	//	host->virtualerr += 1;

	//	host->continueerr += 1;
	//	host->continuesucc = 0;
	//}

	if (host->overload == false && retcode != CallResult::Sueecss)
	{
		bool overload = false;
		
		double err_rate = (double)(host->virtualerr) / (host->virtualerr + host->virtualsucc);
		if (err_rate > errrate)
		{
			overload = true;
		}
		if (overload == false && host->continueerr >= _continueerr)
		{
			overload = true;
		}

		if (overload)
		{
			in_addr inaddr;
			inaddr.s_addr = htonl(host->ip);
			printf("[%d,%d] host %s:%d change overload,succ %u err %u\n",
				_modid, _cmdid, inet_ntoa(inaddr), host->port, host->virtualsucc, host->virtualerr);
			host->SetOverLoad(_initerr);
			_idlelist.remove(host);
			overloadlist.push_back(host);
			return;
		}
	}
	if (host->overload && retcode == CallResult::Sueecss)
	{
		bool idle = false;

		double succ_rate = (double)(host->virtualsucc) / (host->virtualsucc + host->virtualerr);

		if (succ_rate >succrate)
		{
			idle = true;
		}
		if (idle == false && host->continuesucc >= _continuesucc)
		{
			idle = true;
		}
		if (idle)
		{
			in_addr inaddr;
			inaddr.s_addr = htonl(host->ip);
			printf("[%d,%d] host %s:%d change succ,succ %u err %u\n",
				_modid, _cmdid, inet_ntoa(inaddr), host->port, host->virtualsucc, host->virtualerr);
			
			host->SetIdle(_initSucc);
			overloadlist.remove(host);
			_idlelist.push_back(host);
		}
	}

	time_t currtime = time(NULL);
	if (host->overload == false)
	{
		if (currtime - host->idlets >= _idletimeout)
		{
			if (host->CheckWindow(_windowerrrate))
			{
				in_addr inaddr;
				inaddr.s_addr = htonl(host->ip);

				printf("[%d,%d] host %s:%d change overload,real succ %u real err %u\n",
					_modid, _cmdid, inet_ntoa(inaddr), host->port, host->virtualsucc, host->virtualerr);

				host->SetOverLoad(_initerr);

				_idlelist.remove(host);
				overloadlist.push_back(host);

			}
			else
			{
				//not overload
				host->SetIdle(_initSucc);
			}
		}
	}
	else
	{
		if (currtime - host->overloadts >= _overloadtimeout)
		{
			in_addr inaddr;
			inaddr.s_addr = htonl(host->ip);
			
			printf("[%d,%d] host %s:%d reset to idle,vsucc %u,verr%u\n",
				_modid, _cmdid, inet_ntoa(inaddr), host->port, host->virtualsucc, host->virtualerr);

			host->SetIdle(_initSucc);
			overloadlist.remove(host);
			_idlelist.push_back(host);
		}
	}

	if (retcode == CallResult::Sueecss)
	{
		host->virtualsucc += 1;
		host->realsucc += 1;

		host->continuesucc += 1;
		host->continueerr = 0;
	}
	else
	{
		host->realerr += 1;
		host->virtualerr += 1;

		host->continueerr += 1;
		host->continuesucc = 0;
	}


}

void LoadBalance::commit()
{
	if (this->Empty())
	{
		return;
	}
	ReportStatusReq req;
	req.modid = _modid;
	req.cmdid = _cmdid;
	req.ts = static_cast<unsigned int>(time(NULL));
	string loacalip = _config["loadbalance"]["localip"].asString();
	in_addr inaddr;
	inet_aton(loacalip.data(), &inaddr);
	req.caller = htonl(inaddr.s_addr);
	for (auto it = _idlelist.begin(); it != _idlelist.end(); ++it)
	{
		HostCallResult h;
		h.err = (*it)->realerr;
		h.succ = (*it)->realsucc;
		h.overload = false;
		h.ip = (*it)->ip;
		h.port = (*it)->port;

		req.results.push_back(h);
		
	}
	for (auto it = overloadlist.begin(); it != overloadlist.end(); ++it)
	{
		HostCallResult h;
		h.err = (*it)->realerr;
		h.succ = (*it)->realsucc;
		h.overload = true;
		h.ip = (*it)->ip;
		h.port = (*it)->port;

		req.results.push_back(h);
	}
	req.resnum = static_cast<int>(req.results.size());

	report_queue->addTask(req);
	////-----´ò°ü³Éjson------
	//Json::Value jsonvalue;
	//jsonvalue["modid"] = req.modid;
	//jsonvalue["cmdid"] = req.cmdid;
	//jsonvalue["caller"] = req.caller;
	//jsonvalue["resnum"] = req.resnum;
	//jsonvalue["ts"] = req.ts;
	//for (int i = 0; i < req.resnum; ++i)
	//{
	//	Json::Value jsonhost;
	//	HostCallResult h = req.results[i];
	//	jsonhost["err"] = h.err;
	//	jsonhost["ip"] = h.ip;
	//	jsonhost["overload"] = h.overload;
	//	jsonhost["port"] = h.port;
	//	jsonhost["succ"] = h.succ;
	//	
	//	jsonvalue["results"].append(jsonhost);
	//}


}
