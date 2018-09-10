/**
 * @Author: cheng
 * @Date: 下午4:40 18-9-7
 * @Description: 封装实现了自己的malloc、calloc、free
 * @Version: 
 */

#include "mmanage.h"


__thread int error;
void* my_malloc(size_t sizes) {
    void * alloc = NULL;
    if(UNLIKELY(sizes < 0)) {
        error = MALLOC_ERR_SIZE;
        return NULL;
    }
    alloc = my_calloc(sizes);
    if(UNLIKELY(NULL == alloc)){
        error = MALLOC_ERR_NO_MEM;
        return alloc;
    }
    return alloc;
}

void* my_calloc(size_t sizes) {
    void * alloc = NULL;
    if(UNLIKELY(sizes < 0)) {
        error = CALLOC_ERR_SIZE;
        return alloc;
    }
    alloc = calloc(1, sizes);
    if(UNLIKELY(NULL == alloc))
        error = CALLOC_ERR_NO_MEM;
    return alloc;
}

mm_error my_free(void * pointer) {
    if(UNLIKELY(NULL == pointer))
        return FREE_ERR_EMPTY;
    free(pointer);
    return FREE_SUCCEED;
}