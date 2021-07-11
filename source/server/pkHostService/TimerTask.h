#ifndef _Timer_Task_H_
#define _Timer_Task_H_
 
#include <ace/Task.h>
#include <ace/Singleton.h>
#include <ace/Null_Mutex.h>
//#include <ace/OS_NS_Thread.h>

typedef int (*pfnTimerTriggerEvent)(void *pContext);

class TimerTask : public ACE_Task_Base
{
friend class ACE_Singleton<TimerTask, ACE_Null_Mutex>;
public:
	TimerTask(void);
	~TimerTask(void);

public:
	virtual int svc ();

public: 
	int Start(pfnTimerTriggerEvent pTriggerEvent, void* pContext = NULL);
	int Stop();
	/* Seconds */
	int ResetTimer(int nElapsedInterval);

public:
	int m_nElapsedInterval;

private:
	bool m_bStop; 
	ACE_event_t m_hResetEvent;

private:
	pfnTimerTriggerEvent m_pTriggerEvent;
	void *m_pContext;
};
typedef ACE_Singleton<TimerTask, ACE_Null_Mutex> TimerTaskSingleton;
#define TIMERTASK TimerTaskSingleton::instance()

#endif //_Timer_Task_H_
