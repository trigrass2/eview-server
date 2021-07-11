/**************************************************************
 *  Filename:    logictriggerhandle.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 事件触发源监测周期到达时的事件处理类.
 *
 *  @author:     caichunlei
 *  @version     05/21/2008  caichunlei  Initial Version
**************************************************************/
#if !defined(AFX_LOGICTRIGGERHANDLE_HXX__BDAD7CA8_CEE7_470A_AB8E_15488F227DD3__INCLUDED_)
#define AFX_LOGICTRIGGERHANDLE_HXX__BDAD7CA8_CEE7_470A_AB8E_15488F227DD3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ace/Event_Handler.h"
#include "logictriggeragent.hxx"

class CLogicTriggerHandle : public ACE_Event_Handler 
{
public:
	CLogicTriggerHandle();
	virtual ~CLogicTriggerHandle();

	//定时时间到达时的处理函数
	int	handle_timeout(const ACE_Time_Value &current_time, const void *arg);
};

#endif // !defined(AFX_LOGICTRIGGERHANDLE_HXX__BDAD7CA8_CEE7_470A_AB8E_15488F227DD3__INCLUDED_)
