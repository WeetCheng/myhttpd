/**
 * @Author: cheng
 * @Date: 下午4:59 18-9-7
 * @Description: 
 * @Version: 
 */

#ifndef MYHTTPD_MY_STRING_H
#define MYHTTPD_MY_STRING_H

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>

/* 自己实现一个string结构体，主要记录string的地址，string的长度，string的最大容量 */
struct string_s
{
    char *str;      // string的地址
    uint32_t length;// string的长度
    uint32_t cap;   // string的容量
};

/* 定义一个string_s指针类型 */
typedef struct string_s String_s;
typedef String_s * string_t;
typedef unsigned char boolean;
#define MAKE_STRING_S(c_str) make_string(c_str, strlen(c_str))


/* 创建string */
String_s * make_string(const char * content, uint32_t length);
/* 复制string */
String_s * copy_string(const String_s * src);
/* 取得string的buffer */
const char * get_inner_buf(const String_s * itself);
/* 取得string的长度 */
uint32_t get_length(const String_s * itself);
/* 取得string的容量 */
uint32_t get_capacity(const String_s * itself);
/* 判断string是否为空 */
boolean is_empty(const String_s * itself);
/* string查找子串 */
char * search_content(String_s * itself, const char * search, uint32_t length);
/* 释放string */
void free_string(String_s * itself);
/* 清空string */
void clear_string(String_s * itself);
/* 添加string后面 */
uint32_t append_string(String_s * itself, const char * content, uint32_t length);
/* 比较string与buffer的内容 */
boolean compare_string_char(String_s * itself, const char * content, uint32_t length);
/* 比较string与buffer的内容 */
int32_t rcmp_content(const String_s * cmp1, const char * cmp2, uint32_t n);
/* 比较两个string */
boolean compare_string_string(String_s * first, String_s * second);
/* 取string的子串 */
String_s * make_substring(const String_s * itself, uint32_t start, uint32_t length);
/* 复制string的buffer */
char * make_inner_buf(const String_s * itself);

#endif //MYHTTPD_MY_STRING_H
