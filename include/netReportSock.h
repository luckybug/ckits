#ifndef __NET_REPORT_SOCK_H_
#define __NET_REPORT_SOCK_H_

#ifdef __cplusplus
extern "C"
{
#endif

int netReportSockSetLocal(const char * pLocalName);
int netReportSockInit();
int netReportSockUninit();
int net_report(const char* pValue);

#ifdef __cplusplus
};
#endif

#endif

