/**************************************************************
*  Filename:    LogicTriggerType.cxx
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: �¼�����Դ������.
*
*  @author:     caichunlei
*  @version     05/21/2008  caichunlei  Initial Version
**************************************************************/
#include "common/OS.h"
#include <ace/Timer_Wheel.h>
#include <ace/Event.h>
#include <ace/Select_Reactor.h>
#include "common/CV_Timer_Queue.h"
#include "common/pkGlobalHelper.h"
#include "logictriggertype.hxx"
#include <string>

//�������ʽ��ȫ�ֱ���
CCalcLogicExpr* g_pEvaluator = NULL;

/**
*  ��̬����ں���.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
//CTriggerType* Create_TriggerType()
//{
//	CTriggerType *pTriggerType = NULL;
//	ACE_NEW_RETURN (pTriggerType, CLogicTriggerType, 0);
//	return pTriggerType;
//}

/**
*  ���캯��.
*
*
*  @version     05/19/2008  caichunlei  Initial Version.
*/
CLogicTriggerType::CLogicTriggerType() : m_bShouldExit(false)
{
	g_pEvaluator = new CCalcLogicExpr();
	m_pCalcLogicExpr = NULL;
	//m_aceReactor = new ACE_Reactor(new ACE_Select_Reactor());
	//m_pTimeQueue = new ACE_Timer_Wheel();
	ACE_NEW(m_pTimeQueue, CV_Timer_Queue);
}

/**
*  ��������.
*
*
*  @version     05/19/2008  caichunlei  Initial Version.
*/
CLogicTriggerType::~CLogicTriggerType()
{
	if(g_pEvaluator)
		delete g_pEvaluator;
	
	if(m_pCalcLogicExpr)
		delete m_pCalcLogicExpr;
	if (m_pTimeQueue)
	{
		delete m_pTimeQueue;
	}
}

long CLogicTriggerType::InitTriggerType()
{
	m_pCalcLogicExpr = new CCalcLogicExpr();
	if(m_pCalcLogicExpr->InitCalc() != ICV_SUCCESS)
		return EC_ICV_CRS_FAILTOINITTRIGGER;
	
	//�����ȡ�¼�����Դ������Ϣ�쳣�򷵻�
	if(ReadLogicTriggerFromDB() != ICV_SUCCESS)
		return EC_ICV_CRS_FAILTOINITTRIGGER;
	
	CLogicVarList theLogicVarList;
	m_LogicTriggerAgent.GetVarList(&theLogicVarList);
	CLogicVarListIterator iter(theLogicVarList);
	std::string strVar = "";
	std::string strOld = "i_c_v";
	std::string strNew = "#";
	while(!iter.done ())
	{
		CLogicVar* pLogicVal = NULL;
		pLogicVal = iter.next();
		strVar = pLogicVal->m_szVarFullName;
		
		Repalce_All(strVar, strOld, strNew);
		m_pCalcLogicExpr->m_pfn_VarProxy_RegVar(m_pCalcLogicExpr->m_pVarProxy, strVar.c_str());	
		iter++;
		//�ͷ�LogicVarList�и����ڵ���ڴ�
		if(pLogicVal != NULL)
		{
			delete pLogicVal;
			pLogicVal = NULL;
		}
	}
	m_pCalcLogicExpr->m_pfn_VarProxy_Start(m_pCalcLogicExpr->m_pVarProxy);
	

	//��ʱ��������¼�����Դ�Ƿ����㴥������
	//�ӵ�ǰʱ�俪ʼ
	//ACE_Time_Value initialDelay(0);
	ACE_Time_Value initialDelay = m_pTimeQueue->gettimeofday();
	//1����һ��
	ACE_Time_Value interval(CV_LOGICTRIGGER_CHECKPERIOD);
	
	//�趨��ʱ��
	//long lTimerID = m_aceReactor->schedule_timer(&m_logicTriggerHandle, &m_LogicTriggerAgent, initialDelay, interval);
	long lTimerID = m_pTimeQueue->schedule(&m_logicTriggerHandle, &m_LogicTriggerAgent, initialDelay, interval);
	PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("CLogicTriggerType::schedule_timer(), delay=0, interval=1, get timerid=%d"),
		lTimerID);
	
	if(lTimerID < 0)
		return EC_ICV_CRS_FAILTOINITTRIGGER;
	
	LoopCheckTriggers();

	return ICV_SUCCESS;
}

/**
*  ��ʼ������Դ���ж�.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*/
long CLogicTriggerType::LoopCheckTriggers()
{	
	//ѭ��ִ��
	//long lRet = m_aceReactor->run_reactor_event_loop();	
	ACE_Event timer;
	
	printf("Start CLogicTrigger Timer Event Loop!\n");
	PKLog.LogMessage(PK_LOGLEVEL_INFO,_("CLogicTriggerType::LoopCheckTriggers() start.."));
	//return 1;

	//ѭ��ִ��
	while (!m_bShouldExit)
	{
		//printf(".......LoopCheckTriggers\n");
		//ACE_Time_Value max_tv = m_pTimeQueue->gettimeofday();
		ACE_Time_Value max_tv(1);
		ACE_Time_Value *this_timeout = m_pTimeQueue->calculate_timeout(&max_tv);

		if (*this_timeout == ACE_Time_Value::zero)
		{
			//printf("if (*this_timeout == ACE_Time_Value::zero)\n");
			m_pTimeQueue->expire();
		}
		else{
			// convert to absolute time
			// 			ACE_Time_Value next_timeout = m_pTimeQueue->gettimeofday();
			// 			next_timeout += *this_timeout;
			// 			printf("if (timer.wait(&next_timeout) == -1)...   %d\n",next_timeout.get_msec());
			if (m_timer.wait(this_timeout, 0) == -1)
			{
				//printf("if (timer.wait(&next_timeout) == -1)\n");
				m_pTimeQueue->expire();
			}
		}
	}

	PKLog.LogMessage(PK_LOGLEVEL_INFO,_("CLogicTriggerType::LoopCheckTriggers,Stop CLogicTrigger Timer Event Loop!"));
//	printf("Stop CLogicTrigger Timer Event Loop!\n");
	//���յ��˳������������Ϣʱ��ִ�е����ȡ����ʱ��
	//ACE_Reactor::instance()->cancel_timer(m_lTimerId);
	//�ͷ���Դ
	m_pTimeQueue->cancel(&m_logicTriggerHandle);
	
	m_pCalcLogicExpr->FiniTriggerType();
	
	//PKLog.LogMessage(PK_LOGLEVEL_INFO, "CLogicTriggerType::LoopCheckTriggers()::run_reactor_event_loop() end");
	
	/*���յ����������˳���Ϣʱ����ִ�е�����
	*ע����ʱ��
	*/
	//m_aceReactor->cancel_timer(&m_logicTriggerHandle);
	return ICV_SUCCESS;
}

/**
 *  ��ȡ�¼�����Դ��Ϣ.
 *
 *  @param  -[in]	   CCRSDBOperation& theDBOper: [�ṩ���ݿ����]
 *  @param  -[in,out]  T_LogicTriggerList& theLogicTriggerList: [�¼�����Դ��Ϣ�б�]
 *
 *  @version     13/12/2008  zhaobin  Initial Version.
 */
int CLogicTriggerType::ReadLogicTriggers(CCRSDBOperation& theDBOper, T_LogicTriggerList& theLogicTriggerList)
{
	try
	{
		int i = 0;
		int nTriNum = 0;
		int nRowCount = 0;
		int * nzTriIDs = NULL;
		CppSQLite3Table v = theDBOper.GetTable("select fd_trigger_id from t_crs_triandschemerelations \
			order by fd_trigger_id");
		nTriNum = v.numRows();
		if (nTriNum == 0)
		{
			return ICV_SUCCESS;
		}
		
		nzTriIDs = new int[nTriNum];
		for (i = 0; i < nTriNum; i ++)
		{
			v.setRow(i);
			nzTriIDs[i] = atoi(v.fieldValue(0));
		}
		
		CppSQLite3Table t = theDBOper.GetTable("select fd_trigger_id, fd_tritype_id, \
			 fd_trigger_name, fd_trigger_desc, fd_trigger_cfgvalue, fd_triggerset_id, fd_trigger_valid \
			 from t_crs_triggers order by fd_trigger_id;");
		
		//�õ���¼����
		nRowCount = t.numRows();
		for(i = 0; i < nRowCount; i++)
		{
			//�õ���n�е���Ϣ
			t.setRow(i);		
			
			//Modified20081202//
			int nTriType = atoi(t.fieldValue(1)); //�����ж��Ƿ��¼�����Դ
			if (nTriType != 2 || atoi(t.fieldValue(6)) == 0)                    //�����ǣ����ж���һ��
			{
				continue;
			}
			int nTriId = atoi(t.fieldValue(0));
			int j = 0;
			for (; j < nTriNum; j ++)
			{
				if (nzTriIDs[j] == nTriId)
					break;
			}
			if (j >= nTriNum)
				continue;
			//��������
			T_LogicTrigger* pV_LogicTrigger = new T_LogicTrigger;
			pV_LogicTrigger->m_nTriggerID = nTriId;
			PKStringHelper::Safe_StrNCpy(pV_LogicTrigger->m_szTriggerName, t.fieldValue(2), sizeof(pV_LogicTrigger->m_szTriggerName));	
			PKStringHelper::Safe_StrNCpy(pV_LogicTrigger->m_szTriggerDesc, t.fieldValue(3), sizeof(pV_LogicTrigger->m_szTriggerDesc));
			memset(pV_LogicTrigger->m_szLogicExpr, 0x00, CV_LOGICEXPR_MAXLEN);
			
			std::string strParams(t.fieldValue(4));

			int nPos0 = strParams.find(';', 0);
			if (nPos0 == std::string::npos)
			{
				delete []nzTriIDs;
				delete pV_LogicTrigger;
				return EC_ICV_CRS_EXCEPTION;
			}
			pV_LogicTrigger->m_nLogicRetType = atoi(strParams.substr(0, nPos0).c_str());

			int nPos1 = strParams.find(';', nPos0+1);
			if (nPos1 == std::string::npos)
			{
				delete []nzTriIDs;
				delete pV_LogicTrigger;
				return EC_ICV_CRS_EXCEPTION;
			}
			pV_LogicTrigger->m_nKeepTime = atoi(strParams.substr(nPos0+1, nPos1).c_str());
			PKStringHelper::Safe_StrNCpy(pV_LogicTrigger->m_szLogicExpr, strParams.substr(nPos1+1).c_str(), sizeof(pV_LogicTrigger->m_szLogicExpr));

// 			sscanf(t.fieldValue(4), "%d;%d;%s", &pV_LogicTrigger->m_nLogicRetType,
// 				&pV_LogicTrigger->m_nKeepTime, pV_LogicTrigger->m_szLogicExpr);
			
			theLogicTriggerList.insert_tail(pV_LogicTrigger);
		}//for(int i = 0; i < nRowCount; i++)
		delete []nzTriIDs;
	}
	catch(...)
	{
		PKLog.LogMessage(PK_LOGLEVEL_CRITICAL, 
			_("CCRSDBOperation:ReadLogicTriggers catches exception!"));	
		return EC_ICV_CRS_EXCEPTION;
	}

	return ICV_SUCCESS;
}
/**
*  ��SQLite���ݿ��ж�ȡ�¼�����Դ������Ϣ.
*
*
*  @version     05/21/2008  caichunlei  Initial Version.
*  @version     06/10/2008  caichunlei  ���ݿⲿ��ʹ�õ���ʵ��.
*  @version     06/10/2008  caichunlei  ���ݿⲿ��ʹ�÷ǵ���ʵ�֣�����ʹ���з����쳣.
*/
int CLogicTriggerType::ReadLogicTriggerFromDB()
{
	int nRet = 0;
	//��������
	CCRSDBOperation theCRSDBOper;
	T_LogicTriggerList theLogicTriggerList;
	
	//�����ݿ�,�����ݿ��ȡ������Ϣ
	theCRSDBOper.OpenCRSDB();
	if ((nRet = ReadLogicTriggers(theCRSDBOper, theLogicTriggerList)) != ICV_SUCCESS)
	{
		theCRSDBOper.CloseCRSDB();
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("Failed to read logic trigger from config."));//ReadLogicTriggerFromDB (306):    PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("�����ÿ��ȡ�߼�����Դʧ��"));
		return nRet;
	}
	PKLog.LogMessage(PK_LOGLEVEL_INFO, _("The accquired count of trigger is %d(CLogicTriggerType::ReadLogicTriggerFromDB)"), theLogicTriggerList.size());
	
	//��һ��ȡ��Ϣ
	T_LogicTriggerListIterator iter(theLogicTriggerList);
	while(!iter.done())
	{
		T_LogicTrigger* pV_LogicTrigger = NULL;
		pV_LogicTrigger = iter.next();
		
		//�����µ��¼�����Դ����
		CLogicTrigger* pLogicTrigger = new CLogicTrigger(pV_LogicTrigger->m_nLogicRetType);
		//�¼�����Դ��Ϣ��ֵ
		pLogicTrigger->m_nEventID = pV_LogicTrigger->m_nTriggerID;
		PKStringHelper::Safe_StrNCpy(pLogicTrigger->m_szTriggerName, pV_LogicTrigger->m_szTriggerName, sizeof(pLogicTrigger->m_szTriggerName));
		
		PKStringHelper::Safe_StrNCpy(pLogicTrigger->m_szLogicExpr, pV_LogicTrigger->m_szLogicExpr, sizeof(pLogicTrigger->m_szLogicExpr));
		pLogicTrigger->m_nKeepTime = pV_LogicTrigger->m_nKeepTime;
		pLogicTrigger->m_pCalcLogicExpr = m_pCalcLogicExpr;
		std::string strExpr = pLogicTrigger->m_szLogicExpr;
		std::string strOld = "#";
		std::string strNew = "i_c_v";
		Repalce_All(strExpr, strOld, strNew);
		strcpy(pLogicTrigger->m_szLogicExpr, strExpr.c_str());
		//���ʽ�ʷ�����
		pLogicTrigger->m_pNodeType = g_pEvaluator->ParseExpr(pLogicTrigger->m_szLogicExpr);
		if(pLogicTrigger->m_pNodeType)
		{
			//������¼�����Դ�����б�
			m_LogicTriggerAgent.AddLogicTrigger(pLogicTrigger);
		}
		else
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("Expression (%s) parsing is wrong"), pLogicTrigger->m_szLogicExpr);
			delete pLogicTrigger;
		}
		
		iter++;
	}//while(!iter.done ())
	
	PKLog.LogMessage(PK_LOGLEVEL_INFO, _("The count of trigger through parsing is %d(CLogicTriggerType::ReadLogicTriggerFromDB)"), 
		m_LogicTriggerAgent.GetLogicTriggerList()->size());
	
	//�ͷ���Դ
	for (iter.first(); !iter.done(); iter ++)
	{
		delete iter.next();
	}

	theCRSDBOper.CloseCRSDB();
	//����
	return ICV_SUCCESS;
}

//�滻ĳ���ַ����е�ָ���ַ�Ϊ�µ��ַ�
void Repalce_All(std::string& str, std::string& strOld, std::string& strNew)
{
	std::string::size_type pos(0);
	while(pos != std::string::npos)
	{
		pos = str.find(strOld, pos);
		if(pos != std::string::npos)
		{
			str.replace(pos, strOld.length(), strNew);
			pos += strOld.length();
		}
		else
			break;
	}
}