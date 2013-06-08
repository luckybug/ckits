#ifndef __NET_REPORT_SOCK_H_
#define __NET_REPORT_SOCK_H_

#ifdef __cplusplus
extern "C"
{
#endif

int localSockInit();
int localSockUninit();
int newLocalSock(const char * pLocalName,const char * pServerName, int nTimeout);
int freeLocalSock(int index);
int localSockSend(int index , const char* pValue);

#ifdef __cplusplus
};
#endif

#endif

