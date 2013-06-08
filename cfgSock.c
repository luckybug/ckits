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

#include "cfgSock.h"

#define CONF_LISTEN_NAME        "/tmp/cfgServer"
#define CONF_SLICE_SIZE         (16384)

static int si_init_flag     = 0;
static int s_conf_fd        = -1;
static struct sockaddr_un s_conf_addr ;
static int s_buf_index = 0;
static char s_conf_buf[4][CONF_SLICE_SIZE];
static pthread_mutex_t s_atomic;
static char CONF_LOCAL_NAME[16] = "/tmp/cfgClient";


inline int cfgSockSetLocal(const char * pLocalName)
{
    if (pLocalName)
    {   // 仅仅支持 14 字符
        snprintf(CONF_LOCAL_NAME, 15, pLocalName);
        return 0;
    }
    else
        return -1;
}

int cfgSockInit()
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

int cfgSockUninit()
{
    if (!si_init_flag)
        return 0;

    cfgSockSaveFiles();

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

static int cfgSockSend(const char *command)
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

static int cfgSockRecv(char * buf, int len)
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

static int cfgSockRequest(const char* command, char *recvbuf, int recvlen)
{
    int ret;

    if (!command || !recvbuf || !recvlen)
        return -1;

    ret = cfgSockSend(command);
    if (ret >= 0)
    {
        ret = cfgSockRecv(recvbuf, recvlen);
    }

    return ret;
}

const char *cfgSockGetValue(const char *a_pSection, const char *a_pKey, const char *a_pDefault)
{
    int nRet = -1;
    char *conf_buf;

    if (!si_init_flag || (!a_pSection  && !a_pKey))
        return NULL;

    pthread_mutex_lock(&s_atomic);
    conf_buf = s_conf_buf[s_buf_index];

    s_buf_index += 1;
    s_buf_index &= 3;

    if (a_pSection && a_pKey)
    {
        if (!a_pDefault)
            snprintf(conf_buf, CONF_SLICE_SIZE, "R %s.%s %s", a_pSection, a_pKey, "NULL");
        else
            snprintf(conf_buf, CONF_SLICE_SIZE, "R %s.%s %s", a_pSection, a_pKey, a_pDefault);
    }
    else
    {
        const char *key = (a_pSection)? a_pSection : a_pKey;

        if (!a_pDefault)
            snprintf(conf_buf, CONF_SLICE_SIZE, "R %s %s", key, "NULL");
        else
            snprintf(conf_buf, CONF_SLICE_SIZE, "R %s %s", key, a_pDefault);
    }

    cfgSockRequest(conf_buf, conf_buf, CONF_SLICE_SIZE);

    pthread_mutex_unlock(&s_atomic);

    nRet = strncmp(conf_buf, "NULL", 4);
    if (0 == nRet)
    {
        return a_pDefault;
    }

    return conf_buf;
}



int cfgSockSetValue(const char *a_pSection, const char *a_pKey, const char *a_pValue)
{
    char *conf_buf;
//    if (!si_init_flag || !a_pSection || !a_pKey || !a_pValue)
    if (!si_init_flag || (!a_pSection  && !a_pKey) || !a_pValue)
        return -1;

    pthread_mutex_lock(&s_atomic);
    conf_buf = s_conf_buf[s_buf_index];

    s_buf_index += 1;
    s_buf_index &= 3;

    if (a_pSection && a_pKey)
    {
        snprintf(conf_buf, CONF_SLICE_SIZE, "W %s.%s %s", a_pSection, a_pKey, a_pValue);
    }
    else
    {
        const char *key = (a_pSection)? a_pSection : a_pKey;

        snprintf(conf_buf, CONF_SLICE_SIZE, "W %s %s", key, a_pValue);
    }

    cfgSockRequest(conf_buf, conf_buf, CONF_SLICE_SIZE);

    pthread_mutex_unlock(&s_atomic);

    return 0;
}

int cfgSockSaveFiles()
{
    char *conf_buf;
    if (!si_init_flag)
        return -1;

    pthread_mutex_lock(&s_atomic);
    conf_buf = s_conf_buf[s_buf_index];

    s_buf_index += 1;
    s_buf_index &= 3;

    cfgSockRequest("s", conf_buf, sizeof(conf_buf));

    pthread_mutex_unlock(&s_atomic);

    return 0;
}
