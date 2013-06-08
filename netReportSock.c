/*
 * for unix domain socket log system, i.g. print to dvscmd shell
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

#include "netReportSock.h"

#define CONF_LISTEN_NAME        "/tmp/rptServer"
#define CONF_SLICE_SIZE         (16384)

static int si_init_flag     = 0;
static int s_conf_fd        = -1;
static struct sockaddr_un s_conf_addr ;
static int s_buf_index = 0;
static char s_conf_buf[CONF_SLICE_SIZE];
static pthread_mutex_t s_atomic;
static char CONF_LOCAL_NAME[16] = "/tmp/rptClient";


inline int netReportSockSetLocal(const char * pLocalName)
{
    if (pLocalName)
    {   // 仅仅支持 14 字符
        snprintf(CONF_LOCAL_NAME, 15, pLocalName);
        return 0;
    }
    else
        return -1;
}

int netReportSockInit()
{
    if (si_init_flag)
        return 0;

        s_conf_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (s_conf_fd < 0)
    {
        return -1;
    }

    if (0 != pthread_mutex_init(&s_atomic, NULL))
    {
        close(s_conf_fd);
        s_conf_fd = -1;

        return -1;
    }

    unlink(CONF_LOCAL_NAME);            /* OK if this fails */

    bzero(&s_conf_addr , sizeof(s_conf_addr));
    s_conf_addr.sun_family      = AF_UNIX;

    snprintf(s_conf_addr.sun_path, sizeof(s_conf_addr.sun_path), CONF_LOCAL_NAME);

    bind(s_conf_fd, (struct sockaddr *)&s_conf_addr, sizeof(s_conf_addr));

    bzero(&s_conf_addr , sizeof(s_conf_addr));
    s_conf_addr.sun_family      = AF_UNIX;

    snprintf(s_conf_addr.sun_path, sizeof(s_conf_addr.sun_path), CONF_LISTEN_NAME);

    si_init_flag = 1;

    return 0;
}

int netReportSockUninit()
{
    if (!si_init_flag)
        return 0;

    if (s_conf_fd > 0)
    {
        close(s_conf_fd);
    }

    s_conf_fd = -1;

    unlink(CONF_LOCAL_NAME);            /* OK if this fails */

    pthread_mutex_destroy(&s_atomic);

    si_init_flag = 0;

    return 0;
}

static int netReportSockSend(const char *command)
{
    int ret = -1 ;

    int addrlen = sizeof(s_conf_addr) ;
    int len = strlen(command);

    if (!si_init_flag)
        return -1;

    ret = sendto(s_conf_fd , command, len, 0,
                 (struct sockaddr *)&s_conf_addr , addrlen);

    if (ret != len)
    {
        close(s_conf_fd) ;
        s_conf_fd = -1;

        ret = -1 ;

        si_init_flag = 0;
        syslog(LOG_ERR, "send conf message failed , ret = %d , errno %d\n", ret, errno);
    }

    return ret ;
}

static int netReportSockRecv(char * buf, int len)
{
    int ret;
    if (!si_init_flag)
        return -1;

        // socket is blocking
    ret = recv(s_conf_fd , buf, len-1, 0);
    if (ret > 0)
        buf[ret] = '\0';
    else
        buf[0] = '\0';

//      printf("%s\n", buf);
    return ret;
}

static int netReportSockRequest(const char* command, char *recvbuf, int recvlen)
{
    int ret;

    if (!command || !recvbuf || !recvlen)
        return -1;

    ret = netReportSockSend(command);
    if (ret >= 0)
    {
        ret = netReportSockRecv(recvbuf, recvlen);
    }

    return ret;
}

int net_report(const char* pValue)
{
    char *conf_buf;
    if (!si_init_flag || !pValue)
        return -1;

    pthread_mutex_lock(&s_atomic);

    netReportSockRequest(pValue, s_conf_buf, CONF_SLICE_SIZE);

    pthread_mutex_unlock(&s_atomic);

    return 0;
}

