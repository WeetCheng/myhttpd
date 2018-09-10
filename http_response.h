/**
 * @Author: cheng
 * @Date: 上午9:43 18-9-9
 * @Description: 
 * @Version: 
 */

#ifndef MYHTTPD_HTTP_RESPONSE_H
#define MYHTTPD_HTTP_RESPONSE_H

#include "handle_core.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
typedef enum {
    MAKE_PAGE_SUCCESS = 0,
    MAKE_PAGE_FAIL    = 1,
}MAKE_PAGE_STATUS;

MAKE_PAGE_STATUS make_response_page(conn_client *client);
#endif //MYHTTPD_HTTP_RESPONSE_H
