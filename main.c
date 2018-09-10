/**
 * @Author: cheng
 * @Date: 上午9:50 18-9-7
 * @Description: 实现一个简单的httpd服务器
 * @Version: 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "config.h"
#include "handle.h"


int main(int argc, char *argv[])
{
    /* 解析配置文件 */
    config_t config;
    memset(&config, 0, sizeof(config));
    if (-1 == init_config(&config))
        exit(-1);

    /* 创建listener套接字，绑定, 并返回套接字的网络协议族，记录在socket_type中 */
    int socket_type = 0;
    int listenfd = create_listenfd(config.use_addr, config.listen_port, &socket_type);
    if (listenfd < 0)
        return -1;

    /* 监听 */
    int error_code = listen(listenfd, SOMAXCONN);
    if (-1 == error_code)
    {
        perror("Usage listen Queue: ");
        return -1;
    }

    /* */
    signal(SIGPIPE, SIG_IGN);

    handle_loop(listenfd, socket_type, &config);


    return 0;
}
