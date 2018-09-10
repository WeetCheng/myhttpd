/**
 * @Author: cheng
 * @Date: 下午4:23 18-9-9
 * @Description: 
 * @Version: 
 */

#ifndef MYHTTPD_HANDLE_WRITE_H
#define MYHTTPD_HANDLE_WRITE_H
#include "handle_core.h"
/*
 * For EPOLLOUT controller
 * */
HANDLE_STATUS handle_write(conn_client * client);

#endif //MYHTTPD_HANDLE_WRITE_H
