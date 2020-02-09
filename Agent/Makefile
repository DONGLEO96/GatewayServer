CC=g++
FLAGS=-c -Wall -O2 -g -std=c++11

all:
	$(CC) $(FLAGS) AgentServer.cpp -o ../test/AgentServer.o
	$(CC) $(FLAGS) LoadBalance.cpp -o ../test/LoadBalance.o
	$(CC) $(FLAGS) ApiClient.cpp -o ../test/ApiClient.o
	$(CC) $(FLAGS) HostDetailedInfo.cpp -o ../test/HostDetailedInfo.o
	$(CC) $(FLAGS) RouteLoadBalance.cpp -o ../test/RouteLoadBalance.o
	$(CC) $(FLAGS) example/AgentClient.cpp -o ../test/AgentClient.o
	$(CC) $(FLAGS) example/qpstest.cpp -o ../test/qpstest.o
