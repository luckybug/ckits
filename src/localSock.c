/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h> /*for socket(),bind(),close,etc*/
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/un.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <netinet/in.h> /*for sockaddr_in*/
#include <netinet/tcp.h>/*for TCP option*/
#include <arpa/inet.h>
#include <pthread.h>

#include "localSock.h"


#define CONF_SLICE_SIZE         (16384)
#define LOCALSOCK_MAX           (10)

typedef struct taglocalSockInfo
{
    int bUsed;
    int conf_fd ;
    struct sockaddr_un conf_addr ;
    char conf_buf[CONF_SLICE_SIZE];
    pthread_mutex_t atomic;
    char CONF_LOCAL_NAME[16];
    char CONF_LISTEN_NAME[16];
    int nTimeout;
}LocalSockInfo_S,*LPLocalSockInfo;

static LocalSockInfo_S s_localSock[LOCALSOCK_MAX];
static int si_init_flag = 0;
static pthread_mutex_t s_atomic;

int localSockInit()
{
    int i;

    if (si_init_flag)
        return 0;

    if (0 != pthread_mutex_init(&s_atomic, NULL))
    {
        return -1;
    }

    memset(s_localSock, 0x00, sizeof(s_localSock));

    for(i=0; i<LOCALSOCK_MAX; i++)
    {
        s_localSock[i].conf_fd = -1;
        if (0 != pthread_mutex_init(&s_localSock[i].atomic, NULL))
        {
            return -1;
        }
    }

    si_init_flag = 1;

    return 0;
}

int localSockUninit()
{
    if (!si_init_flag)
        return 0;

     int i=0;

    for(i=0; i<LOCALSOCK_MAX; i++)
    {
        if(!s_localSock[i].bUsed)
            continue;

        if (s_localSock[i].conf_fd > 0)
        {
            close(s_localSock[i].conf_fd);
        }

        s_localSock[i].conf_fd = -1;

        unlink(s_localSock[i].CONF_LOCAL_NAME);            /* OK if this fails */

        pthread_mutex_destroy(&s_localSock[i].atomic);
    }

    pthread_mutex_destroy(&s_atomic);

    si_init_flag = 0;

    return 0;
}
int getFreeIndex()
{
    int i;
    pthread_mutex_lock(&s_atomic);

    for(i=0; i<LOCALSOCK_MAX; i++)
    {
        if(!s_localSock[i].bUsed)
            break;
    }
    pthread_mutex_unlock(&s_atomic);

    return i<LOCALSOCK_MAX?i:-1;
}
int newLocalSock(const char * pLocalName,const char * pServerName, int nTimeout)
{
    if (!si_init_flag)
        return -1;

    if(NULL == pLocalName  || NULL == pServerName)
        return -1;

    int index = getFreeIndex();
    if(index < 0 )
        return -1;

    // 仅仅支持 14 字符
    snprintf(s_localSock[index].CONF_LOCAL_NAME, 15, pLocalName);
    snprintf(s_localSock[index].CONF_LISTEN_NAME, 15, pServerName);

    s_localSock[index].conf_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (s_localSock[index].conf_fd < 0)
    {
        return -1;
    }
    unlink(s_localSock[index].CONF_LOCAL_NAME);            /* OK if this fails */

    bzero(&s_localSock[index].conf_addr , sizeof(s_localSock[index].conf_addr));
    s_localSock[index].conf_addr.sun_family      = AF_UNIX;
    snprintf(s_localSock[index].conf_addr.sun_path, sizeof(s_localSock[index].conf_addr.sun_path), s_localSock[index].CONF_LOCAL_NAME);

    bind(s_localSock[index].conf_fd, (struct sockaddr *)&s_localSock[index].conf_addr, sizeof(s_localSock[index].conf_addr));

    bzero(&s_localSock[index].conf_addr , sizeof(s_localSock[index].conf_addr));
    s_localSock[index].conf_addr.sun_family      = AF_UNIX;
    snprintf(s_localSock[index].conf_addr.sun_path, sizeof(s_localSock[index].conf_addr.sun_path), s_localSock[index].CONF_LISTEN_NAME);

    s_localSock[index].bUsed = 1;
    s_localSock[index].nTimeout = nTimeout;

    return index;
}
int freeLocalSock(int index)
{
    if (!si_init_flag)
        return -1;

    if(!s_localSock[index].bUsed)
        return 0;

    pthread_mutex_lock(&s_atomic);

    if (s_localSock[index].conf_fd > 0)
    {
        close(s_localSock[index].conf_fd);
    }

    s_localSock[index].conf_fd = -1;
    unlink(s_localSock[index].CONF_LOCAL_NAME);            /* OK if this fails */

    s_localSock[index].bUsed = 0;

    pthread_mutex_unlock(&s_atomic);

    return 0;
}

static int SockSend(int index, const char *command)
{
    int ret = -1 ;

    int addrlen = sizeof(struct sockaddr_un) ;
    int len = strlen(command);

    ret = sendto(s_localSock[index].conf_fd , command, len, 0,
                 (struct sockaddr *)&s_localSock[index].conf_addr , addrlen);

    if (ret != len)
    {
        //freeLocalSock(index);

        ret = -1 ;
        syslog(LOG_ERR, "send conf message failed , ret = %d , errno %d\n", ret, errno);
    }

    return ret ;
}

static int SockRecv(int index, char * buf, int len)
{
    int ret = 0;

    // socket is blocking
    if(0xFFFF == s_localSock[index].nTimeout)
    {
        ret = recv(s_localSock[index].conf_fd , buf, len-1, 0);
        if (ret > 0)
            buf[ret] = '\0';
        else
            buf[0] = '\0';
    }
    else if(0 == s_localSock[index].nTimeout)
    {
        ret = recv(s_localSock[index].conf_fd  , buf, len-1, MSG_DONTWAIT);
    }
    else
    {
        int count = 0;
        for (count = 0; count < s_localSock[index].nTimeout; )
        {
            usleep(100000);

            ret = recv(s_localSock[index].conf_fd  , buf, len-1, MSG_DONTWAIT);
            if (ret >= 0)
            {
                break;
            }

            count += 100;
        }

    }
    return ret;
}

static int localSockRequest(int index, const char* command, char *recvbuf, int recvlen)
{
    int ret;

    if (!command || !recvbuf || !recvlen)
        return -1;

    ret = SockSend(index, command);
    if (ret >= 0)
    {
        ret = SockRecv(index, recvbuf, recvlen);
    }

    return ret;
}

int localSockSend(int index ,const char* pValue)
{
    char *conf_buf;
    int ret = 0;
    if (!si_init_flag || !pValue)
        return -1;

    if(index<0 || index>= LOCALSOCK_MAX)
        return -1;

    pthread_mutex_lock(&s_localSock[index].atomic);

    if(!s_localSock[index].bUsed  ||  s_localSock[index].conf_fd < 0)
    {
        ret = -1;
        goto done;
    }
    ret = localSockRequest(index, pValue, s_localSock[index].conf_buf, CONF_SLICE_SIZE);

done:
    pthread_mutex_unlock(&s_localSock[index].atomic);

    return ret;
}

