/**************************************************************
 *  Filename:    logictriggertype.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 事件触发源类型类.
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

#define	EC_ICV_RESPONSE_FAILTOINITTRIGGER		-1		//初始化触发源失败

typedef struct
{
	//触发源ID
	int		m_nTriggerID;
	//触发源名称
	char	m_szTriggerName[CV_CRSCOMMNAME_MAXLEN];
	//触发源描述//Modified20081202//
	char	m_szTriggerDesc[CV_CRSDESC_MAXLEN];
	//触发源表达式
	char	m_szLogicExpr[CV_LOGICEXPR_MAXLEN];
	//表达式结果判断类型
	int		m_nLogicRetType;
	//持续时间，单位为秒
	int		m_nKeepTime;
}T_LogicTrigger;

typedef ACE_DLList<T_LogicTrigger> T_LogicTriggerList;
typedef ACE_DLList_Iterator<T_LogicTrigger> T_LogicTriggerListIterator;

class CLogicTriggerType 
{
public:
	CLogicTriggerType();
	virtual ~CLogicTriggerType();

	//初始化触发源并判定
	long	InitTriggerType();

	long	LoopCheckTriggers();
	//从SQLite数据库中读取逻辑触发源配置信息.
	
    //读取事件触发源信息.
	int		ReadLogicTriggers(CCRSDBOperation&, T_LogicTriggerList&);
	int		ReadLogicTriggerFromDB();
	bool				m_bShouldExit;

protected:
	//管理所有事件触发源的代理类变量
	CLogicTriggerAgent	m_LogicTriggerAgent;
	//计算逻辑表达式当前值的变量
	CCalcLogicExpr*		m_pCalcLogicExpr;
	CLogicTriggerHandle m_logicTriggerHandle;
	ACE_Timer_Queue*	m_pTimeQueue;
	ACE_Event				m_timer;
};
void Repalce_All(std::string& str, std::string& strOld, std::string& strNew);
#endif // !defined(AFX_LOGICTRIGGERTYPE_HXX__E6399C7A_9B86_422C_9F75_4D8F3C432563__INCLUDED_)
