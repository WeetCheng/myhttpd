/**
 * @Author: cheng
 * @Date: 下午12:14 18-9-7
 * @Description: 
 * @Version: 
 */

#ifndef MYHTTPD_HANDLE_H
#define MYHTTPD_HANDLE_H

#include <signal.h>
#include "config.h"

enum err_create_listener{
    ERR_PARA_EMPTY = -1,
    ERR_GETADDRINFO = -2,
    ERR_BINDIND = -3,
    ERR_LISTEN = -4
};
/**
* @Description: 设置允许与其他程序共用socket；TCP连接不适用Nagle算法
* @Param:
* @return:
*/
void optimizes(int file_dsption);
/**
* @Description: 根据给定的IP：端口，创建并绑定socket，用于监听。同时，传回socket类型给sock_type
* @Param:
* @return:
*/
int create_listenfd(const char *restrict host_addr, const char *restrict port, int *restrict sock_type);

/**
* @Description: 设置指定的文件描述符为非阻塞
* @Param:
* @return:
*/
int set_nonblock(int fd);


/**
* @Description: http协议的主循环，accept连接请求，接收数据，及发送数据
* @Param:
* @return:
*/
void handle_loop(int fd, int sock_type, const config_t *config);


#endif //MYHTTPD_HANDLE_H
