#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <sched.h>
#include <error.h>
#include <assert.h>
#include "../common/common.h"
#include "../common/epoll.h"
#include "../common/measure.h"

char *server_ip = DEFAULT_SERVER_ADDR;
int server_port = DEFAULT_PORT;
int num_req = NUM_REQ;
bool with_ack = false;
bool with_echo = true;
bool run_throughput = false;
int run_latency = 0; // by default, we run the throughput benchmark
int payload_size = DEFAULT_PAYLOAD_SIZE;
int conn_num = CONNECT_NUMBER; //connect number default 1
struct CON_INFO *fd_con_info = NULL;
size_t start_cycles = 0;
size_t end_cycles = 0;
size_t TOTAL_RECV_PACKAGET = 0;
size_t TOTAL_BANDWIDTH_TIME = 0;
size_t run_num = 0;

void usage(char *program);
void parse_args(int argc, char *argv[]);
int do_read(int epfd, int sockfd, char *buf);
static void do_epoll(int listenfd);
static void
handle_epoll_events(int epollfd, struct epoll_event *events, int num, int listenfd, char *buf);
static void handle_accpet(int epollfd, int listenfd, char *buf);
static void handle_recv(int epfd, int fd, char *buf);
static bool do_send(int epfd, int fd, char *buf);
void throughput_report_perf(size_t duration, size_t pakcnt);

void usage(char *program)
{
    printf("Usage: \n");
    printf("%s\tstart a server and wait for connection\n", program);
    printf("Options:\n");
    printf(" -p <port>      listen on port number(default %d)\n", DEFAULT_PORT);
    printf(" -a <addr>      connect to server addr(default %s)\n", DEFAULT_SERVER_ADDR);
    printf(" -c <connection number>   connection size \n");
    printf(" -s <size>      payload size(default %d)\n", DEFAULT_PAYLOAD_SIZE);
    printf(" -n <requests>  the number of request(default %d)\n", NUM_REQ);
    printf(" -w             transfer data with ack\n");
    printf(" -l             run the lantency benchmark :->nothing\n");
    printf(" -t             run the throughput benchmark\n");
    printf(" -e             TURN OFF with echo!!! Please use in latency write!!!\n");
    printf(" -h             display the help information\n");
}

void parse_args(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (strlen(argv[i]) == 2 && strcmp(argv[i], "-p") == 0)
        {
            if (i + 1 < argc)
            {
                server_port = atoi(argv[i + 1]);
                if (server_port < 0 || server_port > 65535)
                {
                    printf("invalid port number\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read port number\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-a") == 0)
        {
            if (i + 1 < argc)
            {
                server_ip = argv[i + 1];
                i++;
            }
            else
            {
                printf("cannot read server addr\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-s") == 0)
        {
            if (i + 1 < argc)
            {
                payload_size = atoi(argv[i + 1]);
                if (payload_size <= 0)
                {
                    printf("invalid payload size\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read payload size\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-c") == 0)
        {
            if (i + 1 < argc)
            {
                conn_num = atoi(argv[i + 1]);
                if (conn_num <= 0)
                {
                    printf("invalid numbers of connection\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read numbers of connection\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-w") == 0)
        {
            with_ack = true;
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-e") == 0)
        {
            with_echo = false;
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-l") == 0)
        {
            run_latency = 1;
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-t") == 0)
        {
            run_throughput = true;
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-n") == 0)
        {
            if (i + 1 < argc)
            {
                num_req = atoi(argv[i + 1]);
                if (num_req <= 0)
                {
                    printf("invalid the number of requests\n");
                    exit(EXIT_FAILURE);
                }
                i++;
            }
            else
            {
                printf("cannot read the number of requests\n");
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
        else if (strlen(argv[i]) == 2 && strcmp(argv[i], "-h") == 0)
        {
            usage(argv[0]);
            exit(EXIT_SUCCESS);
        }
        else
        {
            printf("invalid option: %s\n", argv[i]);
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[])
{

    int listen_fd;
    struct sockaddr_in address;

    parse_args(argc, argv);

    //in order to store the connection information: recv_size and send_size
    fd_con_info = init_conn_info(conn_num + 100); // initialize connection infomation

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("create socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(server_ip);
    address.sin_port = htons(server_port);
    printf("bind to %s:%d\n", server_ip, server_port);
    if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)))
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(listen_fd, SOCK_BACKLOG_CONN) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    printf("server listens on 0.0.0.0:%d\n", server_port);
    do_epoll(listen_fd);
    close(listen_fd);
    return 0;
}

void do_epoll(int listen_fd)
{
    struct epoll_event wait_event[EPOLLEVENTS];
    char buf[BUFSIZE];

    int epfd;
    epfd = epoll_create(FDSIZE);
    if (epfd < 0)
    {
        perror("epoll_create failed");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    set_nonblock(listen_fd);
    epoll_add_event(epfd, listen_fd, EPOLLIN);

    while (true)
    {
        int ret;
        //puts("[server] do epoll epolling\n");
        ret = epoll_wait(epfd, wait_event, EPOLLEVENTS, -1);
        if (ret < 0)
        {
            perror("epoll_wait()");
        }
        handle_epoll_events(epfd, wait_event, ret, listen_fd, buf);
    }
    close(epfd);
}

static void
handle_epoll_events(int epfd, struct epoll_event *wait_events, int ret, int listen_fd, char *buf)
{
    int fd;
    for (int i = 0; i < ret; ++i)
    {
        fd = wait_events[i].data.fd;
        if (wait_events[i].events & (EPOLLERR | EPOLLHUP))
        {
            perror("wait_event: one fd error\n");
            epoll_delete_event(epfd, fd, EPOLLIN);
            close(fd);
        }
        else if ((fd == listen_fd) && wait_events[i].events & EPOLLIN)
            handle_accpet(epfd, listen_fd, buf);
        else if (wait_events[i].events & EPOLLIN)
            handle_recv(epfd, fd, buf);
    }
}

static void handle_accpet(int epfd, int listen_fd, char *buf)
{
    int sockfd;
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    memset(&cli_addr, 0, sizeof(cli_addr));

    sockfd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len);
    if (sockfd < 0)
    {
        perror("accept failed");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("fd=%d\n", sockfd);
        printf("accept a new client: %s:%d\n", inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);
        set_nonblock(sockfd);
        fd_con_info[sockfd].recv_cnt = 0;
        fd_con_info[sockfd].send_cnt = 0;
        fd_con_info[sockfd].total_throughput_time = 0;
        fd_con_info[sockfd].temp_throughput_time = 0;
        epoll_add_event(epfd, sockfd, EPOLLIN);
    }
    return;
}

static void handle_recv(int epfd, int fd, char *buf)
{
    if (fd_con_info[fd].recv_cnt == 0)
    {
        if (do_read(epfd, fd, buf))
        {
            if (run_throughput)
                fd_con_info[fd].temp_throughput_time = get_cycles();
            do_send(epfd, fd, buf);
        }
        return;
    }
    //printf("recving %ld\n",TOTAL_RECV_PACKAGET/payload_size);

    start_cycles = get_cycles();
    int ret = do_read(epfd, fd, buf);
    end_cycles = get_cycles();
    TOTAL_BANDWIDTH_TIME += end_cycles - start_cycles;
    if (ret == 0)
    {
        perror("error in handle_recv\n");
        return;
    }
    if (ret == 1 && with_ack)
    {
        if (run_throughput)
        {
            fd_con_info[fd].total_throughput_time += (end_cycles - fd_con_info[fd].temp_throughput_time);
            fd_con_info[fd].temp_throughput_time = get_cycles();
        }
        do_send(epfd, fd, buf);
    }
   printf("[+] recv packet %ld\n",TOTAL_RECV_PACKAGET/payload_size);
    if (TOTAL_RECV_PACKAGET >= num_req * payload_size && !run_throughput)
    { //solve bandwidth
        throughput_report_perf(TOTAL_BANDWIDTH_TIME, TOTAL_RECV_PACKAGET / payload_size);
        TOTAL_BANDWIDTH_TIME = 0;
        TOTAL_RECV_PACKAGET = 0;
        if(run_num>= num_req){
            exit(0);
        }
    }

    return;
}

int do_read(int epfd, int sockfd, char *buf)
{
    int n = 0;
    int read_cnt = payload_size;
    if (run_throughput)
    {
        read_cnt = 1;
    }
    while (n < read_cnt)
    {
        int t = 0;
        t = read(sockfd, buf, read_cnt);
        if (t == -1)
        {
            perror("read error\n");
            epoll_delete_event(epfd, sockfd, EPOLLIN);
            close(sockfd);
            return 0;
        }
        else if (t == 0)
        {
            perror("client close.\n");
            epoll_delete_event(epfd, sockfd, EPOLLIN);
            close(sockfd);
            fd_con_info[sockfd].recv_cnt = 0;
            return 0;
        }
        n += t;
    }

    TOTAL_RECV_PACKAGET += n;

    fd_con_info[sockfd].recv_cnt += n;

    if (fd_con_info[sockfd].recv_cnt > num_req * read_cnt)
    {
        if (run_throughput)
        {
            fd_con_info[sockfd].total_throughput_time += (get_cycles() - fd_con_info[sockfd].temp_throughput_time);
        }
        epoll_delete_event(epfd, sockfd, EPOLLIN);
        throughput_report_perf(fd_con_info[sockfd].total_throughput_time, num_req);
        close(sockfd);
        puts("normal close\n");
        run_num++;
        fd_con_info[sockfd].recv_cnt = 0;
        fd_con_info[sockfd].total_throughput_time = 0;
        return 2;
    }
    return 1;
}

static bool do_send(int epfd, int fd, char *buf)
{
    int nwrite = 0;
    int send_cnt = payload_size;
    if (!with_echo)
    {
        send_cnt = 1;
    }
    while (nwrite < send_cnt)
    {
        int n = 0;
        n = write(fd, buf, send_cnt);
        if (n == -1)
        {
            perror("write error\n");
            epoll_delete_event(epfd, fd, EPOLLIN);
            close(fd);
            return false;
        }
        nwrite += n;
    }

    return true;
}

void throughput_report_perf(size_t duration, size_t pakcnt)
{

    double cpu_mhz = get_cpu_mhz();
    double total_time = (double)duration / cpu_mhz;
    double tput1 = (double)pakcnt / total_time * 1000000;
    // bandwidth
    printf("\n\n[+] bandwith : recv buff = %d, total elapse_time = %.2lf, trans bytes = %.2lf Mb, BW (Gbits/s) = %.2lf \n\n", payload_size, total_time, (double)pakcnt * payload_size / (1024 * 1024), (double)pakcnt * payload_size / (total_time * 128 * 1024 * 1024));

    // throughtput
    printf("[+] throughput : requests = %d, payload size = %d, total time = %.2f us, HROUGHPUT = %.2f REQ/s\n\n", num_req, payload_size, total_time, tput1);
    return;
}