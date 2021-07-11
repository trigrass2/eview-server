/**************************************************************
 *  Filename:    ActionTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 加载服务端动作动态库运行的线程类.
 *
 *  @author:     zhaobin
 *  @version     01/06/2009  zhaobin  Initial Version
**************************************************************/
#include "pklog/pklog.h"
#include "ace/High_Res_Timer.h"
#include "ActionTask.h"
#include "MainTask.h"
extern CPKLog g_logger;
//动作客户端HMICRSINIT使用
#define		PFNNAME_INITACTION				"InitAction"	//初始化动作模块的导出函数
#define		PFNNAME_EXITACTION				"ExitAction"	//退出动作模块的导出函数
#define		PFNNAME_EXECACTION				"ExecuteAction"	//执行动作模块的导出函数

#define DEFAULT_SLEEP_TIME					100

/**
 *  构造函数.
 *
 *
 *  @version     05/19/2008  caichunlei  Initial Version.
 */
CActionTask::CActionTask()
{
	m_bStop = false;
	m_bStopped = false;
	m_pfnInitAction = NULL;
	m_pfnExitAction = NULL;
	m_pfnExecuteAction = NULL;

	memset(m_szActionTypeName, 0x00, sizeof(m_szActionTypeName));
	memset(m_szActionDllName, 0x00, sizeof(m_szActionDllName));
}

/**
 *  析构函数.
 *
 *
 *  @version     05/19/2008  caichunlei  Initial Version.
 *  @version     05/21/2008  caichunlei  添加异常判断.
 */
CActionTask::~CActionTask()
{
	//关闭触发源判定动态库
	m_dllAction.close();
}

int GetStringFromMsg(ACE_Message_Block *pMsgBlk, string &strValue)
{
	// 解析字符串长度
	int nStrLen = 0;
	memcpy(&nStrLen, pMsgBlk->rd_ptr(), 4);
	pMsgBlk->rd_ptr(4);

	// 解析字符串
	if(nStrLen > 0)
	{
		char *pValue = new char[nStrLen + 1]();
		memcpy(pValue, pMsgBlk->rd_ptr(), nStrLen);
		pMsgBlk->rd_ptr(nStrLen);

		strValue = std::string(pValue);
		delete [] pValue;
	}
	else
		strValue = "";

	return 0;
}

/**
 *  虚函数，继承自ACE_Task.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
int CActionTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();

	int nRet = OnStart();

	while(!m_bStop)
	{
		ACE_Time_Value tvSleep;
		tvSleep.msec(DEFAULT_SLEEP_TIME); // 100ms
		ACE_OS::sleep(tvSleep);

		ACE_Message_Block *pMsgBlk = NULL;
		ACE_Time_Value tvTimes = ACE_OS::gettimeofday() + ACE_Time_Value(0, 500 * 1000);
        //ACE_Time_Value tvTimes = ACE_OS::gettimeofday();
		while(getq(pMsgBlk, &tvTimes) >= 0)
		{
			// 命令号
			int nCmdCode = 0;
			memcpy(&nCmdCode, pMsgBlk->rd_ptr(), 4);
			pMsgBlk->rd_ptr(4);

			string strActionId, strActionParam, strActionParam2, strActionParam3, strActionParam4, strPriority, strJsonEventParams;
			GetStringFromMsg(pMsgBlk, strActionId);
			GetStringFromMsg(pMsgBlk, strActionParam);
			GetStringFromMsg(pMsgBlk, strActionParam2);
			GetStringFromMsg(pMsgBlk, strActionParam3);
            GetStringFromMsg(pMsgBlk, strActionParam4);
            GetStringFromMsg(pMsgBlk, strPriority);
            GetStringFromMsg(pMsgBlk, strJsonEventParams);

			pMsgBlk->release();
			
			char szActionResult[1024] = {0};
			int nRet = -1;
			if(m_pfnExecuteAction)
			{
				nRet = m_pfnExecuteAction(
                  (char *)strActionId.c_str(), 
                  (char *)strActionParam.c_str(), 
                  (char *)strActionParam2.c_str(),  
                  (char *)strActionParam3.c_str(),  
                  (char *)strActionParam4.c_str(), 
                  (char *)strPriority.c_str(),
                  (char *)strJsonEventParams.c_str(),
                    szActionResult, sizeof(szActionResult));
			}
			else
			{
				strcpy(szActionResult, "失败,无此动作方法,");
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "%s,ACTIONID= %s 的m_pfnExecuteAction为空",szActionResult,strActionId.c_str());
				nRet = -100;
			}

			// 把结果返回给MainTask
			MAIN_TASK->PostActionResult((char *)strActionId.c_str(), nRet, (char *)szActionResult); 
		} // while(getq(pMsgBlk, &tvTimes) >= 0)
	} // while(!m_bStop)

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"LinkServer exit normally!");
	OnStop();

	m_bStopped = true;
	return nRet;	
}

/**
 *  Start the Task and threads in the task are activated.
 *
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     05/23/2008  shijunpu  Initial Version.
 */
int CActionTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}


void CActionTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}


int CActionTask::OnStart()
{
	//加载触发源配置的动态库
	string strAbsPath = PKComm::GetBinPath();
	strAbsPath = strAbsPath + "\\"+ m_szActionDllName ;
	int nRetVal = m_dllAction.open(strAbsPath.c_str());
	if(nRetVal != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("CActionTask:Load Action Dll(%s) failed!"), strAbsPath.c_str());//CActionTask:加载动作动态库(%s)失败!
		return -1;
	}
	g_logger.LogMessage(PK_LOGLEVEL_INFO, _("CActionTask:Load Action Dll(%s) succeed!"), strAbsPath.c_str());//CActionTask:加载动作动态库(%s)成功!

	m_pfnInitAction = (PFN_InitAction)m_dllAction.symbol(ACE_TEXT(PFNNAME_INITACTION));
	m_pfnExitAction = (PFN_ExitAction)m_dllAction.symbol(ACE_TEXT(PFNNAME_EXITACTION));
	m_pfnExecuteAction = (PFN_ExecuteAction)m_dllAction.symbol(ACE_TEXT(PFNNAME_EXECACTION));

	if(!m_pfnInitAction || !m_pfnExitAction || !m_pfnExecuteAction)
	{
		g_logger.LogErrMessage(-3, _("CActionTask:Get action dll(%s) poiter failed!"), strAbsPath.c_str());//CActionTask:获取动作动态库(%s)指针失败!
		return -3;
	}

	// 得到触发源对应的指针
	long lRet = m_pfnInitAction();
	if(lRet != PK_SUCCESS)
	{
		g_logger.LogErrMessage(lRet, _("Call action dll(%s) export function: InitAction failed"), strAbsPath.c_str());//调用动作动态库(%s)导出函数InitAction失败
	}
	else	
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, _("Call action dll(%s) export function: InitAction succeed"), strAbsPath.c_str());//调用动作动态库(%s)导出函数InitAction 成功
	}
	return lRet;
}

// 本任务停止时
void CActionTask::OnStop()
{
	if(m_pfnExitAction)
		m_pfnExitAction();	
	
	
}

// 向动作任务发送一个新的执行动作
// char *szActionId, char *szTagNameList, char *szTagValue, char *szHoldSeconds, char *szActionParam4, char *szPriority, char *szResult, int nResultBufLen
int CActionTask::PostAction(char *szActionId, char *szActionParam, char *szActionParam2, char *szActionParam3, char *szActionParam4, char *szPriority, char* szJsonEventParams)
{
	// 每个字符串包含长度（4个字节）+内容（N个字节）
	int nCtrlMsgLen = 4 * 8 + strlen(szActionId) + strlen(szActionParam) + strlen(szActionParam2) + strlen(szActionParam3) + strlen(szActionParam4) + strlen(szPriority) + strlen(szJsonEventParams) ;
	ACE_Message_Block *pMsg = new ACE_Message_Block(nCtrlMsgLen);

	int nCmdCode = 0;
	// 类型
	memcpy(pMsg->wr_ptr(), &nCmdCode, 4);
	pMsg->wr_ptr(4);

	// 字符串长度 szActionId
	int nStrLen = strlen(szActionId);
	memcpy(pMsg->wr_ptr(), &nStrLen, 4);
	pMsg->wr_ptr(4);
	// 字符串内容
	if(nStrLen > 0)
	{
		memcpy(pMsg->wr_ptr(), szActionId, nStrLen);
		pMsg->wr_ptr(nStrLen);
	}

	// 字符串长度 szActionParam
	nStrLen = strlen(szActionParam);
	memcpy(pMsg->wr_ptr(), &nStrLen, 4);
	pMsg->wr_ptr(4);
	// 字符串内容
	if(nStrLen > 0)
	{
		memcpy(pMsg->wr_ptr(), szActionParam, nStrLen);
		pMsg->wr_ptr(nStrLen);
	}

	// 字符串长度 szActionParam2
	nStrLen = strlen(szActionParam2);
	memcpy(pMsg->wr_ptr(), &nStrLen, 4);
	pMsg->wr_ptr(4);
	// 字符串内容
	if(nStrLen > 0)
	{
		memcpy(pMsg->wr_ptr(), szActionParam2, nStrLen);
		pMsg->wr_ptr(nStrLen);
	}

	// 字符串长度 szActionParam3
	nStrLen = strlen(szActionParam3);
	memcpy(pMsg->wr_ptr(), &nStrLen, 4);
	pMsg->wr_ptr(4);
	// 字符串内容
	if(nStrLen > 0)
	{
		memcpy(pMsg->wr_ptr(), szActionParam3, nStrLen);
		pMsg->wr_ptr(nStrLen);
	}

	// 字符串长度 szActionParam4
	nStrLen = strlen(szActionParam4);
	memcpy(pMsg->wr_ptr(), &nStrLen, 4);
	pMsg->wr_ptr(4);
	// 字符串内容
	if(nStrLen > 0)
	{
		memcpy(pMsg->wr_ptr(), szActionParam4, nStrLen);
		pMsg->wr_ptr(nStrLen);
    }

    // 字符串长度 szPriority
    nStrLen = strlen(szPriority);
    memcpy(pMsg->wr_ptr(), &nStrLen, 4);
    pMsg->wr_ptr(4);
    // 字符串内容
    if (nStrLen > 0)
    {
      memcpy(pMsg->wr_ptr(), szPriority, nStrLen);
      pMsg->wr_ptr(nStrLen);
    }

    // 字符串长度 szJsonEventParams
    nStrLen = strlen(szJsonEventParams);
    memcpy(pMsg->wr_ptr(), &nStrLen, 4);
    pMsg->wr_ptr(4);
    // 字符串内容
    if (nStrLen > 0)
    {
      memcpy(pMsg->wr_ptr(), szJsonEventParams, nStrLen);
      pMsg->wr_ptr(nStrLen);
    }

	putq(pMsg);
	//this->reactor()->notify(this, ACE_Event_Handler::WRITE_MASK); 
	return PK_SUCCESS;
}

void CActionTask::SetActionInfo(const char *szActionTypeName, const char* szDllName)
{
	PKStringHelper::Safe_StrNCpy(m_szActionTypeName, szActionTypeName, sizeof(m_szActionTypeName));
	PKStringHelper::Safe_StrNCpy(m_szActionDllName, szDllName, sizeof(m_szActionDllName));
}
