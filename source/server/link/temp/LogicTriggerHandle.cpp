/**************************************************************
*  Filename:    LogicTriggerHandle.cxx
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: �¼�����Դ������ڵ���ʱ���¼�������.
*
*  @author:     caichunlei
*  @version     05/21/2008  caichunlei  Initial Version
**************************************************************/
#include "logictriggerhandle.hxx"

/**
*  ���캯��.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
CLogicTriggerHandle::CLogicTriggerHandle()
{
	
}

/**
*  ��������.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
CLogicTriggerHandle::~CLogicTriggerHandle()
{
	
}

/**
*  ��ʱʱ�䵽��ʱ�Ĵ�����.
*
*  @param  -[in]  const ACE_Time_Value&  current_time: [��ǰʱ��]
*  @param  -[in,out]  const void*  arg: [�¼�����]
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
int CLogicTriggerHandle::handle_timeout(const ACE_Time_Value &current_time, const void *arg)
{
	const CLogicTriggerAgent *pAgent =
		reinterpret_cast<const CLogicTriggerAgent *> (arg);
	CLogicTriggerAgent *pLogicTriggerAgent =
		const_cast<CLogicTriggerAgent *> (pAgent);
	
	//��������¼�����Դ�Ƿ����㴥������
	pLogicTriggerAgent->CheckAllTriggers();
	
    return ICV_SUCCESS;
}
