/**************************************************************
*  Filename:    LogicTriggerAgent.cxx
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: 事件触发源代理类，用来管理所有事件触发源信息.
*
*  @author:     caichunlei
*  @version     05/21/2008  caichunlei  Initial Version
**************************************************************/
#include "logictriggeragent.hxx"
#include "common/RMAPI.h"


/**
*  构造函数.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
CLogicTriggerAgent::CLogicTriggerAgent()
{
	
}

/**
*  析构函数.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
CLogicTriggerAgent::~CLogicTriggerAgent()
{
	DestoryLogicTriggerList();
}

/**
*  添加事件触发源信息.
*
*  @param  -[in]  CLogicTrigger *pLogicTrigger: [事件触发源信息]
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
void CLogicTriggerAgent::AddLogicTrigger(CLogicTrigger *pLogicTrigger)
{
	m_LogicTriggerList.insert_tail(pLogicTrigger);
}

/**
*  释放所有事件触发源信息.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
void CLogicTriggerAgent::DestoryLogicTriggerList()
{
	CLogicTriggerListIterator iter(m_LogicTriggerList);
	while(!iter.done ())
	{
		CLogicTrigger *pLogicTrigger = iter.next ();
		delete pLogicTrigger;
		
		iter++;
	}
}

/**
*  检查所有事件触发源是否满足触发条件.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
void CLogicTriggerAgent::CheckAllTriggers()
{
	//主机才进行表达式判断
	bool bRMActive = false;
	int lStatus = RM_STATUS_INACTIVE;
	long lRet = GetRMStatus(&lStatus);
	if(lRet != ICV_SUCCESS || lStatus != RM_STATUS_ACTIVE)
	{
		bRMActive = false;
	}
	else
		bRMActive = true;
	
	if(bRMActive)
	{
		CLogicTriggerListIterator iter(m_LogicTriggerList);
		while(!iter.done ())
		{
			iter.next()->CheckTrigger();
			iter++;
		}
	}
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("CLogicTriggerAgent:the machine is not active!"));
	}
}

/**
*  得到变量列表.
*
*  @param  -[in,out]  CLogicVarList*  pLogicVarList: [变量列表]
*
*  @version     08/13/2008  caichunlei  Initial Version.
*/
void CLogicTriggerAgent::GetVarList(CLogicVarList* pLogicVarList)
{
	CLogicTriggerListIterator iter(m_LogicTriggerList);

	while(!iter.done ())
	{
		iter.next()->GetVarList(pLogicVarList);
		iter++;
	}
	
}
