/**************************************************************
 *  Filename:    logictriggertype.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �¼�����Դ������.
 *
 *  @author:     caichunlei
 *  @version     05/21/2008  caichunlei  Initial Version
**************************************************************/
#if !defined(AFX_LOGICTRIGGERTYPE_HXX__E6399C7A_9B86_422C_9F75_4D8F3C432563__INCLUDED_)
#define AFX_LOGICTRIGGERTYPE_HXX__E6399C7A_9B86_422C_9F75_4D8F3C432563__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <ace/Timer_Queue.h>
#include "ace/Event_Handler.h"
#include "ace/Event.h"
#include "link/TriggerIntrl.h"
#include "link/linkdb.h"
#include "link/logicevaluator.hxx"
#include "logictriggeragent.hxx"
#include "logictriggerhandle.hxx"

#define	EC_ICV_RESPONSE_FAILTOINITTRIGGER		-1		//��ʼ������Դʧ��

typedef struct
{
	//����ԴID
	int		m_nTriggerID;
	//����Դ����
	char	m_szTriggerName[CV_CRSCOMMNAME_MAXLEN];
	//����Դ����//Modified20081202//
	char	m_szTriggerDesc[CV_CRSDESC_MAXLEN];
	//����Դ���ʽ
	char	m_szLogicExpr[CV_LOGICEXPR_MAXLEN];
	//���ʽ����ж�����
	int		m_nLogicRetType;
	//����ʱ�䣬��λΪ��
	int		m_nKeepTime;
}T_LogicTrigger;

typedef ACE_DLList<T_LogicTrigger> T_LogicTriggerList;
typedef ACE_DLList_Iterator<T_LogicTrigger> T_LogicTriggerListIterator;

class CLogicTriggerType 
{
public:
	CLogicTriggerType();
	virtual ~CLogicTriggerType();

	//��ʼ������Դ���ж�
	long	InitTriggerType();

	long	LoopCheckTriggers();
	//��SQLite���ݿ��ж�ȡ�߼�����Դ������Ϣ.
	
    //��ȡ�¼�����Դ��Ϣ.
	int		ReadLogicTriggers(CCRSDBOperation&, T_LogicTriggerList&);
	int		ReadLogicTriggerFromDB();
	bool				m_bShouldExit;

protected:
	//���������¼�����Դ�Ĵ��������
	CLogicTriggerAgent	m_LogicTriggerAgent;
	//�����߼����ʽ��ǰֵ�ı���
	CCalcLogicExpr*		m_pCalcLogicExpr;
	CLogicTriggerHandle m_logicTriggerHandle;
	ACE_Timer_Queue*	m_pTimeQueue;
	ACE_Event				m_timer;
};
void Repalce_All(std::string& str, std::string& strOld, std::string& strNew);
#endif // !defined(AFX_LOGICTRIGGERTYPE_HXX__E6399C7A_9B86_422C_9F75_4D8F3C432563__INCLUDED_)
