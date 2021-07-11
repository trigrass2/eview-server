/**************************************************************
*  Filename:    LogicTrigger.cxx
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: 事件触发源类.
*
*  @author:     caichunlei
*  @version     05/21/2008  caichunlei  Initial Version
*  @version     10/15/2008  wangjian	 Fix test bug ID 91873
**************************************************************/
#include "logictrigger.hxx"
#include "common/pkGlobalHelper.h"
#include <math.h>

extern PFN_LinkEventCallback g_pfnNotifyTriggered;

/**
*  构造函数.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*  @version     10/15/2008  wangjian	 Fix test bug ID 91873
*               
*/
CLogicTrigger::CLogicTrigger(int nLogicType)
{
	memset(m_szLogicExpr, 0x00, CV_LOGICEXPR_MAXLEN);
	m_nLogicType = nLogicType;
	m_nKeepTime = 0;
	m_pNodeType = NULL;
	m_bCheckResult = false;
	m_dCurrValue = 0;
	m_dLastValue = 0;
	m_bIsSatisfy = false;
	m_lSatisfyTime = 0;
	m_pCalcLogicExpr = NULL;
	
	switch(m_nLogicType)
	{
	case LOGIC_DATACHANGE:
		m_bLastCalcRet = false;
		break;
	case LOGIC_ONCEFALSE: 
	case LOGIC_ALWAYSTRUE:
		m_bLastCalcRet = false;
		break;
	case LOGIC_ONCETRUE:  
	case LOGIC_ALWAYSFALSE:
		m_bLastCalcRet = true;
		break;
	default:
		m_bLastCalcRet = false;
		break;
	}
	
	//触发源ID
	m_nEventID = -1;
	//触发源名称
	memset(m_szTriggerName, 0, sizeof(m_szTriggerName));
	//触发源描述
	memset(m_szTriggerDesc, 0, sizeof(m_szTriggerName));
}

/**
*  析构函数.
*
*
*  @version     05/19/2008  caichunlei  Initial Version.
*/
CLogicTrigger::~CLogicTrigger()
{
	//删除节点指针
	m_pCalcLogicExpr->DeleteNode(m_pNodeType);
}

/**
*  检查触发源是否满足触发条件.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
void CLogicTrigger::CheckTrigger()
{
	//根据事件触发源类型进行处理
	switch(m_nLogicType)
	{
	case LOGIC_DATACHANGE://数据变化时触发
		DataChangeLogic();
		break;
	case LOGIC_ONCETRUE://数据为真时触发
		OnceTrueLogic();
		break;
	case LOGIC_ONCEFALSE://数据为假时触发
		OnceFalseLogic();
		break;
	case LOGIC_ALWAYSTRUE://数据总为真时触发
		AlwaysTrueLogic();
		break;
	case LOGIC_ALWAYSFALSE://数据总为假时触发
		AlwaysFalseLogic();
		break;
	default:
		break;
	}
}

/**
*  数据变化时的处理函数.
*
*
*  @version     05/30/2008  caichunlei  Initial Version.
*  @version     06/20/2008  caichunlei  修改日志信息.
*/
void CLogicTrigger::DataChangeLogic()
{
	//表达式对应的节点类型为变量才可以进行数据变化时触发
	if(m_pNodeType->type != typeVar)
	{
		PKLog.LogMessage(PK_LOGLEVEL_INFO, _("Expression (%s) is not valid vairable!"), m_szLogicExpr);
		return;
	}
	
	//清空错误信息，便于得到计算后的错误信息
	m_pCalcLogicExpr->SetErrMsg("");
	//得到表达式当前值
	m_dCurrValue = m_pCalcLogicExpr->DoCalcTree(m_pNodeType);
	if(m_pCalcLogicExpr->IsSuccess())
	{
		//上次计算失败
		if(!m_bLastCalcRet)
		{
			m_bLastCalcRet = true;
			m_dLastValue = m_dCurrValue;
		}
		else
		{
			if(fabs(m_dCurrValue - m_dLastValue) >= DOUBLE_EPSILON)
			{
				//通知预案处理模块
				if(g_pfnNotifyTriggered)
				{
					g_pfnNotifyTriggered(m_nEventID);
				}
				PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("Value from %f to %f!"), m_dLastValue, m_dCurrValue);
				m_dLastValue = m_dCurrValue;
			}
		}
	}
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("CLogicTrigger:expression(%s)value calculating is wrong (%s)!"), 
			m_szLogicExpr, m_pCalcLogicExpr->GetErrMsg());
		
		m_bLastCalcRet = false;
	}
}

/**
*  表达式为真时的的处理函数.
*
*
*  @version     05/30/2008  caichunlei  Initial Version.
*  @version     06/20/2008  caichunlei  修改日志信息.
*/
void CLogicTrigger::OnceTrueLogic()
{
	//计算表达式
	m_bCheckResult = m_pCalcLogicExpr->CalculateBool(m_pNodeType);
	if(m_pCalcLogicExpr->IsSuccess())//表达式计算成功
	{
		//表达式由false变为true，满足触发条件
		if((m_bCheckResult == true) && (m_bLastCalcRet == false))
		{
			if(g_pfnNotifyTriggered)
				g_pfnNotifyTriggered(m_nEventID);
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("CLogicTrigger:expression %s from false to true!"), m_szLogicExpr);
		}
		
		//记录表达式判断结果
		m_bLastCalcRet = m_bCheckResult;	
	}
	else//表达式计算不成功
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("CLogicTrigger:expression (%s) value calculating is wrong (%s)!"), 
			m_szLogicExpr, m_pCalcLogicExpr->GetErrMsg());
		
		//m_bLastCalcRet = true;//初始化上次表达式计算结果
		//m_bLastCalcRet = false;//初始化上次表达式计算结果
	}
}

/**
*  表达式为假时的处理函数.
*
*
*  @version     05/30/2008  caichunlei  Initial Version.
*  @version     06/20/2008  caichunlei  修改日志信息.
*/
void CLogicTrigger::OnceFalseLogic()
{
	//计算表达式
	m_bCheckResult = m_pCalcLogicExpr->CalculateBool(m_pNodeType);
	if(m_pCalcLogicExpr->IsSuccess())//表达式计算成功
	{
		//表达式由true变为false，满足触发条件
		if((m_bCheckResult == false) && (m_bLastCalcRet == true))
		{
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("CLogicTrigger:expression %s from true to false!"), m_szLogicExpr);
			if(g_pfnNotifyTriggered)
				g_pfnNotifyTriggered(m_nEventID);
			
		}
		
		//记录表达式判断结果
		m_bLastCalcRet = m_bCheckResult;	
	}
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("CLogicTrigger:expression (%s) value caculating is wrong (%s)!"), 
			m_szLogicExpr, m_pCalcLogicExpr->GetErrMsg());
		
		//m_bLastCalcRet = false;//初始化上次表达式计算结果
		//m_bLastCalcRet = true;//初始化上次表达式计算结果
	}
}

/**
*  表达式总为真时的的处理函数.
*
*
*  @version     05/30/2008  caichunlei  Initial Version.
*  @version     06/20/2008  caichunlei  修改日志信息.
*/
void CLogicTrigger::AlwaysTrueLogic()
{
	//计算表达式
	m_bCheckResult = m_pCalcLogicExpr->CalculateBool(m_pNodeType);
	if(m_pCalcLogicExpr->IsSuccess())//表达式计算成功
	{
		//只有本次表达式为真、上次表达式为真、未触发、大于定义的持续时间才触发
		if((m_bCheckResult == true) && (m_bLastCalcRet == true)
			&& (m_bIsSatisfy == false) && (time(0)-m_lSatisfyTime >= m_nKeepTime))
		{
			m_bIsSatisfy = true;
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("CLogicTrigger:expression %s is always true!"), m_szLogicExpr);
			if(g_pfnNotifyTriggered)
				g_pfnNotifyTriggered(m_nEventID);
			return;
		}
		else//不满足触发条件
		{
			//本次表达式为假
			if(m_bCheckResult == false)
			{
				//肯定不满足
				m_bIsSatisfy = false;
				//重置满足条件开始时间
				m_lSatisfyTime = 0;
			}
			else if(m_bLastCalcRet == false)
			{
				//表达式为真并且上次表达式为假，记录满足条件开始时间
				m_lSatisfyTime = time(0);	
			}
			//记录表达式判断结果
			m_bLastCalcRet = m_bCheckResult;
		}//else
	}
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("CLogicTrigger:expression (%s) value calculatint is wrong(%s)!"), 
			m_szLogicExpr, m_pCalcLogicExpr->GetErrMsg());
		
		m_bLastCalcRet = false;
		m_bIsSatisfy = false;
		m_lSatisfyTime = 0;
	}
}

/**
*  表达式总为假时的的处理函数.
*
*
*  @version     05/30/2008  caichunlei  Initial Version.
*  @version     06/20/2008  caichunlei  修改日志信息.
*/
void CLogicTrigger::AlwaysFalseLogic()
{
	//计算表达式
	m_bCheckResult = m_pCalcLogicExpr->CalculateBool(m_pNodeType);
	if(m_pCalcLogicExpr->IsSuccess())//表达式计算成功
	{
		//只有本次表达式为假、上次表达式为假、未触发、大于定义的持续时间才触发
		if((m_bCheckResult == false) && (m_bLastCalcRet == false)
			&& (m_bIsSatisfy == false) && (time(0)-m_lSatisfyTime >= m_nKeepTime))
		{
			m_bIsSatisfy = true;
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("CLogicTrigger:expression %s is alway false!\n"), m_szLogicExpr);
			if(g_pfnNotifyTriggered)
				g_pfnNotifyTriggered(m_nEventID);
			return;
		}
		else//不满足触发条件
		{
			//本次表达式为假
			if(m_bCheckResult == true)
			{
				//肯定不满足
				m_bIsSatisfy = false;
				//重置满足条件开始时间
				m_lSatisfyTime = 0;
			}
			else if(m_bLastCalcRet == true)
			{
				//表达式为假并且上次表达式为假，记录满足条件开始时间
				m_lSatisfyTime = time(0);	
			}
			//记录表达式判断结果
			m_bLastCalcRet = m_bCheckResult;
		}//else
	}
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("CLogicTrigger:expression (%s) value calculating is wrong (%s)!"), 
			m_szLogicExpr, m_pCalcLogicExpr->GetErrMsg());
		
		m_bLastCalcRet = true;
		m_bIsSatisfy = false;
		m_lSatisfyTime = 0;
	}
}

/**
*  得到表达式中的变量列表.
*
*  @param  -[in,out]  CLogicVarList*  pLogicVarList: [变量列表]
*
*  @version     08/13/2008  caichunlei  Initial Version.
*/
void CLogicTrigger::GetVarList(CLogicVarList* pLogicVarList)
{
	GetVarListByNodeType(m_pNodeType, pLogicVarList);
}

/**
*  根据根节点得到表达式中的变量列表.
*
*  @param  -[in,out]  nodeType*  pNodeType: [根节点类型]
*  @param  -[in,out]  CLogicVarList*  pLogicVarList: [变量列表]
*
*  @version     08/13/2008  caichunlei  Initial Version.
*/
void CLogicTrigger::GetVarListByNodeType(nodeType *pNodeType, CLogicVarList* pLogicVarList)
{
	vector<string> vars;
	m_pCalcLogicExpr->GetVars(pNodeType, vars);
	for(int i = 0; i < vars.size(); i++)
		AddNameToVarList(vars[i].c_str(), pLogicVarList);
}

/**
*  添加变量名称到变量列表.
*
*  @param  -[in,out]  const char*  pVarName: [comment]
*  @param  -[in,out]  CLogicVarList*  pLogicVarList: [comment]
*
*  @version     08/13/2008  caichunlei  Initial Version.
*/
void CLogicTrigger::AddNameToVarList(const char* pVarName, CLogicVarList* pLogicVarList)
{
	CLogicVarListIterator iter(*pLogicVarList);
	while(!iter.done())
	{
		if(strcmp(iter.next()->m_szVarFullName, pVarName) == 0)
			return;
		
		iter++;
	}
	
	if(strlen(pVarName) <= ICV_LONGOBJTAGNAME_MAXLEN)
	{
		CLogicVar* pLogicVar = new CLogicVar();
		PKStringHelper::Safe_StrNCpy(pLogicVar->m_szVarFullName, pVarName, sizeof(pLogicVar->m_szVarFullName));
		pLogicVarList->insert_tail(pLogicVar);
	}
}
