/**
 * @Author: cheng
 * @Date: 下午10:07 18-9-7
 * @Description: 
 * @Version: 
 */

#include "handle_read.h"

/* 线程自己的buffer */
__thread char read_buf2[CONN_BUF_SIZE] = {0};

/**
* @Description: 读取client中的buffer数据
* @Param:
* @return:
*/
static int read_n(conn_client *client)
{

    int read_offset2 = 0;
    int fd = client->file_dsp;
    char *buf = &read_buf2[0];
    int buf_index = read_offset2;
    int read_number = 0;
    int less_capacity = 0;              // read_buf2的空闲空间
    while (1)
    {
        less_capacity = CONN_BUF_SIZE - buf_index;

        /* 如果read_buf2满了,那么将其中的数据保存到client中的r_buffer中,并更新两个缓冲区的索引和空闲空间大小 */
        if (less_capacity <= 1)
        {
            /* 将read_buf2中的数据保存到client的r_buffer中 */
            buf[buf_index] = '\0';
            append_string(client->r_buf, buf, strlen(buf));

            /* 更新两个缓冲区的索引 */
            client->r_buf_offset += read_offset2;
            read_offset2 = 0;
            buf_index = 0;
            less_capacity = CONN_BUF_SIZE - buf_index;
        }

        /* 如果read_buf2没满,那么从客户端socket中读取数据 */
        read_number = (int) read(fd, buf + buf_index, less_capacity);

        /* 读到0个数据,说明客户端发来EOF,读取结束 */
        if (0 == read_number)
        {
            return READ_FAIL;
        }
            /* 读取失败,如果是因为数据重发或者阻塞,那么最后实际上还是读取成功的,则更新buffer */
        else if (-1 == read_number)
        {
            if (EAGAIN == errno || EWOULDBLOCK == errno)
            {
                buf[buf_index] = '\0';
                append_string(client->r_buf, buf, strlen(buf));
                client->r_buf_offset += read_offset2;
                return READ_SUCCESS;
            }
            return READ_FAIL;
        }
            /* 读取read_number个字符 */
        else
        {
            buf_index += read_number;
            read_offset2 = buf_index;
        }
    }
}


/**
* @Description: 从client的buffer中读取一行数据到line_buf中
* @Param:
* @return:
*/
static int get_line(conn_client *restrict client, char *restrict line_buf, int max_line)
{
    int nbytes = 0;
    char *r_buf_find = search_content(client->r_buf, "\n", 1);

    if (NULL == r_buf_find)
    {
        fprintf(stderr, "get_line has read a incomplete Message\n");
        return MESSAGE_INCOMPLETE;
    } else
    {
        nbytes = r_buf_find - (client->r_buf->str + client->r_buf_offset) + 1;
        if (max_line - 1 < nbytes)
            return READ_BUF_OVERFLOW;
        memcpy(line_buf, client->r_buf->str + client->r_buf_offset, nbytes);
        client->r_buf_offset = r_buf_find - client->r_buf->str + 1;
        *r_buf_find = '\r'; /* Let the \n to be \r */
    }
    line_buf[nbytes] = '\0';
    return nbytes;
}




/**
* @Description: 解析请求行
* @Param:
* @return:
*/
static DEAL_LINE_STATUS deal_requ(conn_client *client, requ_line *status)
{
#define READ_HEAD_LINE 3+256+8+1
    char requ_line[READ_HEAD_LINE] = {'\0'};
    int err_code = get_line(client, requ_line, READ_HEAD_LINE);
    if (err_code < 0)
        return DEAL_LINE_REQU_FAIL;

    /* 读取的行最后为\r\n,否则说明读取失败 */
    if ('\n' != requ_line[strlen(requ_line) - 1] && '\r' != requ_line[strlen(requ_line) - 2])
        return MESSAGE_INCOMPLETE;

    /* 从请求行缓冲区中解析出请求方法、请求路径、http版本 */
    err_code = sscanf(requ_line, "%s %s %s", status->method, status->path, status->version);
    if (err_code != 3)
        return DEAL_LINE_REQU_FAIL;

    /* 解析请求的相关信息，保存到连接数据结构client中 */
    /* 记录请求方法是GET、HEAD、POST中哪一种 */
    if (0 == strncasecmp(status->method, "GET", 3))
        client->conn_res.request_method = METHOD_GET;
    else if (0 == strncasecmp(status->method, "HEAD", 4))
        client->conn_res.request_method = METHOD_HEAD;
    else if (0 == strncasecmp(status->method, "POST", 4))
        client->conn_res.request_method = METHOD_POST;
    else
    {
        client->conn_res.request_method = METHOD_UNKNOWN;
        return DEAL_LINE_REQU_FAIL;
    }

    /* 记录请求的http版本 */
    if (0 == strncasecmp(status->version, "HTTP/1.1", 8))
        client->conn_res.request_http_v = HTTP_1_1;
    else if (0 == strncasecmp(status->version, "HTTP/1.0", 8))
        client->conn_res.request_http_v = HTTP_1_0;
    else if (0 == strncasecmp(status->version, "HTTP/0.9", 8))
        client->conn_res.request_http_v = HTTP_0_9;
    else if (0 == strncasecmp(status->version, "HTTP/2.0", 8))
        client->conn_res.request_http_v = HTTP_2_0;
    else
    {
        client->conn_res.request_http_v = HTTP_UNKNOWN;
        return DEAL_LINE_REQU_FAIL;
    }

    /* 记录请求的路径 */
    append_string((client->conn_res).requ_res_path, status->path, (uint32_t)strlen(status->path));

    return DEAL_LINE_REQU_SUCCESS;
#undef READ_HEAD_LINE
}

#define READ_ATTRIBUTE_LINE 256
#define TRUE 1
#define FALSE 0

static DEAL_LINE_STATUS deal_head(conn_client *client)
{
    int nbytes = 0;
    boolean is_post = METHOD_POST == client->conn_res.request_method ? TRUE : FALSE;
    char head_line[READ_ATTRIBUTE_LINE] = {'\0'};

    /* 循环读取行 */
    while ((nbytes = get_line(client, head_line, READ_ATTRIBUTE_LINE)) > 0)
    {
        if (MESSAGE_INCOMPLETE == nbytes)
            return MESSAGE_INCOMPLETE;

        /* 如果读到的空行，说明头部已经读完，退出循环 */
        if (0 == strncmp(head_line, "\r\n", 2))
        {
            break;
        }

        /* 解析POST的头部 */
        if (TRUE == is_post)
        {
            /* 解析Content-Length行，记录到client结构体中 */
            if (0 == strncmp(head_line, "Content-Length", 14))
            {
                char *end = strchr(head_line, '\r');
                *end = '\0';
                sscanf(head_line + 16, "%d", &(client->conn_res.content_length));
                *end = '\r';
            }
        }

        /* 解析Connection行，记录到client结构体中 */
        if (0 == strncmp(head_line, "Connection", 10))
        {
            /* Connection: keep-alive\r\n */
            if (0 == strncmp(head_line + 12, "close", 5))
                client->conn_res.conn_linger = 1;
            else
                client->conn_res.conn_linger = 0;
        }

    }
    if (TRUE == is_post)
    {
        /* TODO POST METHOD
         * make sure the Message has been Receive completely and then Parse them
         * */
    }
    if (nbytes < 0)
    {
        fprintf(stderr, "Error Reading in deal_head\n");
        return DEAL_HEAD_FAIL;
    }
    return DEAL_HEAD_SUCCESS;
}




/**
* @Description: 解析从客户端读到的数据,实际上就是解析客户端的请求
* @Param:
* @return:
*/
PARSE_STATUS parse_reading(conn_client *client)
{
    int err_code = 0;
    requ_line line_status = {0};
    client->r_buf_offset = 0;

    /* 解析请求行 */
    err_code = deal_requ(client, &line_status);
    if (MESSAGE_INCOMPLETE == err_code)  /* Incompletely reading */
        return MESSAGE_INCOMPLETE;
    if (DEAL_LINE_REQU_FAIL == err_code) /* Bad Request */
        return PARSE_BAD_REQUT;

    /* 解析请求头部直到/r/n行 */
    err_code = deal_head(client);  /* The second line to the Empty line */
    if (DEAL_HEAD_FAIL == err_code)
        return PARSE_BAD_SYNTAX;

    /* 构造HTTP响应页 */
    err_code = make_response_page(client);
    if (MAKE_PAGE_FAIL == err_code)
        return PARSE_BAD_REQUT;

    return PARSE_SUCCESS;
}


HANDLE_STATUS handle_read(conn_client *client)
{
    int err_code = 0;

    /* Reading From Socket */
    err_code = read_n(client);
    if (err_code != READ_SUCCESS)
    { /* If read Fail then End this connect */
        return HANDLE_READ_FAILURE;
    }

    /* Parsing the Reading Data */
    err_code = parse_reading(client);
    if (err_code == MESSAGE_INCOMPLETE)
        return MESSAGE_INCOMPLETE;
    if (err_code != PARSE_SUCCESS)
    { /* If Parse Fail then End this connect */
        return HANDLE_READ_FAILURE;
    }
    return HANDLE_READ_SUCCESS;
}