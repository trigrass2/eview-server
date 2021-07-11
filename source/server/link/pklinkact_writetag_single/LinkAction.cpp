/**************************************************************
 *  Filename:    PDBAction.cxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �������ݿ���ģ��.
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

// ����ֵ��ʾ������ַ��ܽ���洢��szResult��
extern "C" ACE_Svc_Export int ExecuteAction(char *szActionId, char *szTagName, char *szTagValue, char *szActionParam3, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen)
{
  return g_pdbAction.ExecuteAction(szActionId, szTagName, szTagValue, szActionParam3, szActionParam4, nPriority, szJsonEventParams, szResult, nResultBufLen);
}


/**
 *  ���캯��.
 *
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
CLinkAction::CLinkAction()
{
	m_hPKData = NULL;
}

/**
 *  ��������.
 *
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
CLinkAction::~CLinkAction()
{

}

/**
 *  ����ִ�����ʼ������.
 *
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
int CLinkAction::InitAction()
{
	//�趨��������������־����
	g_logger.SetLogFileName(PK_LINK_ACTIONLOG_NAME);
	
	m_hPKData = pkInit(PK_LOCALHOST_IP, "����ϵͳ", "");
	if(m_hPKData == NULL)
	{
		int nRet = -1;
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("PKData.pkInit initialize Failed, RetCode: %d"), nRet);
	}
	//����
	return PK_SUCCESS;
}

/**
 *  ����������. ����Ҫ�������ȼ�
 *
 *  @param  -[in]  szTagName: [Tag��������]
 *  @param  -[in]  szTagValue: [����ֵ]
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
int CLinkAction::ExecuteAction(char *szActionId, char *szTagName, char *szTagValue, char *szActionParam3, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen)
{
	memset(szResult, 0, nResultBufLen);
	int nRet = pkControl(m_hPKData, szTagName, TAG_FIELD_VALUE, szTagValue);
	g_logger.LogMessage(PK_LOGLEVEL_INFO,"CLinkAction::ExecuteAction �յ�ִ�ж������ActionID��%s,ActionType:%s, TagName��%s, TagValue:%s",  szActionId, PK_LINK_ACTIONNAME, szTagName, szTagValue);

	char szTmp[1024] = {0};
	if(nRet == 0)
      PKStringHelper::Snprintf(szResult, nResultBufLen, "ִ�ж��� %s,д�����:%s,ֵ:%s, �ɹ�", szActionId, szTagName, szTagValue);
	else
      PKStringHelper::Snprintf(szResult, nResultBufLen, "ִ�ж��� %s,д�����:%s,ֵ:%s ʧ��,������:%d", szActionId, szTagName, szTagValue, nRet);
	
	
	//���ز���ʱ��
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
 *  ��Դ�ͷź���.
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
