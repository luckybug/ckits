
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include "list.h"

#include "timeList.h"

#if defined(DEBUG_TIMELIST) && defined(NWDEBUG)
#define list_debug(str...) printf(str)
#else
#define list_debug(str...)
#endif

#define timeEventInterval(x, y) time_interval((x)->tvRemain, (y)->tvRemain)

static const int MILLION = 1000000;
static const struct timeval DELAY_ZERO = {0, 0};


static const alarmEventNode *
queryAlarmEventNode(timeListType *lst, unsigned int token);


/******************************************************************************************
* ��������: initTimeList
* ��������:
* @iMaxSlots:Ԥ����Ķ�ʱ��Ԫ��
* ����ֵ: �ɹ� =0, ʧ�� <0
* ��������: ��ʼ��һ����ʱ����
******************************************************************************************/
int initTimeList(timeListType *lst)
{
    if(NULL == lst)
        return -1;

#ifdef THREAD_SAFE
    pthread_mutex_init(&(lst->listMutex), NULL);
#endif

    lst->eventHead.tvRemain.tv_sec = 0x7FFFFFFF;
    lst->eventHead.tvRemain.tv_usec = MILLION - 1;
    lst->eventHead.cbTimeout = NULL;
    lst->eventHead.idToken = -1;
    INIT_LIST_HEAD(&lst->eventHead.list);

    gettimeofday(&(lst->tvLastSync), 0);

    lst->listToken = 1;

    return 0;
}

/******************************************************************************************
* ��������: uninitTimeList
* ��������:
* @list_ptr: ��ʱ����
*
* ����ֵ:
* ��������: ���һ����ʱ����
******************************************************************************************/
void uninitTimeList(timeListType* lst)
{
    if (NULL == lst)
        return ;

#ifdef THREAD_SAFE
    pthread_mutex_destroy(&lst->listMutex);
#endif
}


/* res = tv2 - tv1 */
static struct timeval time_interval(struct timeval tv2, struct timeval tv1)
{
    struct timeval ret;
    ret.tv_sec = tv2.tv_sec - tv1.tv_sec;
    ret.tv_usec = tv2.tv_usec - tv1.tv_usec;

    if (ret.tv_usec < 0)
    {
        ret.tv_usec += MILLION;
        ret.tv_sec--;
    }

    if (ret.tv_sec < 0)
    {
        ret = DELAY_ZERO;
    }

    return ret;
}

/* return (tv2 >= tv1)? */
static int time_overthan(struct timeval tv2, struct timeval tv1)
{
    return (tv2.tv_sec > tv1.tv_sec)
         || (tv2.tv_sec == tv1.tv_sec
         && tv2.tv_usec >= tv1.tv_usec);
}

/******************************************************************************************
* ��������: synchronize
* ��������:
* @list_ptr: ��ʱ����
*
* ����ֵ:
* ��������: ͬ����ʱ����
******************************************************************************************/
static void synchronize(timeListType *lst)
{
    struct timeval timeSinceLastSync;
    struct timeval timeNow;

    alarmEventNode* cur;

    if (NULL == lst)
    {
        list_debug("parma error in  synchronize\n");
        return;
    }

    // First, figure out how much time has elapsed since the last sync:
    gettimeofday(&timeNow, 0);

    timeSinceLastSync = time_interval(timeNow, lst->tvLastSync);
    lst->tvLastSync = timeNow;

#ifdef THREAD_SAFE
    pthread_mutex_lock(&lst->listMutex);
#endif

    // Then, adjust the delay queue for any entries whose time is up:
    list_for_each_entry(cur, &(lst->eventHead.list), list)
    {
        if (time_overthan(timeSinceLastSync, cur->tvRemain))
        {
            timeSinceLastSync = \
              time_interval(timeSinceLastSync, cur->tvRemain);

            cur->tvRemain = DELAY_ZERO;
        }
        else
        {
            cur->tvRemain = \
              time_interval(cur->tvRemain, timeSinceLastSync);

            break;
        }
    }

#ifdef THREAD_SAFE
    pthread_mutex_unlock(&lst->listMutex);
#endif

    return;
}

/******************************************************************************************
* ��������: addAlarmEventNode
* ��������:
* @list_ptr: ��ʱ����
* @newEntry: ����Ķ�ʱ��Ԫ
*
* ����ֵ: �ɹ����ض�ʱ�����ж�ʱ��Ԫ��ţ����򷵻�-1
* ��������: �����ݼ��뵽��ʱ������
******************************************************************************************/
int addAlarmEventNode(timeListType *lst, alarmEventNode *newEntry)
{
    alarmEventNode* cur;

    if (NULL == lst)
    {
        list_debug("parma error in  addAlarmEventNode\n");
        return -1;
    }

    synchronize(lst);

#ifdef THREAD_SAFE
    pthread_mutex_lock(&lst->listMutex);
#endif


    list_for_each_entry(cur, &(lst->eventHead.list), list)
    {
        if (time_overthan(newEntry->tvRemain, cur->tvRemain))
        {
            newEntry->tvRemain = timeEventInterval(newEntry, cur);
        }
        else
        {
            cur->tvRemain = timeEventInterval(cur, newEntry);

            break;
        }
    }

    newEntry->idToken = lst->listToken;
    lst->listToken = (lst->listToken + 1) & 0x0FFFFFFF;
    list_add_tail(&(newEntry->list), &(cur->list));

#ifdef THREAD_SAFE
    pthread_mutex_unlock(&lst->listMutex);
#endif
    return newEntry->idToken;
}


/******************************************************************************************
* ��������: timeToNextAlarmEvent
* ��������:
* @list_ptr: ��ʱ����
*
* ����ֵ: �ɹ�������һ���¼������ļ��ʱ��
* ��������: ���ض�ʱ��������һ���¼������ļ��ʱ��
******************************************************************************************/
struct timeval timeToNextAlarmEvent(timeListType *lst)
{
    struct timeval infinity = {0x7FFFFFFF, MILLION - 1};
    alarmEventNode* cur;
    list_t *cur_pos;

    if (NULL == lst)
    {
        list_debug("parma error in  addAlarmEventNode\n");
        return infinity;
    }

    synchronize(lst);
#ifdef THREAD_SAFE
    pthread_mutex_lock(&lst->listMutex);
#endif

    cur_pos = lst->eventHead.list.next;
    cur = list_entry(cur_pos, alarmEventNode, list);

#ifdef THREAD_SAFE
    pthread_mutex_unlock(&lst->listMutex);
#endif

    return cur->tvRemain;
}


/******************************************************************************************
* ��������: handleAlarmEvent
* ��������:
* @list_ptr: ��ʱ����
*
* ����ֵ: ��
* ��������: �鿴��ʱ�������׸�ʱ�䵥Ԫ��ʱ���Ƿ��þ����þ�����лص�����
******************************************************************************************/
void handleAlarmEvent(timeListType *lst)
{
    alarmEventNode* cur;
    list_t *cur_pos;

    if (NULL == lst)
    {
        list_debug("parma error in  addAlarmEventNode\n");
        return;
    }

#ifdef THREAD_SAFE
    pthread_mutex_lock(&lst->listMutex);
#endif

    cur_pos = lst->eventHead.list.next;
    cur = list_entry(cur_pos, alarmEventNode, list);

//    if (DELAY_ZERO != cur->tvRemain)
    if (cur->tvRemain.tv_sec != 0 ||
         cur->tvRemain.tv_usec != 0)
    {
        synchronize(lst);
    }

//    if (DELAY_ZERO == cur->tvRemain)
    if (cur->tvRemain.tv_sec == 0 &&
         cur->tvRemain.tv_usec < 1000/2) // it works when timer can't be more accurate than 1ms
    {
        list_del(&(cur->list));
        if (NULL != cur->cbTimeout)
            (*cur->cbTimeout)(cur);
    }

#ifdef THREAD_SAFE
    pthread_mutex_unlock(&lst->listMutex);
#endif

    return;
}

/******************************************************************************************
* ��������: delAlarmEventNode
* ��������:
* @list_ptr: ��ʱ����
* @token: ��ʱ������ʱ�䵥Ԫ�����
*
* ����ֵ: �ɹ�����ʱ�䵥Ԫ��ָ�룬ʧ�ܷ��� NULL
* ��������: ��ʱ�䵥Ԫ�Ӷ�ʱ�����в����տռ�
******************************************************************************************/
alarmEventNode * delAlarmEventNode(timeListType *lst, unsigned int token)
{
    alarmEventNode* next;
    alarmEventNode *entry_ptr = NULL;

    list_debug("enter delAlarmEventNode,lst=0x%0x\n",(int)lst);
    if (NULL == lst)
    {
        list_debug("list is NULL,return in delAlarmEventNode\n");
        return entry_ptr;
    }

    synchronize(lst);

    entry_ptr = (alarmEventNode *)queryAlarmEventNode(lst, token);
    if (NULL == entry_ptr)
        return entry_ptr;


#ifdef THREAD_SAFE
//  list_debug("before pthread_mutex_lock\n");
    pthread_mutex_lock(&lst->listMutex);
//  list_debug("after pthread_mutex_lock\n");
    if (NULL == entry_ptr->list.next)
    {
        pthread_mutex_unlock(&lst->listMutex);
        return entry_ptr;
    }
#else
    if (NULL == entry_ptr->list.next)
    {
        return entry_ptr;
    }
#endif

    // assume entry in list
    next = list_entry(entry_ptr->list.next, alarmEventNode, list);

    next->tvRemain.tv_sec += entry_ptr->tvRemain.tv_sec;
    next->tvRemain.tv_usec += entry_ptr->tvRemain.tv_usec;

    if (next->tvRemain.tv_usec >= MILLION)
    {
        next->tvRemain.tv_usec -= MILLION;
        next->tvRemain.tv_sec ++;
    }

    list_del(&(entry_ptr->list));

#ifdef THREAD_SAFE
//  list_debug("before pthread_mutex_unlock =0x%0x\n",(int)&lst->data_lock);
    pthread_mutex_unlock(&lst->listMutex);
//  list_debug("after pthread_mutex_unlock =0x%0x\n",(int)&lst->data_lock);
#endif

//  list_debug("leave timed_list_del,lst=0x%0x\n",(int)lst);
    return entry_ptr;
}

/******************************************************************************************
* ��������: timed_list_empty
* ��������:
�� @list_ptr: ��ʱ����
*
* ����ֵ: 0�ǿգ�1Ϊ��
* ��������:�ж϶�ʱ�������޶�ʱ��Ԫ
******************************************************************************************/
int isTimeListEmpty(timeListType *lst)
{
    return list_empty(&lst->eventHead.list);
}

/******************************************************************************************
* ��������: timed_list_search
* ��������:
* @list_ptr: ��ʱ����
* @token: ��ʱ������ʱ�䵥Ԫ�����
*
* ����ֵ: ���������Ķ�ʱ��Ԫָ�룬�Ҳ������ؿ�ֵ
* ��������:��������Ѱ�ҷ��������Ķ�ʱ��Ԫ
******************************************************************************************/
const alarmEventNode * queryAlarmEventNode(timeListType *lst, unsigned int token)
{
//  timeListType* lst = (timeListType*)list_ptr;
    alarmEventNode* obj = NULL;
    alarmEventNode* cur;

    list_debug("enter queryAlarmEventNode\n");
    if (NULL == lst)
    {
        list_debug("list is 0,return in dumpTimeList\n");
        list_debug("leave queryAlarmEventNode\n");
        return NULL;
    }

#ifdef THREAD_SAFE
    pthread_mutex_lock(&lst->listMutex);
#endif

    list_for_each_entry(cur, &(lst->eventHead.list), list)
    {
        if (token == cur->idToken)
        {
            obj = cur;
            break;
        }
    }

#ifdef THREAD_SAFE
    pthread_mutex_unlock(&lst->listMutex);
#endif

    return obj;
}

/******************************************************************************************
* ��������: timed_list_dump
* ��������:
* @list_ptr: ��ʱ����
*
* ����ֵ: ���ض�ʱ�����ж�ʱ��Ԫ����
* ��������: ��ӡ������ʱ����
******************************************************************************************/
int dumpTimeList(timeListType *lst)
{
    int ret = 0;
    alarmEventNode* cur;

    if(NULL == lst)
    {
        list_debug("list is 0,return in dumpTimeList\n");
        return ret;
    }

    synchronize(lst);

    printf("list has entries printed below:\n");
#ifdef THREAD_SAFE
    pthread_mutex_lock(&lst->listMutex);
#endif

    list_for_each_entry(cur, &(lst->eventHead.list), list)
    {
        printf("token %d timeout @%d/%d\n", cur->idToken,
                (int)cur->tvRemain.tv_sec,
                (int)cur->tvRemain.tv_usec);

        ret++;
    }

#ifdef THREAD_SAFE
    pthread_mutex_unlock(&lst->listMutex);
#endif
    return ret;
}

#if 0
int my_timeout(struct _alarmEventNode *thisEvent)
{
    printf("slot %d timeout\n", (int)thisEvent->cbParam);
}

void main()
{
    int i;
    int ret;
    alarmEventNode entry;
    alarmEventNode events[10];

    alarmEventNode *entry_ptr = &entry;
    timeListType thisList;

    initTimeList(&thisList);

    dumpTimeList(&thisList);

    for (i=0; i<5; i++)
    {
        events[i].tvRemain.tv_sec = (i+1)*5;
        events[i].tvRemain.tv_usec = 0;

        events[i].cbTimeout = my_timeout;
        events[i].cbParam = i;

        ret = addAlarmEventNode((&thisList), &events[i]);
    }

    dumpTimeList(&thisList);
    ret = delAlarmEventNode((&thisList), 3);
    printf("ret = %d\n", ret);
    sleep(1);

    dumpTimeList(&thisList);
    ret = delAlarmEventNode((&thisList), 1);
    printf("ret = %d\n", ret);
    sleep(1);

    while(1)
    {
        dumpTimeList(&thisList);
        handleAlarmEvent(&thisList);
        sleep(1);
    }
}
#endif

