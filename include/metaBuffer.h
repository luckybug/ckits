// buffer with data and its meta infomation for communication in process
#ifndef META_BUFFER_H__
#define META_BUFFER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "cJSON.h"
#include "ioStream.h"

#define IOSTREAM_TO_METABUFFER(mb, stm) \
    (mb)->buffer = (stm)->buffer; (mb)->size = (stm)->size;

typedef struct _metaBuffer metaBuffer;

// direct access to 'metaInfo', 'refCount' and call 'unref()' is DISCOURAGED,
// unless you are very clear about it's meaning. using APIs below, instead.
struct _metaBuffer
{
    char    *buffer; // user input
    size_t  size;    // user input


    cJSON  *metaInfo;
    // un-ref/free method of the buffer (as char *buffer refers to)
    // user could provide a recycling method of the buffer
    void (*unref)(void *);

    // TODO: support more than one int64_t
    int64_t ts;
#ifdef THREAD_SAFE
    pthread_mutex_t refMutex;
    // for speed, put an item in metaInfo could be more exclusive
    uint32_t refCount;
#endif
};

// CAUTION: not thread-safe, external protection is needed in multi-threading
#ifdef __cplusplus
extern "C"
{
#endif

// init and add reference
int metabInit(metaBuffer *thisBuf);
int metabFree(metaBuffer *thisBuf);

#ifdef THREAD_SAFE
// increase reference
int metabIncRef(metaBuffer *thisBuf);
// decrease reference and free
int metabDecRef(metaBuffer *thisBuf);
#endif

// token is duplicated using strdup() while put in and get out,
// user is response for memory clean
int metabSetToken(metaBuffer *thisBuf, const char *token, const char* value);
int metabGetToken(metaBuffer *thisBuf, const char *token, const char** value);

int metabSetInt32(metaBuffer *thisBuf, const char *metaName, int32_t value);
int metabGetInt32(metaBuffer *thisBuf, const char *metaName, int32_t* value);

int metabSetInt64(metaBuffer *thisBuf, const char *metaName, int64_t value);
int metabGetInt64(metaBuffer *thisBuf, const char *metaName, int64_t* value);

int metabSetBool(metaBuffer *thisBuf, const char *metaName, bool value);
int metabGetBool(metaBuffer *thisBuf, const char *metaName, bool* value);

int metabSetFloat(metaBuffer *thisBuf, const char *metaName, double value);
int metabGetFloat(metaBuffer *thisBuf, const char *metaName, double* value);


void metabDump(metaBuffer *thisBuf);

#ifdef __cplusplus
};
#endif

#endif

