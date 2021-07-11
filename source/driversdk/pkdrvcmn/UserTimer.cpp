/**************************************************************
**************************************************************/
#include "MainTask.h"
#include "UserTimer.h"
#include "Device.h"
#include "TaskGroup.h"
#include "PythonHelper.h"
#include "pklog/pklog.h"
#include <string>
extern CPKLog g_logger;
using namespace std;

/**
 *  构造函数.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CUserTimer::CUserTimer(CDevice *pDevice):m_pDevice(pDevice)
{
    memset(&m_timerInfo, 0, sizeof(m_timerInfo));
}

/**
 *  析构函数.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CUserTimer::~CUserTimer()
{
}

CUserTimer & CUserTimer::operator=( CUserTimer &theDataBlock )
{
    if (this == &theDataBlock)
        return *this;

    memcpy(&m_timerInfo, &theDataBlock.m_timerInfo, sizeof(m_timerInfo));
    return *this;
}

void CUserTimer::StartTimer()
{
    if(m_timerInfo.nPeriodMS > 0)
    {
        ACE_Time_Value	tvPollRate;		// 扫描周期，单位ms
        tvPollRate.msec(m_timerInfo.nPeriodMS);
        ACE_Time_Value tvPollPhase;
        tvPollPhase.msec(m_timerInfo.nPhaseMS);
        tvPollPhase += ACE_Time_Value::zero;
        m_pDevice->m_pTaskGroup->m_pReactor->schedule_timer((ACE_Event_Handler *)(CUserTimer *)this, NULL, tvPollPhase, tvPollRate);
    }
}

void CUserTimer::StopTimer()
{
    m_pDevice->m_pTaskGroup->m_pReactor->cancel_timer((ACE_Event_Handler *)(CUserTimer *)this);
}

int CUserTimer::handle_timeout( const ACE_Time_Value &current_time, const void *act /*= 0*/ )
{
    // 确保控制命令被优先处理，控制处理的主流程不在这里，在TaskGroup中
    if (m_pDevice != NULL && m_pDevice->m_pTaskGroup != NULL)
        m_pDevice->m_pTaskGroup->HandleWriteCmd();

    //ACE_Timer_Queue *pTimerQue = m_pDevice->m_pTaskGroup->reactor()->timer_queue();
    //bool bIsEmptyQue = pTimerQue->get_first>is_empty();

    if(g_nDrvLang == PK_LANG_CPP) // C语言驱动
    {
		if (g_pfnOnTimer)
		{
			int nRet = g_pfnOnTimer(this->m_pDevice->m_pOutDeviceInfo, (PKTIMER *)&(this->m_timerInfo));
			//if(nRet == 0) // 返回0说明通信正常
			//	m_pDevice->SetDeviceCommStatus(1); // 返回0表示通信正常
			//else
			//	m_pDevice->SetDeviceCommStatus(0); // 返回非0表示通信不正常
		}
    }
    else if(g_nDrvLang == PK_LANG_PYTHON) // python 驱动
        Py_OnTimer(this);
    else
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "设备: %s, 定时器到达时发现驱动未定义OnTimer函数!",  this->m_pDevice->m_strName.c_str());
    return 0;
}

