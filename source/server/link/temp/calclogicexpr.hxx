/**************************************************************
 *  Filename:    calclogicexpr.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 根据计算逻辑表达式对应的节点类型计算表达式结果.
 *
 *  @author:     caichunlei
 *  @version     05/29/2008  caichunlei  Initial Version
 *  @version     06/10/2008  caichunlei  添加初始化函数，加载不同的动态库
**************************************************************/
#if !defined(AFX_CALCLOGICEXPR_HXX__AB278977_A851_4BF4_9DDF_F23F44D20754__INCLUDED_)
#define AFX_CALCLOGICEXPR_HXX__AB278977_A851_4BF4_9DDF_F23F44D20754__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ace/Task.h"

#include "errcode/ErrCode_iCV_Common.hxx"
#include "errcode/ErrCode_iCV_CRS.h"
#include "pklog/pklog.h"

#include "link/linkdb.h"
#include "link/VarProxyIntrl.h"

//#include "calcexpr.hxx"
#include "exprcalc/OpCalcNumerical.h"
#define CV_CRSCOMMNAME_MAXLEN	128
#define CV_LOGICEXPR_MAXLEN		128
#define CV_CRSDESC_MAXLEN	128
typedef struct
{	
	//配置信息ID
	int		m_nCfgID;
	//配置信息名称
	char	m_szCfgName[CV_CRSCOMMNAME_MAXLEN];
	//获取变量值的动态库名称
	char	m_szGetVarValueDll[ICV_LONGFILENAME_MAXLEN];
	//使用该动态库
	bool	m_bIsActive;
}T_LogicTriggerCfgInfo;

typedef ACE_DLList<T_LogicTriggerCfgInfo> T_LogicTriggerCfgInfoList;
typedef ACE_DLList_Iterator<T_LogicTriggerCfgInfo> T_LogicTriggerCfgInfoListIterator;

class CCalcLogicExpr: public COpCalcNumerical  
{
public:
	CCalcLogicExpr();
	virtual ~CCalcLogicExpr();

	// 继承自COpCalcNumerical
public:
	//获取变量当前值的函数
	virtual double GetDoubleVar(const char* szVar);

	// 自有函数
	//初始化函数
	int		InitCalc();
	//加载获取变量的动态库
	int		LoadGetVarValueDll(const char* szDllName);

	long	FiniTriggerType();

    //读取事件触发源获取变量值的配置信息.
	int		ReadLogicTriggerCfgInfos(CCRSDBOperation&, T_LogicTriggerCfgInfoList&);
	bool CalculateBool(nodeType *pNodeType);
public:
	//ACE的动态库，用来加载动态库得到变量值
	ACE_DLL m_WorkingDLL;
	//获取变量的代理类
	//CVarProxy*	m_pVarProxy;
	HVAR_PROXY	m_pVarProxy;
	PFN_VarProxy_Init	m_pfn_VarProxy_Init;
	PFN_VarProxy_RegVar m_pfn_VarProxy_RegVar;
	PFN_VarProxy_Exit	m_pfn_VarProxy_Fini;
	PFN_VarProxy_GetVar m_pfn_VarProxy_GetVar;
	PFN_VarProxy_Start  m_pfn_VarProxy_Start;
};

#endif // !defined(AFX_CALCLOGICEXPR_HXX__AB278977_A851_4BF4_9DDF_F23F44D20754__INCLUDED_)
