/**************************************************************
 *  Filename:    logictriggeragent.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 事件触发源代理类，用来管理所有事件触发源信息.
 *
 *  @author:     caichunlei
 *  @version     05/21/2008  caichunlei  Initial Version
**************************************************************/
#if !defined(AFX_LOGICTRIGGERAGENT_HXX__C0E48E9D_4849_4E1C_BF1A_012392718073__INCLUDED_)
#define AFX_LOGICTRIGGERAGENT_HXX__C0E48E9D_4849_4E1C_BF1A_012392718073__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "logicvar.hxx"

#include "logictrigger.hxx"

class CLogicTriggerAgent  
{
public:
	CLogicTriggerAgent();
	virtual ~CLogicTriggerAgent();

	//添加事件触发源信息
	void	AddLogicTrigger(CLogicTrigger *pLogicTrigger);
	//释放所有事件触发源信息
	void	DestoryLogicTriggerList();
	//检查所有事件触发源是否满足触发条件
	void	CheckAllTriggers();
	//得到变量列表
	void	GetVarList(CLogicVarList* pLogicVarList);
	// 得到逻辑表达式列表
	CLogicTriggerList *GetLogicTriggerList(){return &m_LogicTriggerList;};
private:
	//事件触发源列表
	CLogicTriggerList m_LogicTriggerList;
};

#endif // !defined(AFX_LOGICTRIGGERAGENT_HXX__C0E48E9D_4849_4E1C_BF1A_012392718073__INCLUDED_)
