/**
 * @Author: cheng
 * @Date: 下午4:23 18-9-9
 * @Description: 
 * @Version: 
 */

#include "handle_write.h"

/**
* @Description: 将客户端的写buffer中的数据真正的写到socket中，发送给客户端
* @Param:
* @return:
*/
HANDLE_STATUS handle_write(conn_client *client)
{
    char *w_buf = client->w_buf->str;
    int w_offset = client->w_buf_offset;
    int nbyte = w_offset;
    int count = 0;
    int fd = client->file_dsp;

    while (nbyte > 0)
    {
        w_buf += count;
        count = write(fd, w_buf, nbyte);
        /* 如果发送给客户端失败，处理如下 */
        if (count < 0)
        {
            /* 如果失败原因是EAGAIN或者EWOULDBLOCK，则恢复client的写buffer，然后准备重发 */
            if (EAGAIN == errno || EWOULDBLOCK == errno)
            {
                memcpy(client->w_buf->str, w_buf, strlen(w_buf));
                client->w_buf_offset = nbyte;
                return HANDLE_WRITE_AGAIN;
            }
            /* 如果是其他失败原因*/
            else /* if (EPIPE == errno) */
                return HANDLE_WRITE_FAILURE;
        }
        /* 如果发送了0个字节 */
        else if (0 == count)
            return HANDLE_WRITE_FAILURE;
        /* 如果发送成功 */
        nbyte -= count;
    }
    return HANDLE_WRITE_SUCCESS;
}