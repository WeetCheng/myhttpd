/**
 * @Author: cheng
 * @Date: 下午4:40 18-9-7
 * @Description: 
 * @Version: 
 */

#ifndef MYHTTPD_MMANAGE_H
#define MYHTTPD_MMANAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#if defined(__GNUC__) && (__GNUC__ >= 3) || defined(__clang__) && (__clang_major__ >= 3)
#define LIKELY(x) __builtin_expect(!!(x), 1) //x很可能为真
#define UNLIKELY(x) __builtin_expect(!!(x), 0) //x很可能为假
#else
#define UNLIKELY(x) (x)
#endif
/* 内存管理的错误代码 */
typedef enum {
    ALLOC_SUCCEED     = 0x00, /* Successful */
    MALLOC_ERR_SIZE   = 0x01, /* Invalid Size For Allocate */
    CALLOC_ERR_SIZE   = 0x02,
    MALLOC_ERR_NO_MEM = 0x08, /* No Enough memory For Allocation */
    CALLOC_ERR_NO_MEM = 0x10,
    FREE_ERR_EMPTY    = 0x04, /* Free Pointer is NULL */
    FREE_SUCCEED      = ALLOC_SUCCEED,
} mm_error;

void* my_malloc(size_t sizes);
void* my_calloc(size_t sizes);
mm_error my_free(void * pointer);

#define Malloc my_malloc
#define Calloc my_calloc
#define Free my_free

#endif //MYHTTPD_MMANAGE_H
