/**************************************************************
*  Filename:    LogicTrigger.cxx
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: �¼�����Դ��.
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
*  ���캯��.
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
	
	//����ԴID
	m_nEventID = -1;
	//����Դ����
	memset(m_szTriggerName, 0, sizeof(m_szTriggerName));
	//����Դ����
	memset(m_szTriggerDesc, 0, sizeof(m_szTriggerName));
}

/**
*  ��������.
*
*
*  @version     05/19/2008  caichunlei  Initial Version.
*/
CLogicTrigger::~CLogicTrigger()
{
	//ɾ���ڵ�ָ��
	m_pCalcLogicExpr->DeleteNode(m_pNodeType);
}

/**
*  ��鴥��Դ�Ƿ����㴥������.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
void CLogicTrigger::CheckTrigger()
{
	//�����¼�����Դ���ͽ��д���
	switch(m_nLogicType)
	{
	case LOGIC_DATACHANGE://���ݱ仯ʱ����
		DataChangeLogic();
		break;
	case LOGIC_ONCETRUE://����Ϊ��ʱ����
		OnceTrueLogic();
		break;
	case LOGIC_ONCEFALSE://����Ϊ��ʱ����
		OnceFalseLogic();
		break;
	case LOGIC_ALWAYSTRUE://������Ϊ��ʱ����
		AlwaysTrueLogic();
		break;
	case LOGIC_ALWAYSFALSE://������Ϊ��ʱ����
		AlwaysFalseLogic();
		break;
	default:
		break;
	}
}

/**
*  ���ݱ仯ʱ�Ĵ�����.
*
*
*  @version     05/30/2008  caichunlei  Initial Version.
*  @version     06/20/2008  caichunlei  �޸���־��Ϣ.
*/
void CLogicTrigger::DataChangeLogic()
{
	//���ʽ��Ӧ�Ľڵ�����Ϊ�����ſ��Խ������ݱ仯ʱ����
	if(m_pNodeType->type != typeVar)
	{
		PKLog.LogMessage(PK_LOGLEVEL_INFO, _("Expression (%s) is not valid vairable!"), m_szLogicExpr);
		return;
	}
	
	//��մ�����Ϣ�����ڵõ������Ĵ�����Ϣ
	m_pCalcLogicExpr->SetErrMsg("");
	//�õ����ʽ��ǰֵ
	m_dCurrValue = m_pCalcLogicExpr->DoCalcTree(m_pNodeType);
	if(m_pCalcLogicExpr->IsSuccess())
	{
		//�ϴμ���ʧ��
		if(!m_bLastCalcRet)
		{
			m_bLastCalcRet = true;
			m_dLastValue = m_dCurrValue;
		}
		else
		{
			if(fabs(m_dCurrValue - m_dLastValue) >= DOUBLE_EPSILON)
			{
				//֪ͨԤ������ģ��
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
*  ���ʽΪ��ʱ�ĵĴ�����.
*
*
*  @version     05/30/2008  caichunlei  Initial Version.
*  @version     06/20/2008  caichunlei  �޸���־��Ϣ.
*/
void CLogicTrigger::OnceTrueLogic()
{
	//������ʽ
	m_bCheckResult = m_pCalcLogicExpr->CalculateBool(m_pNodeType);
	if(m_pCalcLogicExpr->IsSuccess())//���ʽ����ɹ�
	{
		//���ʽ��false��Ϊtrue�����㴥������
		if((m_bCheckResult == true) && (m_bLastCalcRet == false))
		{
			if(g_pfnNotifyTriggered)
				g_pfnNotifyTriggered(m_nEventID);
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("CLogicTrigger:expression %s from false to true!"), m_szLogicExpr);
		}
		
		//��¼���ʽ�жϽ��
		m_bLastCalcRet = m_bCheckResult;	
	}
	else//���ʽ���㲻�ɹ�
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("CLogicTrigger:expression (%s) value calculating is wrong (%s)!"), 
			m_szLogicExpr, m_pCalcLogicExpr->GetErrMsg());
		
		//m_bLastCalcRet = true;//��ʼ���ϴα��ʽ������
		//m_bLastCalcRet = false;//��ʼ���ϴα��ʽ������
	}
}

/**
*  ���ʽΪ��ʱ�Ĵ�����.
*
*
*  @version     05/30/2008  caichunlei  Initial Version.
*  @version     06/20/2008  caichunlei  �޸���־��Ϣ.
*/
void CLogicTrigger::OnceFalseLogic()
{
	//������ʽ
	m_bCheckResult = m_pCalcLogicExpr->CalculateBool(m_pNodeType);
	if(m_pCalcLogicExpr->IsSuccess())//���ʽ����ɹ�
	{
		//���ʽ��true��Ϊfalse�����㴥������
		if((m_bCheckResult == false) && (m_bLastCalcRet == true))
		{
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("CLogicTrigger:expression %s from true to false!"), m_szLogicExpr);
			if(g_pfnNotifyTriggered)
				g_pfnNotifyTriggered(m_nEventID);
			
		}
		
		//��¼���ʽ�жϽ��
		m_bLastCalcRet = m_bCheckResult;	
	}
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("CLogicTrigger:expression (%s) value caculating is wrong (%s)!"), 
			m_szLogicExpr, m_pCalcLogicExpr->GetErrMsg());
		
		//m_bLastCalcRet = false;//��ʼ���ϴα��ʽ������
		//m_bLastCalcRet = true;//��ʼ���ϴα��ʽ������
	}
}

/**
*  ���ʽ��Ϊ��ʱ�ĵĴ�����.
*
*
*  @version     05/30/2008  caichunlei  Initial Version.
*  @version     06/20/2008  caichunlei  �޸���־��Ϣ.
*/
void CLogicTrigger::AlwaysTrueLogic()
{
	//������ʽ
	m_bCheckResult = m_pCalcLogicExpr->CalculateBool(m_pNodeType);
	if(m_pCalcLogicExpr->IsSuccess())//���ʽ����ɹ�
	{
		//ֻ�б��α��ʽΪ�桢�ϴα��ʽΪ�桢δ���������ڶ���ĳ���ʱ��Ŵ���
		if((m_bCheckResult == true) && (m_bLastCalcRet == true)
			&& (m_bIsSatisfy == false) && (time(0)-m_lSatisfyTime >= m_nKeepTime))
		{
			m_bIsSatisfy = true;
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("CLogicTrigger:expression %s is always true!"), m_szLogicExpr);
			if(g_pfnNotifyTriggered)
				g_pfnNotifyTriggered(m_nEventID);
			return;
		}
		else//�����㴥������
		{
			//���α��ʽΪ��
			if(m_bCheckResult == false)
			{
				//�϶�������
				m_bIsSatisfy = false;
				//��������������ʼʱ��
				m_lSatisfyTime = 0;
			}
			else if(m_bLastCalcRet == false)
			{
				//���ʽΪ�沢���ϴα��ʽΪ�٣���¼����������ʼʱ��
				m_lSatisfyTime = time(0);	
			}
			//��¼���ʽ�жϽ��
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
*  ���ʽ��Ϊ��ʱ�ĵĴ�����.
*
*
*  @version     05/30/2008  caichunlei  Initial Version.
*  @version     06/20/2008  caichunlei  �޸���־��Ϣ.
*/
void CLogicTrigger::AlwaysFalseLogic()
{
	//������ʽ
	m_bCheckResult = m_pCalcLogicExpr->CalculateBool(m_pNodeType);
	if(m_pCalcLogicExpr->IsSuccess())//���ʽ����ɹ�
	{
		//ֻ�б��α��ʽΪ�١��ϴα��ʽΪ�١�δ���������ڶ���ĳ���ʱ��Ŵ���
		if((m_bCheckResult == false) && (m_bLastCalcRet == false)
			&& (m_bIsSatisfy == false) && (time(0)-m_lSatisfyTime >= m_nKeepTime))
		{
			m_bIsSatisfy = true;
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("CLogicTrigger:expression %s is alway false!\n"), m_szLogicExpr);
			if(g_pfnNotifyTriggered)
				g_pfnNotifyTriggered(m_nEventID);
			return;
		}
		else//�����㴥������
		{
			//���α��ʽΪ��
			if(m_bCheckResult == true)
			{
				//�϶�������
				m_bIsSatisfy = false;
				//��������������ʼʱ��
				m_lSatisfyTime = 0;
			}
			else if(m_bLastCalcRet == true)
			{
				//���ʽΪ�ٲ����ϴα��ʽΪ�٣���¼����������ʼʱ��
				m_lSatisfyTime = time(0);	
			}
			//��¼���ʽ�жϽ��
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
*  �õ����ʽ�еı����б�.
*
*  @param  -[in,out]  CLogicVarList*  pLogicVarList: [�����б�]
*
*  @version     08/13/2008  caichunlei  Initial Version.
*/
void CLogicTrigger::GetVarList(CLogicVarList* pLogicVarList)
{
	GetVarListByNodeType(m_pNodeType, pLogicVarList);
}

/**
*  ���ݸ��ڵ�õ����ʽ�еı����б�.
*
*  @param  -[in,out]  nodeType*  pNodeType: [���ڵ�����]
*  @param  -[in,out]  CLogicVarList*  pLogicVarList: [�����б�]
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
*  ��ӱ������Ƶ������б�.
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
