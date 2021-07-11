/**************************************************************
*  Filename:    LogicTriggerAgent.cxx
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: �¼�����Դ�����࣬�������������¼�����Դ��Ϣ.
*
*  @author:     caichunlei
*  @version     05/21/2008  caichunlei  Initial Version
**************************************************************/
#include "logictriggeragent.hxx"
#include "common/RMAPI.h"


/**
*  ���캯��.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
CLogicTriggerAgent::CLogicTriggerAgent()
{
	
}

/**
*  ��������.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
CLogicTriggerAgent::~CLogicTriggerAgent()
{
	DestoryLogicTriggerList();
}

/**
*  ����¼�����Դ��Ϣ.
*
*  @param  -[in]  CLogicTrigger *pLogicTrigger: [�¼�����Դ��Ϣ]
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
void CLogicTriggerAgent::AddLogicTrigger(CLogicTrigger *pLogicTrigger)
{
	m_LogicTriggerList.insert_tail(pLogicTrigger);
}

/**
*  �ͷ������¼�����Դ��Ϣ.
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
*  ��������¼�����Դ�Ƿ����㴥������.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
void CLogicTriggerAgent::CheckAllTriggers()
{
	//�����Ž��б��ʽ�ж�
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
*  �õ������б�.
*
*  @param  -[in,out]  CLogicVarList*  pLogicVarList: [�����б�]
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
