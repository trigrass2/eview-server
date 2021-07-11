/**************************************************************
 *  Filename:    PDBAction.cxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 过程数据控制模块.
 *
 *  @author:     caichunlei
 *  @version     08/22/2008  caichunlei  Initial Version
**************************************************************/
#define ACE_BUILD_SVC_DLL
#include "ace/svc_export.h"
#include "pklog/pklog.h"
#include "LinkAction.hxx"
#include <ace/OS_NS_string.h>
#include "common/eviewdefine.h"
#include "pkcomm/pkcomm.h"

#include "json/json.h"

#define PK_LINK_ACTIONNAME			"writetag_single"
#define PK_LINK_ACTIONLOG_NAME		"pklinkact_writetag_single"

CLinkAction	 g_pdbAction;
CPKLog g_logger;

extern "C" ACE_Svc_Export int InitAction()
{
	return g_pdbAction.InitAction();
}

extern "C" ACE_Svc_Export int ExitAction()
{
	g_pdbAction.ExitAction();
	return PK_SUCCESS;
}

// 返回值表示结果，字符窜结果存储在szResult中
extern "C" ACE_Svc_Export int ExecuteAction(char *szActionId, char *szTagName, char *szTagValue, char *szActionParam3, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen)
{
  return g_pdbAction.ExecuteAction(szActionId, szTagName, szTagValue, szActionParam3, szActionParam4, nPriority, szJsonEventParams, szResult, nResultBufLen);
}


/**
 *  构造函数.
 *
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
CLinkAction::CLinkAction()
{
	m_hPKData = NULL;
}

/**
 *  析构函数.
 *
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
CLinkAction::~CLinkAction()
{

}

/**
 *  动作执行类初始化函数.
 *
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
int CLinkAction::InitAction()
{
	//设定动作处理函数的日志名称
	g_logger.SetLogFileName(PK_LINK_ACTIONLOG_NAME);
	
	m_hPKData = pkInit(PK_LOCALHOST_IP, "联动系统", "");
	if(m_hPKData == NULL)
	{
		int nRet = -1;
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("PKData.pkInit initialize Failed, RetCode: %d"), nRet);
	}
	//返回
	return PK_SUCCESS;
}

/**
 *  动作处理函数. 不需要考虑优先级
 *
 *  @param  -[in]  szTagName: [Tag变量名称]
 *  @param  -[in]  szTagValue: [变量值]
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
int CLinkAction::ExecuteAction(char *szActionId, char *szTagName, char *szTagValue, char *szActionParam3, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen)
{
	memset(szResult, 0, nResultBufLen);
	int nRet = pkControl(m_hPKData, szTagName, TAG_FIELD_VALUE, szTagValue);
	g_logger.LogMessage(PK_LOGLEVEL_INFO,"CLinkAction::ExecuteAction 收到执行动作命令，ActionID：%s,ActionType:%s, TagName：%s, TagValue:%s",  szActionId, PK_LINK_ACTIONNAME, szTagName, szTagValue);

	char szTmp[1024] = {0};
	if(nRet == 0)
      PKStringHelper::Snprintf(szResult, nResultBufLen, "执行动作 %s,写入变量:%s,值:%s, 成功", szActionId, szTagName, szTagValue);
	else
      PKStringHelper::Snprintf(szResult, nResultBufLen, "执行动作 %s,写入变量:%s,值:%s 失败,返回码:%d", szActionId, szTagName, szTagValue, nRet);
	
	
	//返回操作时间
	//time_t tmNow;
	//time(&tmNow);
	//tm *T_Now =  localtime(&tmNow);
	//char szTimeNow[128]={0};
	//sprintf(szTimeNow,"%d-%d-%d %d:%d:%d", 1900 + T_Now->tm_year, 1 + T_Now->tm_mon, T_Now->tm_mday,T_Now->tm_hour,T_Now->tm_min,T_Now->tm_sec);

	//strncpy(szResult, szTmp, nResultBufLen - 1);
	//strcat(szResult,";");
	//strcat(szResult,szTimeNow);
	return nRet;
}

/**
 *  资源释放函数.
 *
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
int CLinkAction::ExitAction()
{
	if(m_hPKData != NULL)
		pkExit(m_hPKData);
	m_hPKData = NULL;
	return 0; //RDA_Release();
}
