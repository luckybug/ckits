#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <stdint.h>

#define RINGBUFFER_MAGIC  0x72627546    /* 'rbuF' */


#define rb_magic_check(var,err)  {if(var->magic!=RINGBUFFER_MAGIC) return err;}


#ifdef __cplusplus
extern "C" {
#endif

struct _ringBuffer;

typedef struct _ringBuffer ringBuffer;
typedef void (*mutexOp)(void *param);
typedef int (*dataCall)(void *buf, int len);

struct _ringBuffer
{
    int wr_pointer;
    int rd_pointer;
    long magic;
    int size;
    int InCatchCycle;
    mutexOp lock;
    mutexOp unlock;
    void *pMutex;
    uint8_t buffer[0];
};


/* ring buffer functions */
int rb_init(ringBuffer **, int);
int rb_reset(ringBuffer *rb);
int rb_write(ringBuffer *, unsigned char *, int);
int rb_free(ringBuffer *);
int rb_read(ringBuffer *, unsigned char *, int);
int rb_data_size(ringBuffer *);
int rb_clear(ringBuffer *);
int rb_seek_bymask(ringBuffer *rb, unsigned int mask);
int rb_peek_header(ringBuffer *rb, void *pHead, unsigned int nLen);
int rb_uninit (ringBuffer *rb);

#ifdef __cplusplus
};
#endif

#endif
