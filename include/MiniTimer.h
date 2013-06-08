/*

	MiniTimer
	
使用注意：
1、在MiniTimer的回调函数里调用 MiniTimer的相关函数时,必须使用不带锁的接口版本。（否则出现死锁现象） 
2、其他情况下必须使用带锁的版本（即通用版本）
3、MiniTimerRequest 的内存分配由使用者提供，需保证在回调周期内有效
*/
#ifndef	MiniTimer_h
#define MiniTimer_h

#ifdef __cplusplus
extern "C" {
#endif

typedef void Object;
typedef void (*MiniTimerCallbackMethod)(Object *);

/* */
#define MiniTimer_Interval  (long)1000000  // one second 
#define MiniTimer_BaseTime	(long)1

typedef enum {
	MiniTimerCategory_SingleShotCallbackFunction,
	MiniTimerCategory_IntervalCallbackFunction
} MiniTimerCategory;

typedef union MiniTimerCallback_t MiniTimerCallback;
union MiniTimerCallback_t {
	void (*method)(Object *);
};

typedef struct MiniTimerRequest_t	MiniTimerRequest;
struct MiniTimerRequest_t {
	MiniTimerRequest	*next;
	MiniTimerRequest	*previous;

	MiniTimerCategory	category;

	long			initialTick;
	long			nowTick;

	Object			*object;
	MiniTimerCallback	callback;
};

typedef struct MiniTimerRequestList_t	MiniTimerRequestList;
struct MiniTimerRequestList_t	{
	MiniTimerRequest	*top;
	MiniTimerRequest	*bottom;
};

typedef struct MiniTimer_t		MiniTimer;
struct MiniTimer_t {
	MiniTimerRequestList	runList;
	MiniTimerRequestList	suspendList;
};

/* */
int MiniTimer_init(void);

int MiniTimer_unInit(void);

/* */
void MiniTimer_startSingleShotByFunction(MiniTimerRequest *request,
	long time,
	Object *object, void (*method)(Object *object));

void MiniTimer_startSingleShotByFunctionNoLock(MiniTimerRequest *request,
	long time,
	Object *object, void (*method)(Object *object));
	
/* */
void MiniTimer_startIntervalByFunction(MiniTimerRequest *request,
	long time,
	Object *object, void (*method)(Object *object));
	
void MiniTimer_startIntervalByFunctionNoLock(MiniTimerRequest *request,
	long time,
	Object *object, void (*method)(Object *object));
	
/* */
int MiniTimer_stop(MiniTimerRequest *request);
int MiniTimer_stopNoLock(MiniTimerRequest *request);
/*  */
int MiniTimer_suspend(MiniTimerRequest *request);
int MiniTimer_suspendNoLock(MiniTimerRequest *request);
/**/
int MiniTimer_resume(MiniTimerRequest *request);
int MiniTimer_resumeNoLock(MiniTimerRequest *request);

#ifdef __cplusplus
}
#endif

#endif	/* MiniTimer_h */
