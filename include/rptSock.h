#ifndef RPTSOCK_H
#define RPTSOCK_H

#ifdef __cplusplus
extern "C"
{
#endif

int rptSockInit();
int rptSockUninit();

// 必须在 rptSockInit 前使用才生效
inline int rptSockSetLocal(const char * pLocalName);

int rptSockClearInfo(const char *pInfo);
int rptSockSendInfo(const char *pInfo, const char *pValue, int bSendNow);


#ifdef __cplusplus
};
#endif

#endif
