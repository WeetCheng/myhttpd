/**
 * @Author: cheng
 * @Date: 下午12:14 18-9-7
 * @Description: 
 * @Version: 
 */

#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/tcp.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <pthread.h>
#include "handle.h"
#include "config.h"
#include "mmanage.h"
#include "handle_core.h"
#include "my_string.h"
#include "handle_read.h"
#include "handle_write.h"

#define OPEN_FILE 50000

enum SERVE_STATUS
{
    CLOSE_SERVE = 1,
    RUNNING_SERVER = 0
};
char *cache_file = NULL;
char *website_root_path = NULL;
static int *epfd_group = NULL;  /* Workers' epfd set */
static int epfd_group_size = 0; /* Workers' epfd set size */
static enum SERVE_STATUS server_status = RUNNING_SERVER;
static int worker_num = 0;    /* 工作线程数 */
static int listener_num = 1;    /* 监听线程数 */
static conn_client *clients;   /* client集 */

void optimizes(int file_dsption)
{
    const int on = 1;
    /* 允许与其他程序共用socket */
    if (0 != setsockopt(file_dsption, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
    {
        perror("REUSEADDR: ");
    }
    /* TCP连接不适用Nagle算法 */
    if (setsockopt(file_dsption, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)))
        perror("TCP_NODELAY: ");

}

int set_nonblock(int fd)
{
    int old_flg;
    old_flg = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, old_flg | O_NONBLOCK);
    return old_flg;
}

int create_listenfd(const char *restrict host_addr, const char *restrict port, int *restrict sock_type)
{

    struct addrinfo info_of_host;   // 期望的addrinfo类型
    struct addrinfo *result;
    struct addrinfo *p;

    memset(&info_of_host, 0, sizeof(info_of_host));
    info_of_host.ai_family = AF_UNSPEC; // 网络协议族没有明确定义，都可以；
    info_of_host.ai_flags = AI_PASSIVE; // 被动连接，一般是服务器端使用的，用于bind
    info_of_host.ai_socktype = SOCK_STREAM; // TCP连接

    /* 根据IP地址，端口获取本地服务器的addrinfo, 用链表的组织方式存在result中 */
    int errcode;
    if (0 != (errcode = getaddrinfo(host_addr, port, &info_of_host, &result)))
    {
        fputs(gai_strerror(errcode), stderr);
        return ERR_GETADDRINFO;
    }

    int listenfd = 0;
    for (p = result; p != NULL; p = p->ai_next)
    {
        /* 创建socket，将用来监听 */
        listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (-1 == listenfd)
            continue;
        /* 设置socket允许共用，并且TCP连接不使用Nagle算法 */
        optimizes(listenfd);

        /* 绑定监听socket与IP地址　*/
        if (-1 == bind(listenfd, p->ai_addr, p->ai_addrlen))
        {
            close(listenfd);
            continue;
        }

        /* 如果已经成功创建一个用于监听的socket，就不用找了 */
        break;
    }
    freeaddrinfo(result);
    /* 如果一直无法创建监听socket，那么报错 */
    if (NULL == p)
    {
        fprintf(stderr, "In %s, Line: %d\nError Occur while Open/Binding the listen fd\n", __FILE__, __LINE__);
        return ERR_BINDIND;
    }

    /* 告诉调用者最终创建的监听socket的网络协议族是什么 */
    *sock_type = p->ai_family;

    /* 将监听socket设为非阻塞 */
    set_nonblock(listenfd);

    return listenfd;
}


static void shutdown_server(int arg)
{
    server_status = CLOSE_SERVE;
    return;
}

/*
 * Modify including Re-register Each sock while it woke up by epfd
 * */
static inline void mod_event(int epfd, int fd, int event_flag)
{
    struct epoll_event event = {0};
    event.data.fd = fd;
    event.events |= event_flag;
    if (-1 == epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event))
    {
        perror("epoll_ctl MOD: ");
        exit(-1);
    }
}

/**
* @Description: 为worker线程做准备工作，创建一组worker的epoll，并为它们的连接准备空间
* @Param:
* @return:
*/
static void prepare_workers(const config_t *config)
{
    /* 打开服务器根路径下的/index.html文件 */
    char file[266] = {'\0'};
    snprintf(file, 266, "%s%s", config->root_path, "index.html");
    int fd = open(file, O_RDONLY);
    assert(fd > 0);

    /* 将打开的文件内容映射到内存中 */
    cache_file = mmap(NULL, 51, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(cache_file != NULL);

    /* 将创建一组epoll */
    epfd_group_size = worker_num;

    /* 先为epoll组分配空间 */
    epfd_group = Malloc(epfd_group_size * (sizeof(int)));
    if (NULL == epfd_group)
        exit(-1);


    /* 创建一组epoll */
    for (int i = 0; i < epfd_group_size; ++i)
    {
        /* 创建epoll */
        epfd_group[i] = epoll_create1(0);
        if (-1 == epfd_group[i])
            exit(-1);

        /* 为当前的epoll分配空间，用于存储其clients,并初始化所有的client */
        clients = Malloc(OPEN_FILE * sizeof(conn_client));
        if (NULL == clients)
            exit(-1);

        else
        {
            for (int j = 0; j < OPEN_FILE; ++j)
            {
                clients[j].r_buf = MAKE_STRING_S("");
                clients[j].w_buf = MAKE_STRING_S("");
                clients[j].conn_res.requ_res_path = MAKE_STRING_S("");
            }
        }

        website_root_path = Malloc(strlen(config->root_path) + 1);
        if (NULL == website_root_path)
        {
            exit(-1);
        } else
        {
            strncpy(website_root_path, config->root_path, strlen(config->root_path));
        }
    }
}

/**
* @Description: 清空连接信息
* @Param:
* @return:
*/
static void clear_clients(conn_client *clear)
{
    clear->file_dsp = -1;
    clear->conn_res.conn_linger = 0;
    //- clear->read_offset = 0;
    clear->r_buf_offset = 0;
    clear->w_buf_offset = 0;
    clear_string(clear->r_buf);
    clear_string(clear->w_buf);
    //clear->r_buf->use->clear(clear->r_buf);
    //clear->w_buf->use->clear(clear->w_buf);
    clear_string(clear->conn_res.requ_res_path);
    //clear->conn_res.requ_res_path->use->clear(clear->conn_res.requ_res_path);
}

/**
* @Description: 销毁所有的epoll，销毁clients结构体，销毁服务器端的root路径
* @Param:
* @return:
*/
static inline void destroy_resouce()
{
    Free(epfd_group);
    Free(clients);
    Free(website_root_path);
}

/**
* @Description: 在epoll中添加新的关注的文件描述符。默认event是边缘触发的，并且可以接收对端断开连接 这一event
* @Param:
* @return:
*/
static inline void add_event(int epfd, int fd, int event_flag)
{
    struct epoll_event event = {0};
    event.data.fd = fd;
    event.events = event_flag | EPOLLET | EPOLLRDHUP;
    if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event))
    {
        perror("epoll_ctl ADD: ");
        exit(-1);
    }
}


/**
* @Description: 监听epoll线程，当有连接请求时，建立连接，并设为非阻塞socket；然后将连接socket添加到工作epoll中
* @Param: arg是监听epoll的文件描述符
* @return:
*/
static void *listen_thread(void *arg)
{
    int listen_epfd = *(int *) arg;
    struct epoll_event new_client = {0};
    /* Adding new Client Sock to the Workers' thread */
    int balance_index = 0;
    while (server_status != CLOSE_SERVE)
    {
        /* epoll阻塞直到有event产生（即有连接请求）或超时(2000ms) */
        int is_work = epoll_wait(listen_epfd, &new_client, 1, 2000);
        int sock = 0;

        /* 如果有连接请求，那么accept，并将连接设为非阻塞，并将连接socket添加到某一个worker epoll中 */
        while (is_work > 0)
        {
            /* new_client中记录了发起连接请求的客户端socket */
            sock = accept(new_client.data.fd, NULL, NULL);

            /* 将连接设为非阻塞 */
            if (sock > 0)
            {
                set_nonblock(sock);
                clear_clients(&clients[sock]);
                clients[sock].file_dsp = sock;
                add_event(epfd_group[balance_index], sock, EPOLLIN);
                balance_index = (balance_index + 1) % worker_num; // 这次添加连接socket到 工作epoll 1中，下次添加到2中....
            }
                /* 如果连接失败 */
            else
                break;
        } /* new Connect */
    }/* main while */

    /* 如果关掉了服务，才会执行到这里，关闭监听epoll，并退出线程 */
    close(listen_epfd);
    pthread_exit(0);
}

/**
* @Description: 工作epoll线程
* @Param:
* @return:
*/
static void *workers_thread(void *arg)
{
    int deal_epfd = *(int *) arg;
    struct epoll_event new_apply = {0};
    while (server_status != CLOSE_SERVE)
    {
        /* 等待产生event */
        int is_apply = epoll_wait(deal_epfd, &new_apply, 1, 2000);

        /* 产生event后，看是哪个socket产生，判断产生的event是不是可读event，如果是处理读event */
        if (is_apply > 0)
        {
            /* 哪个socket产生event */
            int sock = new_apply.data.fd;
            conn_client *new_client = &clients[sock];

            /* 产生的event是否是可读event */
            if (new_apply.events & EPOLLIN)
            {
                int err_code = handle_read(new_client);
                /* 处理完event后，需要重新将要关注的socket注册到epoll中 */
                if (err_code == MESSAGE_INCOMPLETE)
                {
                    mod_event(deal_epfd, sock, EPOLLIN);
                    continue;
                } else if (err_code != HANDLE_READ_SUCCESS)
                {
                    close(sock);
                    continue;
                }

                /* Try to Send the Message immediately */
                err_code = handle_write(new_client);
                if (HANDLE_WRITE_AGAIN == err_code)
                {
                    /* TCP Write Buffer is Full */
                    mod_event(deal_epfd, sock, EPOLLOUT);
                    continue;
                } else if (HANDLE_WRITE_FAILURE == err_code)
                {
                    /* Peer Close */
                    close(sock);
                    continue;
                } else
                {
                    /* Write Success */
                    if (1 == new_client->conn_res.conn_linger)
                        mod_event(deal_epfd, sock, EPOLLIN);
                    else
                    {
                        close(sock);
                        continue;
                    }
                }
            } // Read Event
            else if (new_apply.events & EPOLLOUT)
            { /* Writing Work */
                int err_code = handle_write(new_client);
                if (HANDLE_WRITE_AGAIN == err_code) /* TCP's Write buffer is Busy */
                    mod_event(deal_epfd, sock, EPOLLOUT);
                else if (HANDLE_READ_FAILURE == err_code)
                { /* Peer Close */
                    close(sock);
                    continue;
                }
                /* if Keep-alive */
                if (1 == new_client->conn_res.conn_linger)
                    mod_event(deal_epfd, sock, EPOLLIN);
                else
                {
                    close(sock);
                    continue;
                }
            } else
            { /* EPOLLRDHUG EPOLLERR EPOLLHUG */
                close(sock);
            }
        } /* New Apply */
    } /* main while */
    return (void *) 0;
}

void handle_loop(int fd, int sock_type, const config_t *config)
{
    signal(SIGINT, shutdown_server);
    /* 除去1个用来做listener，剩下的内核都用来做worker */
    worker_num = config->core_num - listener_num;

    /* 创建一个epoll用于监听 */
    int listen_epfd = epoll_create1(0);
    if (-1 == listen_epfd)
    {
        perror("epoll create1: error");
        exit(-1);
    }

    /* 注册listen_epfd需要关注的socket和event */
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLET | EPOLLERR | EPOLLIN;
    epoll_ctl(listen_epfd, EPOLL_CTL_ADD, fd, &event);

    /* 为worker和listener的线程做准备 */
    prepare_workers(config);
    pthread_t listener_set[listener_num];
    pthread_t worker_set[worker_num];

    /* 为每一个监听epoll创建线程 */
    for (int i = 0; i < listener_num; ++i)
    {
        pthread_create(&listener_set[i], NULL, listen_thread, (void *) &listen_epfd);
        //pthread_detach(listener_set[i]);
    }

    /* 为每一个工作epoll创建线程 */
    for (int j = 0; j < worker_num; ++j)
    {
        pthread_create(&worker_set[j], NULL, workers_thread, (void *) &(epfd_group[j]));
        pthread_detach(worker_set[j]);
    }

    /* 阻塞在这里，知道listener全部都终止了，才会销毁资源，然后退出handle_loop */
    for (int k = 0; k < listener_num; ++k)
    {
        pthread_join(listener_set[k], NULL);
    }
    destroy_resouce();
}