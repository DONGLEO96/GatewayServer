SUBDIRS=Agent json Reactor reporter util dns
CC=g++
LIBS=-lpthread -lmysqlclient

compile:
	rm -rf test
	mkdir test
	@list='$(SUBDIRS)';for subdir in $$list;do\
		$(MAKE) -C $$subdir;\
	done

link:
	rm -rf app
	mkdir app
	rm -rf test/lib
	mkdir test/lib
	ar crv ./test/lib/reactor.a ./test/EventLoop.o ./test/InputBuff.o ./test/IObuf.o ./test/json_reader.o ./test/json_writer.o ./test/json_value.o ./test/OutputBuff.o ./test/ReactorBuff.o ./test/TcpClient.o ./test/TcpConnection.o ./test/TcpServer.o ./test/ThreadPool.o ./test/UdpClient.o ./test/UdpServer.o ./test/BuffPool.o
	g++ ./test/AgentClient.o ./test/ApiClient.o ./test/json_reader.o ./test/json_value.o ./test/json_writer.o -o ./app/AgentClient
	g++ ./test/dnsClient.o ./test/lib/reactor.a  $(LIBS) -o ./app/dnsClient
	g++ ./test/reportClient.o ./test/lib/reactor.a $(LIBS) -o ./app/reportClient
	g++ ./test/uclient.o ./test/lib/reactor.a $(LIBS) -o ./app/UdpServer
	g++ ./test/AgentServer.o ./test/lib/reactor.a ./test/RouteLoadBalance.o ./test/LoadBalance.o ./test/ConfigFile.o ./test/HostDetailedInfo.o $(LIBS) -o ./app/AgentServer
	g++ ./test/dnsService.o ./test/Subscribe.o ./test/DnsRoute.o ./test/lib/reactor.a ./test/ConfigFile.o $(LIBS) -o ./app/dnsServer
	g++ ./test/reportService.o ./test/lib/reactor.a ./test/StoreReport.o ./test/ConfigFile.o -lpthread -lmysqlclient -o ./app/reportServer
	g++ ./test/GenerateFile.o ./test/ConfigFile.o ./test/lib/reactor.a -o ./app/GenerateConfig 
	g++ ./test/uclient.o ./test/lib/reactor.a $(LIBS) -o ./app/UdpClient

clean:
	rm -rf test
	rm -rf app
