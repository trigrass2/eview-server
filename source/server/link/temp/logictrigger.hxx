/**************************************************************
 *  Filename:    logictrigger.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 事件触发源类.
 *
 *  @author:     caichunlei
 *  @version     05/21/2008  caichunlei  Initial Version
 *  @version     06/13/2008  caichunlei  将事件触发源的宏定义放入联动系统的宏定义中
**************************************************************/
#if !defined(AFX_LOGICTRIGGER_HXX__23F314A9_ADE0_44FC_BFCC_7BD813E15B65__INCLUDED_)
#define AFX_LOGICTRIGGER_HXX__23F314A9_ADE0_44FC_BFCC_7BD813E15B65__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ace/Containers.h"

#include "pklog/pklog.h"
#include "errcode/ErrCode_iCV_Common.hxx"
#include "errcode/ErrCode_iCV_CRS.h"

#include "link/TriggerIntrl.h"
#include "link/linkdefine.h"

#include "link/nodedefines.hxx"
#include "logicvar.hxx"
#include "calclogicexpr.hxx"

class CLogicTrigger
{
public:
	CLogicTrigger(int nLogicType);
	virtual ~CLogicTrigger();
	//删除节点指针
	void	DeleteNodeType(nodeType *pNodeType);

	//检查触发源是否满足触发条件
	void	CheckTrigger();
	//数据变化时的处理函数
	void	DataChangeLogic();
	//表达式为真时的的处理函数
	void	OnceTrueLogic();
	//表达式为假时的处理函数
	void	OnceFalseLogic();
	//表达式总为真时的的处理函数
	void	AlwaysTrueLogic();
	//表达式总为假时的的处理函数
	void	AlwaysFalseLogic();
	//得到表达式中的变量列表
	void	GetVarList(CLogicVarList* pLogicVarList);
	//根据根节点得到表达式中的变量列表
	void	GetVarListByNodeType(nodeType *pNodeType, CLogicVarList* pLogicVarList);
	//添加变量名称到变量列表
	void	AddNameToVarList(const char* pVarName, CLogicVarList* pLogicVarList);
public:
	//事件触发源的逻辑表达式
	char	m_szLogicExpr[CV_LOGICEXPR_MAXLEN];
	//事件触发源类型，见LogicType的定义
	int		m_nLogicType;
	//逻辑表达式持续时间，当事件触发源类型为LOGIC_ALWAYSTRUE或LOGIC_ALWAYSFALSE时有效
	int		m_nKeepTime;
	//计算逻辑表达式的变量
	CCalcLogicExpr*	m_pCalcLogicExpr;

	//表达式对应的节点类型指针
	nodeType*	m_pNodeType;
	//表达式判断结果
	bool	m_bCheckResult;
	//本次表达式计算值
	double	m_dCurrValue;
	//上次表达式计算值
	double	m_dLastValue;
	//上次逻辑表达式判断是否成功
	bool	m_bLastCalcRet;
	//是否满足总为真或总为假
	bool	m_bIsSatisfy;
	//总为真或总为假的开始时间
	time_t	m_lSatisfyTime;

public:
	//触发源ID
	int		m_nEventID;
	//触发源名称
	char	m_szTriggerName[CV_CRSCOMMNAME_MAXLEN];
	//触发源描述
	char	m_szTriggerDesc[CV_CRSDESC_MAXLEN];
	
};

typedef ACE_DLList<CLogicTrigger> CLogicTriggerList;
typedef ACE_DLList_Iterator<CLogicTrigger> CLogicTriggerListIterator;

#endif // !defined(AFX_LOGICTRIGGER_HXX__23F314A9_ADE0_44FC_BFCC_7BD813E15B65__INCLUDED_)
