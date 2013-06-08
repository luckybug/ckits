#ifndef NW_EPOLL_PROC_H
#define NW_EPOLL_PROC_H

#ifdef __cplusplus
extern "C"
{
#endif


typedef struct _epTransport epTransport;
typedef struct _epVirtualTable epVirtualTable;

typedef int (*epTransportFunc)(epTransport* _this);
typedef int (*epTimeoutHandler)(void * param);

struct _epTransport
{
	int descriptor;
	epVirtualTable *vptr;

// private:
	epTransport *next;    // linkage;
};

struct _epVirtualTable
{
    epTransportFunc handleRead;  /* called when socket got read event */
    epTransportFunc handleWrite; /* called when socket got write event */
    epTransportFunc handleClose; /* called when socket should close */
    epTransportFunc dealloc; /* when transport instance should deallocate */
// private:
//    unsigned int __structSize;
};


/******************************************************************************************
* ��������: epAction
* ��������:
*
* ����ֵ: �ɹ�ʱ����0,  ����ʱ-1
* ��������: epoll ѭ��
******************************************************************************************/
int epAction();

int epAddTransport(epTransport *transport);
epTransport * epGetTransport(int theDesc);

int epCallLater(int seconds, int useconds, epTimeoutHandler handler, void *param);
int epCancelCall(int token);

#ifdef __cplusplus
};
#endif


#endif /* __select_proc_h */

