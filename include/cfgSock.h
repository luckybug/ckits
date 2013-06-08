#ifndef CFGSOCK_H
#define CFGSOCK_H

#ifdef __cplusplus
extern "C"
{
#endif

int cfgSockInit();
int cfgSockUninit();

// ������ cfgSockInit ǰʹ�ò���Ч
inline int cfgSockSetLocal(const char * pLocalName);

int cfgSockSetValue(const char * a_pSection, const char * a_pKey, const char * a_pValue);
const char *cfgSockGetValue(const char * a_pSection, const char * a_pKey, const char * a_pDefault);

int cfgSockSaveFiles();

#ifdef __cplusplus
};
#endif

#endif
