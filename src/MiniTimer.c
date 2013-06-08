/*
	MiniTimer

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include "MiniTimer.h"



#define RUN_LIST_SENTRY		(MiniTimerRequest *)&(pthis->runList)
#define SUSPEND_LIST_SENTRY	(MiniTimerRequest *)&(pthis->suspendList)
#define msec_to_ticks(n)	(long)((n < MiniTimer_BaseTime)? 1: (n / MiniTimer_BaseTime))


static 	sem_t		s_semMiniTimerMutex;
static  pthread_t 	s_threadMiniTimer = 0;
static 	sem_t		s_semMiniTimer;
static 	sigset_t	sigset;

static void *MiniTimerThreadFunc(void *pArg);
static void MiniTimer_clock(void);

static void EnterCritical(void)
{
	sem_wait(&s_semMiniTimerMutex);	
}

static void LeaveCritical(void)
{
	sem_post(&s_semMiniTimerMutex);	
}

/* */
static void MiniTimer_initMiniTimerRequest(MiniTimerRequest *pthis,
			MiniTimerCategory category,
			long time,
			Object *object,	MiniTimerCallback callback)
{
	pthis->category = category;
	pthis->initialTick = (long)msec_to_ticks(time);
	pthis->nowTick = msec_to_ticks(time);
	pthis->object = object;
	pthis->callback = callback;
}

/*  */
static void MiniTimer_initRunList(MiniTimer *pthis)
{
	pthis->runList.top = RUN_LIST_SENTRY;
	pthis->runList.bottom = RUN_LIST_SENTRY;
}

/*  */
static void MiniTimer_linkRunList(MiniTimer *pthis, MiniTimerRequest *request)
{
	MiniTimerRequest *position = pthis->runList.top;

	while (position != RUN_LIST_SENTRY) {
		if (position->nowTick > request->nowTick) {
			position->nowTick -= request->nowTick;

			request->previous = position->previous;
			request->next = position;

			position->previous->next = request;
			position->previous = request;
			break;
		} else {
			request->nowTick -= position->nowTick;
			position = position->next;
		}
	}
	if (position == RUN_LIST_SENTRY) {
		request->previous = position->previous;
		request->next = position;

		position->previous->next = request;
		position->previous = request;
	}
}

/*  */
static int MiniTimer_unlinkRunList(MiniTimer *pthis, MiniTimerRequest *request)
{
	int returnValue = -1;

	MiniTimerRequest *position = pthis->runList.top;
	long nowTick = 0;

	while ((position != RUN_LIST_SENTRY) && (position != request)) {
		nowTick += position->nowTick;
		position = position->next;
	}
	if (position == request) {
		if (position->next != RUN_LIST_SENTRY)
			position->next->nowTick += request->nowTick;

		request->nowTick += nowTick;

		request->next->previous = request->previous;
		request->previous->next = request->next;
		returnValue = 0;
	}
	return returnValue;
}

/* */
int MiniTimer_findRunList(MiniTimer *pthis, MiniTimerRequest *request)
{
	int returnValue = -1;

	MiniTimerRequest *position = pthis->runList.top;

	while ((position != RUN_LIST_SENTRY) && (position != request)) {
		position = position->next;
	}
	if (position == request) {
		returnValue = 0;
	}
	return returnValue;
}

/*  */
static void MiniTimer_clockRunList(MiniTimer *pthis)
{
	MiniTimerRequest *position = pthis->runList.top;

	if (position != RUN_LIST_SENTRY) {
		if (position->nowTick > 0)
			position->nowTick--;

		while ((position->nowTick <= 0) && (position != RUN_LIST_SENTRY)){

			position->next->previous = position->previous;
			position->previous->next = position->next;

			switch (position->category) {
			case MiniTimerCategory_SingleShotCallbackFunction:
				(position->callback.method)(position->object);
				break;

			case MiniTimerCategory_IntervalCallbackFunction:
				position->nowTick = position->initialTick;
				MiniTimer_linkRunList(pthis, position);
				(position->callback.method)(position->object);
				break;

			default:
				printf("category:%d is unknow \n", position->category);
				break;
			}
			position = pthis->runList.top;
		}
	}
}

/* */
static void MiniTimer_initSuspendList(MiniTimer *pthis)
{
	pthis->suspendList.top = SUSPEND_LIST_SENTRY;
	pthis->suspendList.bottom = SUSPEND_LIST_SENTRY;
}

/*  */
static void MiniTimer_linkSuspendList(MiniTimer *pthis, MiniTimerRequest *request)
{
	MiniTimerRequest *position = pthis->suspendList.top;

	request->previous = position->previous;
	request->next = position;

	position->previous->next = request;
	position->previous = request;
}

/*  */
static int MiniTimer_unlinkSuspendList(MiniTimer *pthis, MiniTimerRequest *request)
{
	int returnValue = -1;

	MiniTimerRequest *position = pthis->suspendList.top;

	while ((position != SUSPEND_LIST_SENTRY) && (position != request)) {
		position = position->next;
	}
	if (position == request) {
		request->next->previous = request->previous;
		request->previous->next = request->next;
		returnValue = 0;
	}
	return returnValue;
}

/* */
static int MiniTimer_findSuspendList(MiniTimer *pthis, MiniTimerRequest *request)
{
	int returnValue = -1;

	MiniTimerRequest *position = pthis->suspendList.top;

	while ((position != SUSPEND_LIST_SENTRY) && (position != request)) {
		position = position->next;
	}
	if (position == request) {
		returnValue = 0;
	}
	return returnValue;
}

static MiniTimer articTimer;

static void MiniTime(int arg )
{
	MiniTimer_clock();
	alarm(1); 
}

/* */
int MiniTimer_init(void)
{
	int nRet = 0;
	
	MiniTimer_initRunList(&articTimer);
	MiniTimer_initSuspendList(&articTimer);
	
	nRet = sem_init(&s_semMiniTimerMutex,0,1);
	if(0!=nRet)
	{
		goto error_ret;
	}

	nRet = sem_init(&s_semMiniTimer,0,0);
	if(0!=nRet)
	{
		sem_destroy(&s_semMiniTimerMutex);
		goto error_ret;
	}
		
#if 1	
	nRet = pthread_create(&s_threadMiniTimer,NULL,MiniTimerThreadFunc,NULL);
	if(0 != nRet)
	{
		sem_destroy(&s_semMiniTimerMutex);
		sem_destroy(&s_semMiniTimer);
		goto error_ret;
	}
#else	
	signal(SIGALRM,MiniTime);
	alarm(1); 
#endif

	return 0;
	
error_ret:
	
	if(s_threadMiniTimer)
	{
		void *thread_result = NULL;
		pthread_join(s_threadMiniTimer,&thread_result);
		s_threadMiniTimer = 0;
	}
			
	return -1;	
}

int MiniTimer_unInit(void)
{
	if(s_threadMiniTimer)
	{
		void *thread_result = NULL;
		pthread_join(s_threadMiniTimer,&thread_result);
		s_threadMiniTimer = 0;
	}

	sem_destroy(&s_semMiniTimerMutex);
	sem_destroy(&s_semMiniTimer);
	
	return 0;
}

/*  */
void MiniTimer_startSingleShotByFunction(MiniTimerRequest *request,
	long time,
	Object *object, void (*method)(Object *object))
{
	EnterCritical();
	
	MiniTimer_startSingleShotByFunctionNoLock(request, time, object, method);
	
	LeaveCritical();
}

void MiniTimer_startSingleShotByFunctionNoLock(MiniTimerRequest *request,
	long time,
	Object *object, void (*method)(Object *object))
{
	MiniTimerCallback callback;
		
	if (MiniTimer_unlinkRunList(&articTimer, request) == -1)
		MiniTimer_unlinkSuspendList(&articTimer, request);

	callback.method = method;
	MiniTimer_initMiniTimerRequest(request,
		MiniTimerCategory_SingleShotCallbackFunction,
		time,
		object, callback);

	MiniTimer_linkRunList(&articTimer, request);	
}

/* */
void MiniTimer_startIntervalByFunction(MiniTimerRequest *request,
	long time,
	Object *object, void (*method)(Object *object))
{
	EnterCritical();
	
	MiniTimer_startIntervalByFunctionNoLock(request, time, object, method);
	
	LeaveCritical();
}

void MiniTimer_startIntervalByFunctionNoLock(MiniTimerRequest *request,
	long time,
	Object *object, void (*method)(Object *object))
{
	MiniTimerCallback callback;
		
	if (MiniTimer_unlinkRunList(&articTimer, request) == -1)
		MiniTimer_unlinkSuspendList(&articTimer, request);

	callback.method = method;
	MiniTimer_initMiniTimerRequest(request,
		MiniTimerCategory_IntervalCallbackFunction,
		time,
		object, callback);

	MiniTimer_linkRunList(&articTimer, request);
	
}

/* */
int MiniTimer_stop(MiniTimerRequest *request)
{
	int returnValue = 0;

	EnterCritical();

	returnValue = MiniTimer_stopNoLock(request);

	LeaveCritical();
	
	return returnValue;
}
int MiniTimer_stopNoLock(MiniTimerRequest *request)
{
	int returnValue = 0;
	
	if (MiniTimer_unlinkRunList(&articTimer, request) == -1)
		returnValue = MiniTimer_unlinkSuspendList(&articTimer, request);
	
	return returnValue;
}
/* */
int MiniTimer_suspend(MiniTimerRequest *request)
{
	int returnValue = 0;

	EnterCritical();

	returnValue = MiniTimer_suspendNoLock(request);

	LeaveCritical();
	
	return returnValue;
}
int MiniTimer_suspendNoLock(MiniTimerRequest *request)
{
	int returnValue = 0;
	
	if (MiniTimer_unlinkRunList(&articTimer, request) == 0)
		MiniTimer_linkSuspendList(&articTimer, request);
	else
		returnValue = MiniTimer_findSuspendList(&articTimer, request);
	
	return returnValue;
}
/*  */
int MiniTimer_resume(MiniTimerRequest *request)
{
	int returnValue = 0;

	EnterCritical();

	returnValue = MiniTimer_resumeNoLock(request);

	LeaveCritical();
	
	return returnValue;
}
int MiniTimer_resumeNoLock(MiniTimerRequest *request)
{
	int returnValue = 0;
	
	if (MiniTimer_unlinkSuspendList(&articTimer, request) == 0)
		MiniTimer_linkRunList(&articTimer, request);
	else
		returnValue = MiniTimer_findRunList(&articTimer, request);
	
	return returnValue;
}
/*  */
void MiniTimer_clock(void)
{
	EnterCritical();
		
	MiniTimer_clockRunList(&articTimer);
	
	LeaveCritical();	
}

long MiniTimer_GetTicks(MiniTimerRequest* pthis)
{
	int i;

	i=0;
	
	EnterCritical();
		
	if(	MiniTimer_findRunList(&articTimer,pthis)==0){
		MiniTimerRequest *position = articTimer.runList.top;
		while (position != (MiniTimerRequest *)&(articTimer.runList)) {
			if(position != pthis) {
				i += position->nowTick;
				position = position->next;
			}
			else{
				i += position->nowTick;
				break;
			}
		}
	}
	else if(MiniTimer_findSuspendList(&articTimer,pthis)){
		MiniTimerRequest *position = articTimer.suspendList.top;
		while (position != (MiniTimerRequest *)&(articTimer.suspendList)) {
			if(position != pthis) {
				i += position->nowTick;
				position = position->next;
			}
			else{
				i += position->nowTick;
				break;
			}
		}
	}
	else
		i = 0;

	LeaveCritical();	
	
	return i;
}

static void MiniTimerSigAlrmFunc(sigval_t v)
{
	sem_post(&s_semMiniTimer);

	return;
}

static int	MiniTimerCreateTimer(int seconds, int id)
{ 
	timer_t tid;
	struct sigevent se;
	struct itimerspec ts, ots;

	memset(&se, 0, sizeof(se));
	se.sigev_notify				=	SIGEV_THREAD;
	se.sigev_notify_function	=	MiniTimerSigAlrmFunc;
	se.sigev_value.sival_int	=	id;
	if(timer_create(CLOCK_REALTIME, &se, &tid)<0) 
		return -1;

	ts.it_value.tv_sec			=	1;
	ts.it_value.tv_nsec			=	0;
	ts.it_interval.tv_sec		=	seconds;
	ts.it_interval.tv_nsec		=	0;
	if(timer_settime(tid,TIMER_ABSTIME,&ts,&ots)<0)
	{	
		timer_delete(tid);
		return -1; 
	}	

	return	0;
} 

static inline void   MiniTimerusleep(long   usec)   
{   
	struct   timeval   tvSelect;   

	tvSelect.tv_sec    =   1;   
	tvSelect.tv_usec   =   0;   

	select(0,   NULL,   NULL,   NULL,   &tvSelect);   
 }  

static void *MiniTimerThreadFunc(void *pArg)
{
	int nRet = 0;
    printf("%s %s %d  pid:%d \n", __FILE__, __FUNCTION__, __LINE__, getpid());
    fflush(stdout);
	int nRunTime = 0;
	
	nRet = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
	if(0!=nRet)
	{
		exit(EXIT_FAILURE);
	}
	
	nRet = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
	if(0!=nRet)
	{
		exit(EXIT_FAILURE);
	}

/*	
	nRet = MiniTimerCreateTimer(1,1);
	if(0!=nRet)
	{
		exit(EXIT_FAILURE);
	}
	printf("MiniTimerCreateTimer :%d \n", time(NULL));
*/

	if (sigemptyset(&sigset) == -1)
	{
		exit(EXIT_FAILURE);
	}

	if (sigaddset(&sigset, SIGALRM) == -1)
	{
		exit(EXIT_FAILURE);
	}
   	if (sigprocmask(SIG_BLOCK, &sigset, NULL) == -1)
	{
		exit(EXIT_FAILURE);
	}
   
	while(1)
	{	
		if(nRunTime>60)
		{
			int sval = 0;
			//sem_getvalue(&s_semMiniTimer, &sval);
			//printf("INFO :Mini Timer Thread is Running[%d].\n",sval);
			nRunTime = 0;
		}
		
		//sem_wait(&s_semMiniTimer);
		MiniTimerusleep(MiniTimer_Interval);
		MiniTimer_clock();
		
		nRunTime++;

	}
	printf("MiniTimer :Mini Timer Thread end.\n");
	pthread_exit(0);
}


/*   
//for test

void MiniTime_callback(void *pvoid)
{
	printf("artictime callback  time:%d\n ",time(NULL));
}

int main(char *argv[], int argc)
{
	MiniTimer_init();

	MiniTimerRequest mOnTimerRequest;
	MiniTimer_startIntervalByFunction(&(mOnTimerRequest), 5, 0, MiniTime_callback);

	while(1)
	{
		sleep(10);
	}
	return 0;
}

*/
