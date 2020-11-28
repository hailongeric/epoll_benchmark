#!/bin/bash

# payload_size_arr=(64)
payload_size_arr=(8 16 32 64 128 256 512 1024 2048 4096 8192)
# 16384 32768 65536 131072 262144 524288 1048576
metric_arr=(0)
# metric_arr=(0)  latency -l -w -e -
# throughput -t -w -s '' -c '' 
thread_arr=(1 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32)
# thread_arr=(12)
# n=2000
n_arr=(10000 100000)
port=8099

num_partitions=1

NTS_ROOTPATH_210=/home/ntb-server1/NTSock
NTS_ROOTPATH_211=/home/ntb-server2/NTSock

for metric in ${metric_arr[@]};
do

for payload_size in ${payload_size_arr[@]};
do

for thread in ${thread_arr[@]};
do

for n in ${n_arr[@]};
do

pnum=`lsof -i:$port | wc -l`
while [ $pnum -ne 0 ]
do
    let port++
    pnum=`lsof -i:$port | wc -l`
done

echo "Metric = $metric, Payload Size = $payload_size, n = $n, # of thread = $thread, Port = $port"

# pidof ntb_fwd | xargs kill -9

# Start 210 Proxy
# echo "Start 210 Proxy"
echo "==================PROXY 210==========================="
/home/ntb-server1/hailong/NTSock/proxy.sh > ntp.log &
sleep 5
# ls /dev/shm
# echo "========================MONITOR 210====================="
# echo "Start 210 Monitor"
# ls /dev/shm
# ~/monitor.sh &
# sleep 5

# Start 211 Proxy
echo "Start 211 Proxy"
ssh ntb-server2@10.176.22.211 << eeooff 
echo "==================PROXY 211==========================="
cd /home/ntb-server2/hailong/NTSock
nohup /home/ntb-server2/hailong/NTSock/proxy.sh > ntp.log &
sleep 5
# ls /dev/shm
# sudo touch /dev/shm/test
# ls /dev/shm
# echo `pidof ntb_proxy`
# echo "========================PROXY 211====================="
# ls /dev/shm
# echo `pidof ntb_proxy`
# nohup ~/monitor.sh >ntm.log &
# sleep 5
# nohup ~/nts_client.sh &
exit
eeooff

# ssh ntb-server2@10.176.22.211 "ls /dev/shm; ls /home"<< eeooff
# echo "LOGIN-----------------------------------------"
# ls /dev/shm
# exit
# eeooff


# sleep 5

# Start 210 Monitor
echo "--------------------------Start 210 Monitor------------------------"
ls /dev/shm
/home/ntb-server1/hailong/NTSock/monitor.sh > ntm.log &
sleep 2
echo "========================End MONITOR 210============================"

# # Start 211 Monitor
echo "--------------------------Start 211 Monitor------------------------"
ssh ntb-server2@10.176.22.211 << eeooff
# ls /dev/shm
# echo `pidof ntb_proxy`
nohup /home/ntb-server2/hailong/NTSock/monitor.sh > ntm.log &
sleep 2
echo "========================End MONITOR 211============================"
exit
eeooff

sleep 5

# Run 210 (Server side)
echo "--------------------------Start 210 NTS Server APP------------------------"
# ~/nts_server.sh > ~/ntb_benchmark_${metric}_${payload_size}_${n}_${thread}.txt &
cd /home/ntb-server1/hailong/NTSock/.eric/
LD_PRELOAD=/home/ntb-server1/hailong/NTSock/libnts/build/src/lib/.libs/libnts.so ./epoll-server -a 10.176.22.210 -p $port -l -w -s $payload_size -n $n -c $thread > /home/ntb-server1/hailong/NTSock/data/epoll_benchmark_${metric}_${payload_size}_${n}_${thread}.txt &
sleep 3
echo "--------------------------End 210 NTS Server APP------------------------"

# Run 211
sleep 2
ssh ntb-server2@10.176.22.211 << eeooff
echo "--------------------------Start 211 NTS Client APP------------------------"
sleep 2
# ~/nts_client.sh > ~/ntb_benchmark_${metric}_${payload_size}_${n}_${thread}.txt &
cd /home/ntb-server2/hailong/NTSock/.eric/
LD_PRELOAD=/home/ntb-server2/hailong/NTSock/libnts/build/src/lib/.libs/libnts.so ./epoll-client -a 10.176.22.210 -p $port -l -w -s $payload_size -n $n -c $thread > /home/ntb-server2/hailong/NTSock/data/epoll_benchmark_${metric}_${payload_size}_${n}_${thread}.txt &
sleep 25
if pgrep epoll-client; then pkill epoll-client; fi
echo "--------------------------End 211 NTS Client APP------------------------"
exit
eeooff

# sleep 10

# Kill 210 Server, monitor, proxy
echo "--------------------------Start kill 210 NTSocks Process------------------------"
if pgrep epoll-server; then sudo pkill epoll-server; fi
if pgrep ntb-monitor; then pkill ntb-monitor; fi
if pgrep ntb_proxy; then sudo pkill ntb_proxy; fi

sleep 2
echo "--------------------------End kill 210 NTSocks Process------------------------"

# Kill 211 Client, monitor, proxy
ssh ntb-server2@10.176.22.211 << eeooff
echo "--------------------------Start kill 211 NTSocks Process------------------------"
# if pgrep epoll-client; then pkill epoll-client; fi
if pgrep ntb-monitor; then pkill ntb-monitor; fi
if pgrep ntb_proxy; then sudo pkill ntb_proxy; fi
sleep 2
echo "--------------------------End kill 211 NTSocks Process------------------------"
exit
eeooff

echo "************************* [END] Metric = $metric, Payload Size = $payload_size, n = $n, # of thread = $thread, Port = $port*****************************"

sleep 5

done
done
done
done

# pidof "./bin/client" | xargs kill -9
# pidof "ntb-monitor" | xargs kill -9
# pidof "ntb_proxy" | sudo xargs kill -9