### NTsocket

> 多路复用技术：socket _ select poll epoll
> 基于epoll实现并发连接socket的数据传输性能测试：
>
> 1. Latency: Average latency, tail latency
> 2. Throughput: msgs/s, bytes/s
>
> client实现为一个命令：输入参数中包含了：payload size, connection number, number of request
> num_partitions(针对ntp组件)，packet size 对于传输性能的影响(修改配置文件)。
> TCP socket编程：基于epoll 
> payload size: request/response的大小
> 输入参数：number of request
> latency 与throughput这是两个不同的东西


> 设置非阻塞模式
   flags = fcntl(sockfd, F_GETFL, 0);   

      fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);  

      if((iret = fcntl(nfd, F_SETFL, O_NONBLOCK)) < 0)

server IP:
ntb-server1@10.176.22.210
ntb-server2@10.176.22.211

>
payload size: 8 16 32 64 128 256 512 1024 2048 4096 8192
连接数量: 1 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32


在处理throughout的时候，可以选用所有的连接都是并发的，我们可以记录每一个连接的信息，使用fd_conn_info.
在处理latency的时候，还是使用同样的方法，但是需要代码重写


>
1. epoll基于kernel socket的性能测试
2. NTSocks feature/epoll分支中epoll的功能测试，有问题群里反馈。基本没问题后得出一组性能数据
3. NTSocks feature/ntp-partition分支的功能测试  