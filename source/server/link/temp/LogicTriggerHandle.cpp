/**************************************************************
*  Filename:    LogicTriggerHandle.cxx
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: 事件触发源监测周期到达时的事件处理类.
*
*  @author:     caichunlei
*  @version     05/21/2008  caichunlei  Initial Version
**************************************************************/
#include "logictriggerhandle.hxx"

/**
*  构造函数.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
CLogicTriggerHandle::CLogicTriggerHandle()
{
	
}

/**
*  析构函数.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
CLogicTriggerHandle::~CLogicTriggerHandle()
{
	
}

/**
*  定时时间到达时的处理函数.
*
*  @param  -[in]  const ACE_Time_Value&  current_time: [当前时间]
*  @param  -[in,out]  const void*  arg: [事件参数]
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
int CLogicTriggerHandle::handle_timeout(const ACE_Time_Value &current_time, const void *arg)
{
	const CLogicTriggerAgent *pAgent =
		reinterpret_cast<const CLogicTriggerAgent *> (arg);
	CLogicTriggerAgent *pLogicTriggerAgent =
		const_cast<CLogicTriggerAgent *> (pAgent);
	
	//检查所有事件触发源是否满足触发条件
	pLogicTriggerAgent->CheckAllTriggers();
	
    return ICV_SUCCESS;
}
