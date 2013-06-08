#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "ringbuffer.h"


int rb_init(ringBuffer **rb, int size)
{
    ringBuffer *ring;

    if (rb==NULL || size <= 1024)
    {
        return -1;
    }

    size = (size + 3) & (~3);
    ring = (ringBuffer *)malloc(sizeof(ringBuffer) + size);
    if (ring == NULL)
    {
       // ERROR("Not enough memory");
        return -1;
    }

    ring->size = size;
    ring->magic = 0;

    rb_reset(ring);

    // TODO:
    ring->lock = NULL;
    ring->unlock = NULL;
    ring->pMutex = NULL;

    *rb = ring;

    return 0;
}

int rb_uninit(ringBuffer *rb)
{
    if (rb)
    {
        free(rb);
    }
    return 0;
}

int rb_reset(ringBuffer *rb)
{
    if (!rb)
        return -1;

    rb->rd_pointer = 0;
    rb->wr_pointer = 0;

    rb->InCatchCycle = 1;
    memset(rb->buffer, 0, rb->size);

    return 0;
}

int rb_write(ringBuffer *rb, unsigned char * buf, int len)
{
    int total;
    int i;

    if(len <= 0)
    {
        return 0;
    }

    total = rb_free(rb);
    if(len > total)
        len = total;
    else
        total = len;

    if (rb->lock && rb->unlock)
        rb->lock(rb->pMutex);

    i = rb->wr_pointer;
    if (i + len > rb->size)
    {
        memcpy(rb->buffer + i, buf, rb->size - i);
        buf += rb->size - i;
        len -= rb->size - i;
        i = 0;
    }
    memcpy(rb->buffer + i, buf, len);
    rb->wr_pointer = i + len;

    if(rb->wr_pointer >= rb->size)
    {
        int tmp = 0;
    }

    if(rb->wr_pointer < rb->rd_pointer)
    {
        rb->InCatchCycle = 0;
    }

    if (rb->lock && rb->unlock)
        rb->unlock(rb->pMutex);

    return total;
}

int rb_free(ringBuffer *rb)
{
    int nFreeSize;
    nFreeSize = (rb->size - rb_data_size(rb));
    return nFreeSize;
}


int rb_read(ringBuffer *rb, unsigned char * buf, int max)
{
    int total;
    int i;

    total = rb_data_size(rb);

    if(max > total)
        max = total;
    else
        total = max;

    if (rb->lock && rb->unlock)
        rb->lock(rb->pMutex);

    i = rb->rd_pointer;
    if(i + max > rb->size)
    {
        memcpy(buf, rb->buffer + i, rb->size - i);
        buf += rb->size - i;
        max -= rb->size - i;
        i = 0;
    }
    memcpy(buf, rb->buffer + i, max);
    rb->rd_pointer = i + max;

    if(rb->rd_pointer >= rb->size)
    {
        int tmp = 0;
    }

    if(rb->wr_pointer >= rb->rd_pointer)
    {
        rb->InCatchCycle = 1;
    }

    if (rb->lock && rb->unlock)
        rb->unlock(rb->pMutex);

    return total;
}

int rb_seek_bymask(ringBuffer *rb, unsigned int mask)
{
    int nSize = rb_data_size(rb);
    int i;
    int nLen = nSize >> 2;//数据对应的4字节长度

    if (rb->lock && rb->unlock)
        rb->lock(rb->pMutex);

    if (rb->rd_pointer >= rb->size)
    {
        rb->rd_pointer = 0;
    }
    for (i=0; i<nLen; i++)
    {
        //unsigned int *pTemp = (unsigned int *)(rb->buffer+rb->rd_pointer);
        unsigned int tmp ;
        memcpy(&tmp, (rb->buffer+rb->rd_pointer), 4);
        if (tmp == mask)//帧头标识
        {
            if(rb->wr_pointer >= rb->rd_pointer)
            {
                rb->InCatchCycle = 1;
            }
            if (rb->lock && rb->unlock)
                rb->unlock(rb->pMutex);
            return 0;
        }
        if (rb->rd_pointer > rb->size - 4)
        {
            rb->rd_pointer = 0;
        }
        else
        {
            rb->rd_pointer += 4;
        }

    }
    if (rb->wr_pointer >= rb->rd_pointer)
    {
        rb->InCatchCycle = 1;
    }

    if (rb->lock && rb->unlock)
        rb->unlock(rb->pMutex);

    return -1;
}

int rb_peek_header(ringBuffer *rb,  void * pHead, unsigned int nLen)
{
    char *buf= (char *)pHead;

    int i;

    //int nLen = 16;//帧头长度
    if (rb->lock && rb->unlock)
        rb->lock(rb->pMutex);

    i = rb->rd_pointer;
    if (i + nLen > rb->size)
    {
        memcpy(buf, rb->buffer + i, rb->size - i);
        buf += rb->size - i;
        nLen -= rb->size - i;
        i = 0;
    }
    memcpy(buf, rb->buffer + i, nLen);
    if (rb->lock && rb->unlock)
        rb->unlock(rb->pMutex);
    return 0;
}

int rb_data_size(ringBuffer *rb)
{
    int nDataSize;
    if (rb->lock && rb->unlock)
        rb->lock(rb->pMutex);
    if (rb->InCatchCycle)
    {
        nDataSize = (rb->wr_pointer - rb->rd_pointer);
    }
    else
    {
        nDataSize = (rb->size - rb->rd_pointer + rb->wr_pointer);
    }
    if (rb->lock && rb->unlock)
        rb->unlock(rb->pMutex);

    return nDataSize;
}


int rb_clear(ringBuffer *rb)
{
    if (rb->lock && rb->unlock)
        rb->lock(rb->pMutex);
    memset(rb->buffer,0,rb->size);
    rb->rd_pointer=0;
    rb->wr_pointer=0;
    rb->InCatchCycle = 1;
    if (rb->lock && rb->unlock)
        rb->unlock(rb->pMutex);
    return 0;
}

