# A C++ LoadBalance GateWay

## 1.简介

使用C++实现了一个简单的远程服务调用负载均衡系统。

![](GateWay.png)

总体架构如图。

### 网关服务器
1. 在本负载均衡系统中同坐modid（模块ID），和cmdid（子模块ID）标识一种服务，客户根据需要的服务，向网关发送modid/cmdid查询提供该服务的服务器地址
2. 网关响应客户端的查询，返回可以提供对应服务的服务器地址给客户端
3. 客户端获取服务器的真实IP后，自行对服务器发起服务请求
4. 客户端调用远程服务完成后，将调用结果（成功/失败）向网关进行汇报
5. 网关服务器除去提供查询服务外，会将上报的结果提交到结果记录服务器
6. 网关服务器定期向路由信息服务器拉去最新的路由信息
7. 网关服务器根据现有路由信息和当前时间段的调用结果进行负载均衡


### 调用结果记录服务器
1. 网关作为该服务器的客户端，向调用记录服务器发送所有客户端远程服务调用的结果
2. 调用记录服务器将网关发送的调用结果写入数据库中

### 路由信息服务器
1. 路由信息服务器装载服务配置数据库中的所有路由信息
2. 网关作为路由信息服务器的客户端，定期向路由配置服务器请求最新的路由信息
3. 路由信息服务器定期装路由配置数据库中的路由信息，当配置数据库中的数据发生变化时，会将变化的路由信息推送给订阅过该服务的网关

### 服务配置数据库
数据库中记录了所有的服务路由信息及近期内所有服务器的调用结果，可以实现一个web端或者客户端对数据库进行管理


## 2.Model
在负载均衡系统中分别实现了TCP服务器和UDP服务器。其中路由信息服务器和调用结果记录服务器是TCP服务器，网关服务器是UDP服务器。

#### 2.1 TCP Server

TCP服务器是多线程的Reactor模型服务器，其中共有两种线程，一种是Acceptor线程，另一种是IO线程，其中Acceptor线程管理监听套接字，负责接收所有的新连接请求，然后将已连接套接字分发给IO线程，由IO线程负责与客户端之间进行消息读写

网关服务器将和路由信息服务器和调用结果记录服务器进行长期通信，他们之间的连接使用TCP连接，且这两个服务器只为网关服务器提供服务，使用TCP连接负荷并不大
#### 2.2 UDP Server
UDP服务器实现比较简单，由一个单线程的事件循环实现，直接从监听套接字中读取信息，解包计算后将返回的结果写入监听套接字即可

将网关设计为UDP服务器的原因：

- 网关服务器为大量的客户端提供路由信息查询服务，仅一次消息收发就能完成，使用TCP连接还需要额外的三次握手和四次挥手，开销很大

- 且TCP连接需要占用更多的服务器资源（如文件描述符），客户端完成路由信息查询后就不再与网关进行信息交互，所以在网关服务器上没有必要使用TCP连接，为这些客户端保留句柄

- UDP协议虽然并不能提供可靠通信，但是路由信息本身也是可以容忍丢包的，对于客户端而言，如果查询请求或者查询结果没有出现了丢包导致无法获取路由信息，重新查询一次即可

#### 2.3 内存池
在Reactor模型中，实现了一个内存池，内存池首先通过malloc从标准库中获取到一定数量的内存，并将它们分割为不同大小的缓冲区为TCP连接的数据读写提供读写缓冲区。
![](memorypool.png)

当TCP连接需要缓冲区时，直接从内存池中，选择适合自己的缓冲区，并将该缓冲区从链表中移除。
当TCP连接析构时，将不再使用的缓冲区归还到对应链表的链表头中即可


## 3.调用结果记录服务器

![](reportServer.png)
调用结果记录服务器结构如图，直接在Reactor模型的TCP服务器基础上实现，每当有网关上报调用结果时，服务器将上报的数据解包分析并将结果写入数据库中。

但是写入数据库是一个阻塞IO操作，这种操作本身速度较慢，且有阻塞服务器的风险，所以并不在IO线程中完成数据库的写入，IO线程将需要写入的数据封装好，把写入任务添加到一个专门的任务线程池中，由任务线程池完成数据库的写入，这样设计不会阻塞记录服务器的事件主循环。

## 4.路由信息服务器

在路由信息服务器中有三种线程，第一类时Acceptor线程，第二类是IO线程，负责完成服务器与客户端之间的信息收发，第三类是后台线程，负责定期装载数据库中的路由信息，并负责将有变化的服务路由信息推送给订阅过该服务的客户端（通过将推送任务添加到IO线程中，不是直接向客户端发送信息）




## 5.网关服务器

![](onlygateway.png)
网关服务器负责直接和客户端进行交互，在网关服务器中共有三种线程

1. RouteMessageClientThread。该线程作为路由信息服务器的客户端，从任务队列中取得需要请求的modid和cmdid，并发送请求到路由信息服务器中拉去对应的路由信息，并且根据新获取的路由信息更新网关服务器中的路由信息。
2. ReportStatusClientThread。该线程作为调用结果记录服务器的客户端，从任务队列中取得上报的数据并发送给调用结果记录服务器
3. UDPServerThread。该线程是网关服务器用来与客户端交互的线程，通过UDP服务线程获取客户端的请求，如果路由信息存在则直接返回给客户端，如果路由信息不存在则将获取路由信息的任务添加到任务队列中，等到RouteMessageClientThread消费。同时UDP服务器接收客户端的上报的调用结果，将调用结果信息添加到任务队列中，等到ReportStatusClientThread消费。
4. 在实际的实现中，网关服务器中共使用了三个UDP服务器线程，分别监听三个不同的套接字，管理不同的路由信息，客户端根据（modid+cmdid）%3的结果，选择对应的UDP服务器进行查询

## 6.负载均衡算法

#### 6.1 UDP服务器选择

网关服务器中会运行三个UDP服务器，分别监听三个套接字，编号为0，1，2号服务器，客户端根据(modid+cmdid)%3的结果，选择对应编号的UDP服务器进行路由信息查询

#### 6.2 网关负载均衡

网关服务器中的三个UDP服务器，它们只管理(modid+cmdid)%3等于自身编号的服务路由信息。
![](routemap.png)

每个LoadBalance中保存着该cmdid和modid所对应的所有主机地址，且将他们分别放置在不同的队列中，一个是空闲队列，一个是超载队列。
![](loadbalance.png)

当每次客户端来获取路由信息时，UDP服务器根据请求的modid和cmdid找到对应的LoadBalance，并从空闲队列的队首中取出一个主机信息发送给客户端。然后将队首的主机信息放到空闲队列尾端中。

#### 6.3 超载队列尝试

对于负荷过大的服务器主机，网关会将其放到超载队列的队尾。对于超载的服务器，因为其负荷过大，容易调用失败，所以一般不会对其进行调用。但是也需要给超载队列中的主机一个尝试的机会。每当一个modid/cmdid所表示的服务被调用了probe次后，就尝试调用一次超载队列中的主机。probe的值可以在配置文件中修改

#### 6.4 超载/空闲判别

客户端每次对远程主机调用完成后会向网关服务器上报本次调用结果，即调用成功还是失败。网关服务器会将调用结果继续发送给report服务器让其写入到数据库中。但是网关服务器本身也会对调用结果进行记录以便用于判别队列是否超载/空闲。

##### 判别超载：

- 如果一个主机连续调用失败超过一定的次数则可以认为它已经超载
- 如果一个主机的失败率高于一个阈值，则可以认为它已经超载

对于超载的主机，将其从IdleList中移除，并添加到OverLoadList的队尾

##### 判别空闲：

- 如果一个主机连续调用成功超过一定的次数则可以认为它已经空闲
- 如果一个主机的成功率高于一个阈值，则可以认为它已经空闲

对于已经空闲的主机，将其从OverLoadList中移除，并添加到IdleList的队尾


##### 初始化虚拟成功/失败次数

如果一个主机刚开始运行，被调用了两次，一次成功，一次失败，其失败率为50%，很有可能会将这个主机判别为超载主机，这是不合理的。所以在一个主机初始化时，应该为其设计一个初始化的虚拟成功次数，如100次，这样不会因为初期的一两次调用失败就导致主机被判别为超载状态。

对于设置为超载的主机也是一样，为其设置一个初始化的虚拟失败次数，这样不会因为偶尔的一两次成功就将其判别为空闲主机。

如果不设置初始化次数，那么主机很有可能在空闲/超载在状态之间发生抖动。

##### 时间窗口

一个主机如果已经被调用了100000次，调用一直成功，但是在近期内逐渐出现调用失败，一共调用失败了500次，但是500/100000=0.5%，很有可能达不到失败率的阈值。为了让网关更快的感觉到主机的超载，应该每隔一段时间就将主机的调用次数恢复到初始化的状态，这样主机一旦超载，可以更快的被网关所察觉并调整主机的状态。

比如网关服务器每隔15秒钟就将主机重置为初始化状态，成功次数设置为初始化虚拟成功次数，失败次数归零，重新开始计数。

##### 时间窗口失败率

因为每隔一段时间，主机的调用状态就会切换到初始化状态，当调用次数不够的时候，可能难以判别到主机超载。比如一台主机的初始化成功次数为100次，在这个时间窗口内共调用了10次主机，其中9次失败，1次成功，但是使用之前的失败率计算方式结果为9/110，不到10%，很有可能无法判别主机超载，所以对于一段时间窗口，应该计算其真实的失败率，即9/10=90%，如此高的失败率，很容易判断出主机超载。

即主机每一次调用之后，网关服务器会计算其成功率/失败率，在计算中会添加初始化的虚拟成功/失败次数进行计算，以判别主机的空闲/超载状态。

而在一段时间窗口完结后，网关服务器会计算在时间窗口内的真实失败率（不添加初始化次数计算），根据失败率判断主机是否超载。

## 7.Build
在项目根目录中:

	make compile
	#生成目标文件
	make link
	#生成可执行文件

生成文件在app文件夹中

## 8.Usage

	./GenerateConfig
	#生成配置文件
	
	#可以对配置文件进行修改，也可以直接运行
	
	./dnsServer
	#运行路由信息服务器
	
	./reportServer 
	#运行调用结果记录服务器

	./AgentServer
	#运行负载均衡服务器

## 9.ConfigFile

配置文件使用了json字符串，内容如下：

	{
		#dns服务器配置
	   "dns" : {
		  #dns服务器IP
	      "ip" : "127.0.0.1",
		  #dns服务器允许的最大连接数
		  "maxconn" : 1024,
		  #dns服务器的端口
	      "port" : 9000,
		  #dns服务器的IO线程数量
	      "threadnum" : 3
	   },
		#网关服务器配置
	   "loadbalance" : {
		  #连续失败次数，可以判别超载
	      "continueerr" : 15,
		  #连续成功次数，可以判别空闲		  
	      "continuesucc" : 15,
		  #失败率，可以判别超载
	      "errrate" : 0.10,
		  #空闲服务器的时间窗口
	      "idletimeout" : 15,
		  #初始化失败次数
	      "initerr" : 5,
		  #初始化成功次数
	      "initsucc" : 100,
		  #网关服务器的IP
	      "localip" : "127.0.0.1",
		  #超载服务器的时间窗口
	      "overloadtimeout" : 15,
		  #每probenum次，给超载队列中的服务器一次尝试机会
	      "probenum" : 10,
		  #成功率，可以判别空闲
	      "succrate" : 0.950,
		  #窗口时间内的失败率，可以判别超载
	      "windowerrrate" : 0.70
	   },
	   #数据库的地址和配置，但是暂时未使用，在程序中将数据库配置写死了，reporter服务器连接本地数据库
	   "mysql" : {
	      "dbname" : "dns",
	      "ip" : "127.0.0.1",
	      "port" : 3306,
	      "pwd" : "111111",
	      "user" : "root"
	   },
	   #reactor模型服务器测试使用
	   "reactor" : {
	      "ip" : "127.0.0.1",
	      "maxconn" : 1024,
	      "port" : 8000,
	      "threadnum" : 3
	   },
	   #report服务器配置
	   "report" : {
	      "ip" : "127.0.0.1",
	      "maxconn" : 1024,
	      "port" : 9001,
	      "threadnum" : 3,
		  #上面属性意义同dns中配置
		  #任务线程池线程数，专门用于向数据库写入数据
	      "writethread" : 3
	   }
	}


## 10.数据库

数据库中共有四个表，建表语句在sql文件夹中init.sql中

#### 注册/修改/删除服务
当需要在负载均衡服务器中增加新的服务器或者修改原有服务器配置时，使用如下sql语句：

	SET @time = UNIX_TIMESTAMP(NOW());
	
	INSERT INTO RouteData(modid, cmdid, serverip, serverport) VALUES(1, 1, 3232235953, 9999);
	UPDATE RouteVersion SET version = @time WHERE id = 1;
	
	INSERT INTO RouteChange(modid, cmdid, version) VALUES(1, 1, @time);

即将服务器的配置数据写入RouteData中，将RouteVersion中的版本信息更新为修改时的时间戳，在RouteChange表中记录的是本次修改的服务器modid/cmdid

删除或者修改时也如此修改，在修改RoutaData的同时更新RouteVersion版本信息，并将修改的服务表示符modid/cmdid写入到RouteChange中。