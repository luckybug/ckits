#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "metaBuffer.h"
#include "myAlloc.h"


#ifdef THREAD_SAFE
static pthread_mutex_t allocMutex=PTHREAD_MUTEX_INITIALIZER;

// printf("metaBuffer::%s\n", __FUNCTION__);
#define CHECK_SINGLETON \
do{ \
    if (!initializeFlag) { \
        pthread_mutex_lock(&allocMutex); \
        if (!initializeFlag) { \
            cJSON_Hooks _hook = {&_malloc, &_free}; \
            init_pool_allocator(&_metaInfoAlloc); \
            cJSON_InitHooks(&_hook); \
            initializeFlag = 1; \
        } \
        pthread_mutex_unlock(&allocMutex); \
    } \
}while(0)
#else // non thread-safe version

#define CHECK_SINGLETON \
do{ \
    if (!initializeFlag) { \
        cJSON_Hooks _hook = {&_malloc, &_free}; \
        init_pool_allocator(&_metaInfoAlloc); \
        cJSON_InitHooks(&_hook); \
        initializeFlag = 1; \
    } \
}while(0)
#endif // end THREAD_SAFE


#if !defined(METABUFFER_POOLSIZE)
#define METABUFFER_POOLSIZE     4096
#endif

static uint8_t metaPool[METABUFFER_POOLSIZE];

static pool_alloc_t _metaInfoAlloc = \
  { .chunkMem = metaPool, .chunkSize = sizeof(metaPool) };

static int initializeFlag = 0;

static void * _malloc(size_t size)
{
#ifdef THREAD_SAFE
    pthread_mutex_lock(&allocMutex);
    void *_mem = pool_malloc(&_metaInfoAlloc, size);
    pthread_mutex_unlock(&allocMutex);
    return _mem;
#else
    return pool_malloc(&_metaInfoAlloc, size);
#endif
}

static void _free(void *ptr)
{
#ifdef THREAD_SAFE
    pthread_mutex_lock(&allocMutex);
    pool_free(&_metaInfoAlloc, ptr);
    pthread_mutex_unlock(&allocMutex);
#else
    pool_free(&_metaInfoAlloc, ptr);
#endif
}

#if 0
typedef void *(*allocFunc)(size_t size);
typedef void (*freeFunc)(void *ptr);

static allocFunc _malloc = malloc;
static freeFunc _free = free;
#endif

int metabInit(metaBuffer *thisBuf)
{
    CHECK_SINGLETON;

    if (!thisBuf || !thisBuf->buffer || !thisBuf->size) {
        return -1;
    }

    if (!thisBuf->metaInfo) {
        thisBuf->metaInfo = cJSON_CreateObject();
#ifdef THREAD_SAFE
        if (thisBuf->metaInfo) {
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&thisBuf->refMutex, &attr);
            thisBuf->refCount = 1;
        }
#endif
    }
    return (thisBuf->metaInfo)? 0 : -1;
}

int metabFree(metaBuffer *thisBuf)
{
    CHECK_SINGLETON;

    if (!thisBuf) {
        return -1;
    }

    if (thisBuf->metaInfo) {
#ifdef THREAD_SAFE
        pthread_mutex_lock(&thisBuf->refMutex);
        if (thisBuf->refCount != 0) {
            // print warning
        }
#endif
        cJSON_Delete(thisBuf->metaInfo);
        thisBuf->metaInfo = NULL;
#ifdef THREAD_SAFE
        pthread_mutex_unlock(&thisBuf->refMutex);
        pthread_mutex_destroy(&thisBuf->refMutex);
#endif
    }

    if (!thisBuf->buffer) {
        if (thisBuf->unref == NULL) {
            free(thisBuf->buffer);
        } else {
            thisBuf->unref(thisBuf->buffer);
        }
        thisBuf->buffer = NULL;
        thisBuf->size = 0;
    }

    return 0;
}

#ifdef THREAD_SAFE
int metabIncRef(metaBuffer *thisBuf)
{
    CHECK_SINGLETON;

    if (!thisBuf || !thisBuf->metaInfo) {
        return -1;
    }

    int ret = -1;
    pthread_mutex_lock(&thisBuf->refMutex);
    do
    {
        if (thisBuf->refCount == 0) {
            // print warning
        }

        ++thisBuf->refCount;
        ret = 0;
    }while(0);
    pthread_mutex_unlock(&thisBuf->refMutex);

    return ret;
}

int metabDecRef(metaBuffer *thisBuf)
{
    CHECK_SINGLETON;

    if (!thisBuf || !thisBuf->metaInfo
            || thisBuf->refCount == 0) {
        return -1;
    }

    int ret = 0;
    pthread_mutex_lock(&thisBuf->refMutex);
    if (--thisBuf->refCount == 0) {
        pthread_mutex_unlock(&thisBuf->refMutex);
        return metabFree(thisBuf);
    } else {
        pthread_mutex_unlock(&thisBuf->refMutex);
        return 0;
    }
}
#endif

int metabSetToken(metaBuffer *thisBuf, const char *token, const char* value)
{
    CHECK_SINGLETON;

    if (!thisBuf || !thisBuf->metaInfo || \
            !token || !value) {
        return -1;
    }

    int ret = -1;
#ifdef THREAD_SAFE
    pthread_mutex_lock(&thisBuf->refMutex);
#endif
    do
    {
        cJSON_AddStringToObject(thisBuf->metaInfo, token, value);
        ret = 0;
    }while(0);
#ifdef THREAD_SAFE
    pthread_mutex_unlock(&thisBuf->refMutex);
#endif
    return ret;
}

int metabGetToken(metaBuffer *thisBuf, const char *token, const char** pValue)
{
    CHECK_SINGLETON;

    if (!thisBuf || !thisBuf->metaInfo || \
            !token || !pValue) {
        return -1;
    }

    int ret = -1;
#ifdef THREAD_SAFE
    pthread_mutex_lock(&thisBuf->refMutex);
#endif
    do
    {
        // detach, read and put back. do not support delete individual item
        cJSON *_item;
        _item = cJSON_DetachItemFromObject(thisBuf->metaInfo, token);
        if (!_item) {
            *pValue = NULL;
            break;
        }

        *pValue = _item->valuestring;
        cJSON_AddItemToObject(thisBuf->metaInfo, token, _item);

        ret = 0;
    } while (0);
#ifdef THREAD_SAFE
    pthread_mutex_unlock(&thisBuf->refMutex);
#endif
    return ret;
}

int metabSetInt32(metaBuffer *thisBuf, const char *token, int32_t value)
{
    CHECK_SINGLETON;

    if (!thisBuf || !thisBuf->metaInfo || !token) {
        return -1;
    }

    int ret = -1;
#ifdef THREAD_SAFE
    pthread_mutex_lock(&thisBuf->refMutex);
#endif
    do
    {
        cJSON_AddNumberToObject(thisBuf->metaInfo, token, (double)value);
        ret = 0;
    } while (0);
#ifdef THREAD_SAFE
    pthread_mutex_unlock(&thisBuf->refMutex);
#endif
    return ret;
}

int metabGetInt32(metaBuffer *thisBuf, const char *token, int32_t* pValue)
{
    CHECK_SINGLETON;

    if (!thisBuf || !thisBuf->metaInfo || \
            !token || !pValue) {
        return -1;
    }

    int ret = -1;
#ifdef THREAD_SAFE
    pthread_mutex_lock(&thisBuf->refMutex);
#endif
    do
    {
        // detach, read and put back. do not support delete individual item
        cJSON *_item;
        _item = cJSON_DetachItemFromObject(thisBuf->metaInfo, token);
        if (!_item) {
            break;
        }

        *pValue = _item->valueint;
        cJSON_AddItemToObject(thisBuf->metaInfo, token, _item);

        ret = 0;
    } while (0);
#ifdef THREAD_SAFE
    pthread_mutex_unlock(&thisBuf->refMutex);
#endif
    return ret;
}

// FIXME: discard what 'token' is by now
int metabSetInt64(metaBuffer *thisBuf, const char *token, int64_t value)
{
    CHECK_SINGLETON;

    if (!thisBuf || !thisBuf->metaInfo/* || !token*/) {
        return -1;
    }

    int ret = -1;
#ifdef THREAD_SAFE
    pthread_mutex_lock(&thisBuf->refMutex);
#endif
    do
    {
        thisBuf->ts = value;
        ret = 0;
    } while (0);
#ifdef THREAD_SAFE
    pthread_mutex_unlock(&thisBuf->refMutex);
#endif
    return ret;
}

// FIXME: discard what 'token' is by now
int metabGetInt64(metaBuffer *thisBuf, const char *token, int64_t* pValue)
{
    CHECK_SINGLETON;

    if (!thisBuf || !thisBuf->metaInfo || \
            /*!token || */!pValue) {
        return -1;
    }

    int ret = -1;
#ifdef THREAD_SAFE
    pthread_mutex_lock(&thisBuf->refMutex);
#endif
    do
    {
        *pValue = thisBuf->ts;
        ret = 0;
    } while (0);
#ifdef THREAD_SAFE
    pthread_mutex_unlock(&thisBuf->refMutex);
#endif
    return ret;
}

int metabSetBool(metaBuffer *thisBuf, const char *token, bool value)
{
    CHECK_SINGLETON;

    if (!thisBuf || !thisBuf->metaInfo || !token) {
        return -1;
    }

    int ret = -1;
#ifdef THREAD_SAFE
    pthread_mutex_lock(&thisBuf->refMutex);
#endif
    do
    {
        cJSON_AddBoolToObject(thisBuf->metaInfo, token, value);
        ret = 0;
    } while (0);
#ifdef THREAD_SAFE
    pthread_mutex_unlock(&thisBuf->refMutex);
#endif
    return ret;
}

int metabGetBool(metaBuffer *thisBuf, const char *token, bool* pValue)
{
    CHECK_SINGLETON;

    if (!thisBuf || !thisBuf->metaInfo || \
            !token || !pValue) {
        return -1;
    }

    int ret = -1;
#ifdef THREAD_SAFE
    pthread_mutex_lock(&thisBuf->refMutex);
#endif
    do
    {
        // detach, read and put back. do not support delete individual item
        cJSON *_item;
        _item = cJSON_DetachItemFromObject(thisBuf->metaInfo, token);
        if (!_item) {
            break;
        }

        *pValue = (_item->type == cJSON_True);
        cJSON_AddItemToObject(thisBuf->metaInfo, token, _item);

        ret = 0;
    } while (0);
#ifdef THREAD_SAFE
    pthread_mutex_unlock(&thisBuf->refMutex);
#endif
    return ret;
}

int metabSetFloat(metaBuffer *thisBuf, const char *token, double value)
{
    CHECK_SINGLETON;

    if (!thisBuf || !thisBuf->metaInfo || !token) {
        return -1;
    }

    int ret = -1;
#ifdef THREAD_SAFE
    pthread_mutex_lock(&thisBuf->refMutex);
#endif
    do
    {
        cJSON_AddNumberToObject(thisBuf->metaInfo, token, value);
        ret = 0;
    } while (0);
#ifdef THREAD_SAFE
    pthread_mutex_unlock(&thisBuf->refMutex);
#endif
    return ret;
}

int metabGetFloat(metaBuffer *thisBuf, const char *token, double* pValue)
{
    CHECK_SINGLETON;

    if (!thisBuf || !thisBuf->metaInfo || \
            !token || !pValue) {
        return -1;
    }

    int ret = -1;
#ifdef THREAD_SAFE
    pthread_mutex_lock(&thisBuf->refMutex);
#endif
    do
    {
        // detach, read and put back. do not support delete individual item
        cJSON *_item;
        _item = cJSON_DetachItemFromObject(thisBuf->metaInfo, token);
        if (!_item) {
            break;
        }

        *pValue = _item->valuedouble;
        cJSON_AddItemToObject(thisBuf->metaInfo, token, _item);

        ret = 0;
    } while (0);
#ifdef THREAD_SAFE
    pthread_mutex_unlock(&thisBuf->refMutex);
#endif
    return ret;
}

void metabDump(metaBuffer *thisBuf)
{
    int ret = -1;
    do
    {
#ifdef THREAD_SAFE
        pthread_mutex_lock(&allocMutex);
        if (initializeFlag) {
            pool_alloc_dump(&_metaInfoAlloc);
        }
        pthread_mutex_unlock(&allocMutex);
#endif

        if (!thisBuf) {
            break;
        }
#ifdef THREAD_SAFE
        printf("MetaBuffer: %p/%u size %d\n", \
               thisBuf->buffer, thisBuf->refCount, thisBuf->size);
#else
        printf("MetaBuffer: %p size %d\n", \
               thisBuf->buffer, thisBuf->size);
#endif
        if (thisBuf->metaInfo) {
#ifdef THREAD_SAFE
            pthread_mutex_lock(&thisBuf->refMutex);
#endif
            char *info = cJSON_PrintUnformatted(thisBuf->metaInfo);
            if (info) {
                printf("<meta> %s\n", info);
                _free(info);
            } else {
                printf("<meta> none\n");
            }
#ifdef THREAD_SAFE
            pthread_mutex_unlock(&thisBuf->refMutex);
#endif

        } else {
            printf("<meta> none\n");
        }

        ret = 0;
    } while (0);

    return ret;
}

