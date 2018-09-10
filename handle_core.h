/**
 * @Author: cheng
 * @Date: 下午4:56 18-9-7
 * @Description: 
 * @Version: 
 */

#ifndef MYHTTPD_HANDLE_CORE_H
#define MYHTTPD_HANDLE_CORE_H

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "my_string.h"


enum REQUST_METHOD{
    METHOD_GET  = 0,
    METHOD_HEAD = 1,
    METHOD_POST = 2,
    METHOD_UNKNOWN = 3,
};

enum HTTP_VERSION {
    HTTP_0_9 = 0,
    HTTP_1_0 = 1,
    HTTP_1_1 = 2,
    HTTP_2_0 = 3,
    HTTP_UNKNOWN = HTTP_0_9,
};

struct connection {
    int  file_dsp;
#define CONN_BUF_SIZE 512
    int r_buf_offset;
    int w_buf_offset;
    string_t r_buf;
    string_t w_buf;
    struct {
        /* Is it Keep-alive in Application Layer */
        boolean conn_linger  : 1;
        boolean set_ep_out   : 1;
        boolean is_read_done : 1; /* Read from Peer Done? */
        boolean request_http_v : 2; /* HTTP/1.1 1.0 0.9 2.0 */
        boolean request_method : 2; /* GET HEAD POST */
        int content_type : 4;  /* 2 ^ 4 -> 16 Types */
        int content_length; /* For POST */
        string_t requ_res_path; /* / */
    }conn_res;
};
typedef struct connection conn_client;

typedef enum {
    HANDLE_READ_SUCCESS = -(1 << 1),
    HANDLE_READ_FAILURE = -(1 << 2),
    HANDLE_WRITE_SUCCESS = -(1 << 3),
    HANDLE_WRITE_AGAIN   = -(1 << 5),
    HANDLE_WRITE_FAILURE = -(1 << 4),
}HANDLE_STATUS;
#endif //MYHTTPD_HANDLE_CORE_H
