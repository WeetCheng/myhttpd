/**
 * @Author: cheng
 * @Date: 上午9:56 18-9-7
 * @Description: 
 * @Version: 
 */

#ifndef MYHTTPD_CONFIG_H
#define MYHTTPD_CONFIG_H
#define IPV6_LENGTH_CHAR (8*4 + 7)      // 8个字段，每个字段长4个字符，再加上7个:
struct config_from_file
{
    int core_num;   /* CPU Core Numbers */
#define ADDR_SIZE IPV6_LENGTH_CHAR
    char use_addr[ADDR_SIZE];
#define PORT_SIZE 10
    char listen_port[PORT_SIZE];
#define PATH_LENGTH 256         // 定义服务器的根路径长度不超过256字节
    char root_path[PATH_LENGTH]; /* page root path */
};

typedef struct config_from_file config_t;   // 配置内容的结构体为config_t

/**
* @Description: 读取并解析配置文件，设置相关参数
* @Param:
* @return:
*/
int init_config(config_t *config);

#endif //MYHTTPD_CONFIG_H
