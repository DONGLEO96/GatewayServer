#include "RouteLoadBalance.h"
#include<assert.h>




RouteLoadBalance::RouteLoadBalance(int id,Json::Value config):_lbmap(),_mutex(),_id(id),dns_queue(NULL),report_queue(NULL),_config(config)
{
	updatetime = config["loadbalance"]["updatetime"].asUInt();
	if (updatetime == 0)
	{
		updatetime = 10;
	}
}

RouteLoadBalance::~RouteLoadBalance()
{
}

void RouteLoadBalance::ResetAllLoadBalance()
{
	lock_guard<mutex> lock(_mutex);
	for (auto it = _lbmap.begin(); it != _lbmap.end(); ++it)
	{
		auto lb = it->second;
		lb->status = LoadBalance::Status::New;
	}
}

void RouteLoadBalance::SetDnsQueue(ThreadQueue<DnsRequestMsg>* queue)
{
	dns_queue = queue;
}

void RouteLoadBalance::SetReportQueue(ThreadQueue<ReportStatusReq>* queue)
{
	report_queue = queue;
}

void RouteLoadBalance::ReportHost(ReportRequest req)
{
	int modid = req.modid;
	int cmdid = req.cmdid;
	int retcode = req.result;
	int ip = req.host.ip;
	int port = req.host.port;

	unsigned long long key = (static_cast<unsigned long long>(modid) << 32) + cmdid;

	lock_guard<mutex> lock(_mutex);
	if (_lbmap.find(key) != _lbmap.end())
	{
		LoadBalance* lb = _lbmap[key];
		lb->report(ip, port, retcode);
		lb->commit();
	}
}

int RouteLoadBalance::GetRouteInfo(int modid, int cmdid, DnsResponseMsg & rep)
{
	int ret = CallResult::Sueecss;
	unsigned long long key = (static_cast<unsigned long long>(modid) << 32) + cmdid;

	lock_guard<mutex> lock(_mutex);

	if (_lbmap.find(key) != _lbmap.end())
	{
		auto lb = _lbmap[key];
		vector<HostDetailedInfo*> hosts = lb->GetAllHost();

		for (auto it = hosts.begin(); it != hosts.end(); ++it)
		{
			rep.ips.push_back((*it)->ip);
			rep.ports.push_back((*it)->port);
		}

		if (lb->status == LoadBalance::Status::New && time(NULL) - lb->_lastupdatets > updatetime)
		{
			lb->pull();
		}
	}
	else
	{
		LoadBalance* lb = new LoadBalance(modid, cmdid, this->dns_queue, this->report_queue, _config);
		if (lb == NULL)
		{
			fprintf(stderr, "new LoadBalance error\n");
			exit(1);
		}
		_lbmap[key] = lb;
		lb->pull();
		ret = CallResult::NotFound;
	}
	return ret;
}

int RouteLoadBalance::GetHosts(int modid, int cmdid, HostRespose & rep)
{
	int res = CallResult::Sueecss;
	unsigned long long key = (static_cast<unsigned long long>(modid) << 32) + cmdid;
	lock_guard<mutex> lock(_mutex);
	if (_lbmap.find(key) != _lbmap.end())
	{
		LoadBalance* lb = _lbmap[key];
		if (lb->Empty())
		{
			assert(lb->status == LoadBalance::Status::Pulling);
			rep.callresult = CallResult::NotFound;
		}
		else
		{
			res = lb->GetOneHost(rep);
			rep.callresult = res;

			//-----超时重新拉去路由信息-----
			if (lb->status == LoadBalance::Status::New && time(NULL) - lb->_lastupdatets > updatetime)
			{
				printf("Pull Route Info Again\n");
				lb->pull();
			}
		}
	}

	else
	{
		LoadBalance* lb = new LoadBalance(modid, cmdid, this->dns_queue,this->report_queue, _config);
		if (lb == NULL)
		{
			fprintf(stderr, "new load balance error\n");
			exit(1);
		}
		_lbmap[key] = lb;
		lb->pull();
		rep.callresult = CallResult::NotFound;
		res = CallResult::NotFound;
	}
	return res;

}

int RouteLoadBalance::UpdateHost(int modid, int cmdid, DnsResponseMsg & rep)
{
	unsigned long long key = (static_cast<unsigned long long>(modid) << 32) + cmdid;

	lock_guard<mutex> lock(_mutex);
	if (_lbmap.find(key) != _lbmap.end())
	{
		auto lb = _lbmap[key];
		if (rep.hostnum == 0)
		{
			delete lb;
			_lbmap.erase(key);
		}
		else
		{
			lb->update(rep);
		}
	}
	return 0;
}
