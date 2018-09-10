/**
 * @Author: cheng
 * @Date: 上午9:43 18-9-9
 * @Description: 
 * @Version: 
 */

#include <time.h>
#include <stdio.h>
#include "http_response.h"

extern char *cache_file;
extern char *website_root_path;
/* HTTP Date */
static const char *date_week[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *date_month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
/* HTTP Content-Type */
typedef enum
{
    TEXT_HTML = 0, TEXT_PLAIN = 1/* .txt .sor .sol or nothingand so on */,
    IMAGE_GIF = 2, IMAGE_JPEG = 3/* .jfif .jpeg .jpg .jpe*/,
    IMAGE_PNG = 4, IMAGE_BMP = 5,
} CONTENT_TYPE;

static const char *content_type[] = {
        "text/html; charset=utf-8",
        "text/plain; charset=utf-8",
        "image/git", "image/jpeg",
        "image/png", "image/bmp",
};
static const char *http_ver[] = {
        "HTTP/0.9", /* HTTP_0_9 */
        "HTTP/1.0", /* HTTP_1_0 */
        "HTTP/1.1", /* HTTP_1_1 */
        "HTTP/2.0", /* HTTP_2_0 */
        "UNKNOWN",  /* HTTP_UNKNOWN */
};


/* HTTP状态码 */
static const char *const
        ok_200_status[] = {"200",
                           "OK", ""};
// defined but not used
/*static const char * const
        redir_304_status[] = {"304",
                              "Not Modify", "Your file is the Newest"};*/
static const char *const
        clierr_400_status[] = {"400",
                               "Bad Request", "WuShxin Server can not Resolve that Apply"};
static const char *const
        clierr_403_status[] = {"403",
                               "Forbidden", "Request Forbidden"};
static const char *const
        clierr_404_status[] = {"404",
                               "Not Found", "WuShxin Server can not Find the page"};
static const char *const
        clierr_405_status[] = {"405",
                               "Method Not Allow", "WuShxin Server has not implement that method yet ~_~"};
static const char *const
        sererr_500_status[] = {"500",

                               "Internal Server Error", "Apply Incomplete, WuShxin Server Meet Unknown Status"};

/**
* @Description: 判断请求的路径是否合法
* @Param:
* @return:
*/
static inline int check_evil_path(string_t uri)
{
    /* 这里只解决一种不合法的路径：即想访问上级目录 */
    return (NULL != search_content(uri, "..", 2)) ? 1 : 0;
}


__thread char local_write_buf[CONN_BUF_SIZE] = {0};

static int write_to_buf(conn_client *restrict client, // connection client message
                        const char *const *restrict status, int rsource_size)
{
#define STATUS_CODE 0
#define STATUS_TITLE 1
#define STATUS_CONTENT 2

    /* 线程内创建了一个临时写buffer */
    char *write_buf = &local_write_buf[0];
    /* 客户端请求的资源路径 */
    string_t resource = client->conn_res.requ_res_path;
    /* 客户端的写buffer */
    string_t w_buf = client->w_buf;
    int w_count = 0;

    /* 获取当前时间 */
    struct tm *utc;
    time_t now;
    time(&now);
    utc = gmtime(&now);

/* 构造HTTP响应头部 */
    /* 1.构造HTTP响应消息的状态行
     * | 协议版本 | 状态码 | 状态消息 | */
    w_count += snprintf(write_buf + w_count, (size_t) (CONN_BUF_SIZE - w_count), "%s %s %s\r\n",
                        http_ver[client->conn_res.request_http_v],
                        status[STATUS_CODE], status[STATUS_TITLE]);

    /* 2.构造HTTP响应消息的消息报头——时间 */
    w_count += snprintf(write_buf + w_count, (size_t) CONN_BUF_SIZE - w_count,
                        "Date: %s, %02d %s %d %02d:%02d:%02d GMT\r\n",
                        date_week[utc->tm_wday], utc->tm_mday,
                        date_month[utc->tm_mon], 1900 + utc->tm_year,
                        utc->tm_hour, utc->tm_min, utc->tm_sec);
    /* 3.构造HTTP响应消息的消息报头——content-type */
    w_count += snprintf(write_buf + w_count, (size_t) CONN_BUF_SIZE - w_count, "Content-Type: %s\r\n",
                        content_type[client->conn_res.content_type]);
    /* 4.构造HTTP响应消息的消息报头——content-length */
    w_count += snprintf(write_buf + w_count, (size_t) CONN_BUF_SIZE - w_count, "Content-Length: %u\r\n",
                        0 == rsource_size
                        ? (unsigned int) strlen(status[2]) : (unsigned int) rsource_size);
    /* 5.构造HTTP响应的消息报头——connection */
    w_count += snprintf(write_buf + w_count, (size_t) CONN_BUF_SIZE - w_count, "Connection: close\r\n");
    /* 6.构造HTTP响应的空行 */
    w_count += snprintf(write_buf + w_count, (size_t) CONN_BUF_SIZE - w_count, "\r\n");

    write_buf[w_count] = '\0';
    /* 将响应报文写入客户端的写缓冲中 */
    append_string(w_buf, write_buf, strlen(write_buf));
    client->w_buf_offset = w_count;
/* 构造HTTP响应头部结束 */

/* 构造HTTP响应正文 */
    /* 根据请求方法是GET还是HEAD，添加或者不添加响应正文 */
    /* rsource_size == 0表示GET方法，需要返回内容 */
    if (0 == rsource_size)
    {
        append_string(w_buf, status[STATUS_CONTENT], strlen(status[STATUS_CONTENT]));
        return 0;
    }
        /* rsource_size == 1表示HEAD方法，需要返回内容 */
    else if (-1 == rsource_size)
    {
        return 0;
    }

    /* 如果请求文件是index.html,则打开文件，映射到内存，在将文件内容写入客户端的写缓冲中 */
    if (0 != rcmp_content(resource, "index.html", 10))
    {
        /* 代开文件 */
        int fd = open(resource->str, O_RDONLY);
        if (fd < 0)
        {
            return -1; /* Write again */
        }

        /* 将文件映射到内存中 */
        char *file_map = mmap(NULL, (size_t) rsource_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (NULL == file_map)
        {
            assert(file_map != NULL);
        }
        close(fd);

        /* 构造HTTP响应消息的正文：文件内容 */
        append_string(w_buf, file_map, (uint32_t) rsource_size);
        client->w_buf_offset += rsource_size;

        /*取消文件映射 */
        munmap(file_map, (unsigned int) rsource_size);
    } else
    {
        char *file_map = cache_file;
        append_string(w_buf, file_map, (uint32_t) rsource_size);
        client->w_buf_offset += rsource_size;
    }
    return 0;
#undef STATUS_CODE
#undef STATUS_TITLE
#undef STATUS_CONTENT
}

/**
* @Description: 构造客户端发来的请求路径在server中的真实路径。
* 如客户端请求路径是/index.html，而改文件在server中的真实路径是在/home/cheng/index.html
* @Param:
* @return:
*/
static inline void deal_uri_string(string_t uri)
{
    /* 如果客户端请求路径是"/",那么转换成/home/cheng/index.html */
    if (1 == get_length(uri) && NULL != search_content(uri, "/", 1))
    {
        clear_string(uri);
        append_string(uri, website_root_path, strlen(website_root_path));
        append_string(uri, "index.html", 10);
    }
        /* 如果客户端请求的路径不是"/"，那么转换成/home/cheng/客户端请求的路径 */
    else
    {
        char tmp[1024] = {0};
        snprintf(tmp, 1024, "%s%s", website_root_path, uri->str + 1);
        clear_string(uri);
        append_string(uri, tmp, strlen(tmp));
    }
}

typedef enum
{
    IS_NORMAL_FILE = 0,
    NO_SUCH_FILE = 1,
    FORBIDDEN_ENTRY = 2,
    IS_DIRECTORY = 3,
} URI_STATUS;

/**
* @Description: 判断客户端请求的路径在服务器端是否存在
* @Param:
* @return:
*/
static URI_STATUS check_uri_str(string_t restrict uri_str, int *restrict file_size)
{
    /* 判断文件的状态 */
    /* 如果文件不存在 */
    struct stat buf = {0};
    if (stat(uri_str->str, &buf) < 0)
    {
        fprintf(stderr, "Get File %s ", uri_str->str);
        perror("Message: ");
        return NO_SUCH_FILE; /* No such file */
    }
    /* 如果文件被其他程序打开 */
    if (0 == (buf.st_mode & S_IROTH))
    {
        fprintf(stderr, "FORBIDDEN READING\n");
        return FORBIDDEN_ENTRY; /* Forbidden Reading */
    }
    /* 如果路径是一个目录 */
    if (S_ISDIR(buf.st_mode))
    {
        return IS_DIRECTORY; /* Is Directiry */
    }
    /* 正常 */
    *file_size = (int) (buf.st_size);
    return IS_NORMAL_FILE;
}

/* 判断客户端请求的资源的类型 */
static CONTENT_TYPE check_res_type(string_t uri)
{
    int length = get_length(uri);
    char *search = uri->str;
    char *type = NULL;
    for (int i = length - 1; i >= 0 && (search[i] != '/'); --i)
    {
        if ('.' == search[i])
        {
            type = search + i;
            break;
        }
    }
    if (NULL == type) return TEXT_PLAIN;
    if (0 == strncasecmp(type, ".jepg", 5) || 0 == strncasecmp(type, ".jpg", 4) ||
        0 == strncasecmp(type, ".jpe", 4) || 0 == strncasecmp(type, ".jfif", 5))
        return IMAGE_JPEG;
    if (0 == strncasecmp(type, ".png", 4)) return IMAGE_PNG;
    if (0 == strncasecmp(type, ".bmp", 4)) return IMAGE_BMP;
    if (0 == strncasecmp(type, ".gif", 4)) return IMAGE_GIF;
    if (0 == strncasecmp(type, ".html", 5)) return TEXT_HTML;
    return TEXT_PLAIN;
}

/**
* @Description: 记录客户端请求的资源的类型
* @Param:
* @return:
*/
static inline int set_res_type(conn_client *client)
{
    client->conn_res.content_type = check_res_type(client->conn_res.requ_res_path);
    return 0;
}

/**
* @Description: 构造响应页
* @Param:
* @return:
*/
MAKE_PAGE_STATUS make_response_page(conn_client *client)
{
    int err_code = 0;
    int uri_file_size = 0;

    string_t uri_str = client->conn_res.requ_res_path;

    /* 请求路径是否为空或者"/" */
    if (1 <= get_length(uri_str))
    {

        /* 判断请求路径是否非法 */
        err_code = check_evil_path(uri_str);
        if (1 == err_code)
        {
            err_code = write_to_buf(client, clierr_403_status, 0);
            if (err_code < 0)
                return MAKE_PAGE_FAIL;
            return MAKE_PAGE_SUCCESS;
        }
        /* 将客户端请求的路径转换成服务器端的真实路径 */
        deal_uri_string(uri_str);
    }
    else
    {
        goto SERVER_ERROR;
    }

/* 分别处理GET,HEAD,POST,Unknown请求方法 */
    /* 处理GET方法
     * 1.文件正常，则返回文件内容
     * 2.文件被禁用，则返回403
     * 3.文件不存在或者为目录，则返回404*/
    if (METHOD_GET == client->conn_res.request_method)
    {
        /* 判断文件是否能正常打开，如果可以返回文件大小 */
        err_code = check_uri_str(uri_str, &uri_file_size);
        if (IS_NORMAL_FILE == err_code)
        {
            set_res_type(client);
            if ((err_code = write_to_buf(client, ok_200_status, uri_file_size)) < 0)
                goto SERVER_ERROR;
        }
        else if (FORBIDDEN_ENTRY == err_code)
        {
            err_code = write_to_buf(client, clierr_403_status, 0);
            if (err_code < 0)
                return MAKE_PAGE_FAIL;
            return MAKE_PAGE_SUCCESS;
        }
        else /*if (NO_SUCH_FILE == err_code || IS_DIRECTORY == err_code)*/ {
            write_to_buf(client, clierr_404_status, 0);
        }
    }
    /* 处理HEAD方法
     * 1.文件正常，则返回响应头
     * 2.文件被禁用，则返回403
     * 3.文件不存在或者为目录，则返回404 */
    else if (METHOD_HEAD == client->conn_res.request_method)
    {
        err_code = check_uri_str(uri_str, &uri_file_size);
        if (IS_NORMAL_FILE == err_code)
        {
            /* Check what kind of Resource does peer Request */
            set_res_type(client);
            if ((err_code = write_to_buf(client, ok_200_status, -1)) < 0)
                goto SERVER_ERROR;
        }
        else if (FORBIDDEN_ENTRY == err_code)
        {
            write_to_buf(client, clierr_403_status, -1);
        }
        else /*if (NO_SUCH_FILE == err_code || IS_DIRECTORY == err_code)*/ {
            write_to_buf(client, clierr_404_status, -1);
        }
    }
    /* 处理POST方法
     * 未实现 */
    else if (METHOD_POST == client->conn_res.request_method)
    {
        //else if ( 0 == strncasecmp(client->conn_res.requ_method->str, "POST", 4)) {
        /* TODO
         * POST Method
         * */
        write_to_buf(client, clierr_405_status, 0);
    }
    /* 处理未知方法
     * 返回400*/
    else
    { /* Unknown Method */
        write_to_buf(client, clierr_400_status, 0);
    }
    return MAKE_PAGE_SUCCESS;

    SERVER_ERROR:
    err_code = write_to_buf(client, sererr_500_status, 0);
    if (err_code < 0)
        return MAKE_PAGE_FAIL;
    return MAKE_PAGE_SUCCESS;
}