/**
 * @Author: cheng
 * @Date: 下午10:07 18-9-7
 * @Description: 
 * @Version: 
 */

#ifndef MYHTTPD_HANDLE_READ_H
#define MYHTTPD_HANDLE_READ_H

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include "handle_core.h"
#include "http_response.h"

HANDLE_STATUS handle_read(conn_client *client);

typedef enum
{
    /* read_n error */
    READ_SUCCESS = -(1 << 0), /* Read From Socket success, continue to parse */
    READ_FAIL = -(1 << 1), /* Read From Socket Fail, close connection */
    // Discard
    READ_OVERFLOW = -(1 << 2), /* Read Buffer is Overflow, Reject the Connection,
                                Log it and Report to the Server Developer */
    /* get_line error */
    READ_BUF_FAIL = -(1 << 3), /* Read From conn_client buf fail */
    READ_BUF_OVERFLOW = -(1 << 4), /* Read from conn_client buf is more than the deal buf */
} READ_STATUS;

typedef enum
{
    PARSE_SUCCESS = 1 << 1, /* Parse the Reading Success, set the event to Write Event */
    PARSE_BAD_SYNTAX = 1 << 2, /* Parse the Reading Fail, for the Wrong Syntax */
    PARSE_BAD_REQUT = 1 << 3, /* Parse the Reading Success, but Not Implement OR No Such Resources*/
} PARSE_STATUS;

typedef enum {
    DEAL_LINE_REQU_SUCCESS = 0,
    DEAL_LINE_REQU_FAIL    = -1,
    DEAL_HEAD_SUCCESS = -2,
    DEAL_HEAD_FAIL    = -3,
    MESSAGE_INCOMPLETE = -4,
}DEAL_LINE_STATUS;

/******************************************************************************************/
/* Deal with the read buf data which has been read from socket */
#define METHOD_SIZE 128
#define PATH_SIZE METHOD_SIZE*2
#define VERSION_SIZE METHOD_SIZE
typedef struct requ_line
{
    char path[PATH_SIZE];
    char method[METHOD_SIZE];
    char version[VERSION_SIZE];
} requ_line;

#endif //MYHTTPD_HANDLE_READ_H
