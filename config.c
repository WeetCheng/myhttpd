/**
 * @Author: cheng
 * @Date: 上午9:56 18-9-7
 * @Description: 
 * @Version: 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

/* 设置配置文件搜索路径：
 * 1. ./config.conf
 * 2. ./config/config.conf
 * */
static const char *config_search_path[] = {"../config.conf", "./config.conf", NULL};

int init_config(config_t *config)
{
    const char ** roll = config_search_path;
    FILE *file = NULL;

    /* 按照搜索路径优先级搜索配置文件 */
    for (int i = 0; roll[i] != NULL; ++i)
    {
        file = fopen(roll[i], "r");
        if(file != NULL)
            break;
    }
    /* 如果搜索不到配置文件，报错退出 */
    if(file == NULL)
    {
        exit(-1);
    }
    /* 如果找到配置文件，开始解析 */

    char buf[PATH_LENGTH] = {'\0'};
    char *ret = fgets(buf, PATH_LENGTH, file);  // 读取一行
    while(ret != NULL)
    {
        /* 忽略#后面的内容 */
        char * check = strchr(buf, '#');
        if (check != NULL)
            *check = '\0';
        /* 取:前面的内容 */
        char * pos = strchr(buf, ':');
        if (pos != NULL)
        {
            *pos++ = '\0';

            /* 1.如果是thread，读到core_num */
            if (0 == strncasecmp(buf, "thread", 6))
            {
                sscanf(pos, "%d", &config->core_num);
            }

            /* 2.如果是root，读到root_path */
            else if (0 == strncasecmp(buf, "root", 4))
            {
                sscanf(pos, "%s", config->root_path);
                /* 如果配置文件中的路径不是以斜杠结束，则自动添加 */
                if ((config->root_path)[strlen(config->root_path) - 1] != '/')
                {
                    strncat(config->root_path, "/", 1);
                }
            }

            /* 3.如果是port，读到listen_port */
            else if (0 == strncasecmp(buf, "port", 4))
            {
                sscanf(pos, "%s", config->listen_port);
            }

            /* 4.如果是addr，读到use_addr */
            else if (0 == strncasecmp(buf, "addr", 4))
            {
                sscanf(pos, "%s", config->use_addr);
            }

        }

        /* 读取下一行 */
        ret = fgets(buf, PATH_LENGTH, file);
    }

    fclose(file);
    return 0;
}