
#ifndef NW_TIME_LIST_H__
#define NW_TIME_LIST_H__

#include "list.h"
#include <sys/time.h>

#ifdef __cplusplus
extern "C"
{
#endif


//#define THREAD_SAFE
//#define DEBUG_TIMELIST

typedef struct _alarmEventNode alarmEventNode;
typedef struct _timeListType timeListType;

// ��ʱ�¼��ص�����
typedef int (*alarmEventHandler)(alarmEventNode *thisEvent);

// ��ʱ��Ԫ����
struct _alarmEventNode
{
// public:
	struct timeval      tvRemain;	// �趨�ļ��ʱ��

	alarmEventHandler   cbTimeout;	// �ص�����ָ��
	void*               cbParam;	// �ص�����

// protected:
    unsigned int        reserved;
	unsigned int        idToken;

// private:
	list_t              list;       // timed_list use, don't modify
};


// ��ʱ����
struct _timeListType
{
// private
    alarmEventNode  eventHead;
	unsigned int    listToken;

    struct timeval  tvLastSync;
#ifdef THREAD_SAFE
	pthread_mutex_t listMutex;
#endif
};


/******************************************************************************************
* ��������: initTimeList
* ��������:
* @lst: ��ʱ����
* ����ֵ: �ɹ� =0, ʧ�� <0
* ��������: ��ʼ��һ����ʱ����
******************************************************************************************/
int initTimeList(timeListType *lst);

/******************************************************************************************
* ��������: uninitTimeList
* ��������:
* @lst: ��ʱ����
*
* ����ֵ:
* ��������: ����һ����ʱ����
******************************************************************************************/
void uninitTimeList(timeListType *lst);

/******************************************************************************************
* ��������: addAlarmEventNode
* ��������:
* @lst: ��ʱ����
* @newEntry: ����Ķ�ʱ��Ԫ
*
* ����ֵ: �ɹ����ض�ʱ�����ж�ʱ��Ԫ��ţ����򷵻�-1
* ��������: �����ݼ��뵽��ʱ������
******************************************************************************************/
int addAlarmEventNode(timeListType *lst, alarmEventNode *newEntry);

/******************************************************************************************
* ��������: delAlarmEventNode
* ��������:
* @lst: ��ʱ����
* @token: ��ʱ������ʱ�䵥Ԫ�����
*
* ����ֵ: �ɹ�����ʱ�䵥Ԫ��ָ�룬ʧ�ܷ��� NULL
* ��������: ��ʱ�䵥Ԫ�Ӷ�ʱ����ɾ��
******************************************************************************************/
alarmEventNode * delAlarmEventNode(timeListType *lst, unsigned int token);

/******************************************************************************************
* ��������: isTimeListEmpty
* ��������:
�� @lst: ��ʱ����
*
* ����ֵ: 0�ǿգ�1Ϊ��
* ��������:�ж϶�ʱ�������޶�ʱ��Ԫ
******************************************************************************************/
int isTimeListEmpty(timeListType *lst);

/******************************************************************************************
* ��������: dumpTimeList
* ��������:
* @lst: ��ʱ����
*
* ����ֵ: ���ض�ʱ�����ж�ʱ��Ԫ����
* ��������: ��ӡ������ʱ����
******************************************************************************************/
int dumpTimeList(timeListType *lst);

/******************************************************************************************
* ��������: handleAlarmEvent
* ��������:
* @lst: ��ʱ����
*
* ����ֵ: ��
* ��������: �鿴��ʱ�������׸�ʱ�䵥Ԫ��ʱ���Ƿ��þ����þ�����лص�����
******************************************************************************************/
void handleAlarmEvent(timeListType *lst);

/******************************************************************************************
* ��������: timeToNextAlarmEvent
* ��������:
* @lst: ��ʱ����
*
* ����ֵ: �ɹ�������һ���¼������ļ��ʱ��
* ��������: ���ض�ʱ��������һ���¼������ļ��ʱ��
******************************************************************************************/
struct timeval timeToNextAlarmEvent(timeListType *lst);

#ifdef __cplusplus
};
#endif

#endif

