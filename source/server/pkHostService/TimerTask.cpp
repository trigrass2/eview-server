#include "TimerTask.h"
//#include <ace/Thread_Manager.h>
//#include <ace/Time_Value.h>


TimerTask::TimerTask(void)
	: m_bStop(true),
	m_pContext(NULL),
	m_pTriggerEvent(NULL),
	m_nElapsedInterval(3)
{
}
TimerTask::~TimerTask(void)
{
	Stop();
}

 
int TimerTask::svc()
{
	if (NULL == m_pTriggerEvent) 
		return -1;

	ACE_Time_Value tTimeOut(m_nElapsedInterval);
	while (!m_bStop)
	{
		int iRet = ACE_OS::event_timedwait(&m_hResetEvent, &tTimeOut, 0);
		if (m_bStop) 
			break; 

		if (0 == iRet) 
			continue; 

		if (ETIME == ACE_OS::last_error())
		{
			m_pTriggerEvent(m_pContext);
		}
	} 
	return 0;
}

int TimerTask::Start(pfnTimerTriggerEvent pTriggerEvent, void* pContext)
{  
	if (NULL == pTriggerEvent) 
		return -1;

	m_pContext = pContext;
	m_pTriggerEvent = pTriggerEvent;

	if (ACE_OS::event_init(&m_hResetEvent, 0, 0) != 0) 
		return -1;

	m_bStop = false;
	if (activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED) != 0) 
		return -1; 
	return 0;
}

int TimerTask::Stop()
{
	if (!m_bStop)
	{
		m_bStop = true;
		ACE_OS::event_signal(&m_hResetEvent);
		wait(); 
		ACE_OS::event_destroy(&m_hResetEvent); 
	} 
	return 0;
}
 
int TimerTask::ResetTimer(int nElapsedInterval)
{
	m_nElapsedInterval = nElapsedInterval;
	return ACE_OS::event_signal(&m_hResetEvent);
}  