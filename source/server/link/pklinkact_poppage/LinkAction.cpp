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
#include "pkcomm/pkcomm.h"
#include "pkdata/pkdata.h"
#include "json/json.h"
#include "common/eviewdefine.h"

#define PK_LINK_ACTIONNAME_OPENPAGE	"pklinkact_poppage"
#define ACTION_TAGDATA_MAXLEN		4096	// ������tag���ݲ���̫����
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
extern "C" ACE_Svc_Export int ExecuteAction(char *szActionId, char *szActionParam, char *szActionParam2, char *szActionParam3, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen)
{
  return g_pdbAction.ExecuteAction(szActionId, szActionParam, szActionParam2, szActionParam3, szActionParam4, nPriority, szJsonEventParams, szResult, nResultBufLen);
}


/**
 *  ���캯��.
 *
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
CLinkAction::CLinkAction()
{
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

#include <fstream>  
#include <streambuf>  
#include <iostream>  

// �õ�PKMemDB�Ķ˿ں�PKMemDB������
int GetPKMemDBPort(int &nListenPort, string &strPassword)
{
	nListenPort = PKMEMDB_LISTEN_PORT;
	strPassword = PKMEMDB_ACCESS_PASSWORD;

	string strConfigPath = PKComm::GetConfigPath();
	strConfigPath = strConfigPath + PK_OS_DIR_SEPARATOR + "pkmemdb.conf";
	std::ifstream textFile(strConfigPath.c_str());
	if (!textFile.is_open())  // �����ڸ��ļ�������Ϊ��sqlite��eview.db
	{
		// g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s not exist", strConfigPath.c_str());
		return -1;
	}

	// �����ļ���С  
	textFile.seekg(0, std::ios::end);
	std::streampos len = textFile.tellg();
	textFile.seekg(0, std::ios::beg);

	// ����1  
	std::string strConfigData((std::istreambuf_iterator<char>(textFile)), std::istreambuf_iterator<char>());
	textFile.close();
	vector<string> vecLine = PKStringHelper::StriSplit(strConfigData, "\n");
	for (int iLine = 0; iLine < vecLine.size(); iLine++)
	{
		string strLine = vecLine[iLine];
		vector<string> vecContent = PKStringHelper::StriSplit(strLine, " ");
		if (vecContent.size() < 2)
			continue;
		string strKey = vecContent[0];
		string strValue = vecContent[1];
		if (strKey.compare("port") == 0)
			nListenPort = ::atoi(strValue.c_str());
		if (strKey.compare("requirepass") == 0)
			strPassword = strValue.c_str();
	}
	return 0;
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
	g_logger.SetLogFileName(PK_LINK_ACTIONNAME_OPENPAGE);
	int nPKMemDBListenPort = PKMEMDB_LISTEN_PORT;
	string strPKMemDBPassword = PKMEMDB_ACCESS_PASSWORD;
	GetPKMemDBPort(nPKMemDBListenPort, strPKMemDBPassword);

	int nRet = m_redisPublish.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str(), 0);
	if(nRet != PK_SUCCESS)
	{
		nRet = -1;
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("CPDBAction::InitAction redis initialize Failed, RetCode: %d"), nRet);
	}
	//����
	return PK_SUCCESS;
}

/**
 *  ����������. ����Ҫ�������ȼ�
 *
 *  @param  -[in]  szActionParam: [��������]
 *  @param  -[in]  szActionParam2: [�������]
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
int CLinkAction::ExecuteAction(char *szActionId, char *szPageName, char *szParamType, char *szParamValue, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen)
{
	Json::Value jsonAction;
	char szTmLink[ACTION_TAGDATA_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurTimeString(szTmLink,sizeof(szTmLink));
	memset(szResult, 0, nResultBufLen);
	g_logger.LogMessage(PK_LOGLEVEL_INFO,"CLinkAction::ExecuteAction ִ�ж�����ActionID��%s,ActionType:%s, PageName��%s, ParamType:%s, ParamValue:%s", 
		szActionId,"poppage", szPageName,szParamType, szParamValue);

	char szTmp[1024] = {0};
	jsonAction["action"] = "poppage";
	jsonAction["pagename"] = szPageName;
	jsonAction["paramtype"] = szParamType;
	jsonAction["paramvalue"] = szParamValue;
	string  strJsonAction = jsonAction.toStyledString();
	int nRet = m_redisPublish.publish(CHANNELNAME_LINK_ACTION2CLIENT, strJsonAction.c_str());
	if(nRet == 0)
	{
		sprintf(szTmp, "��������:%s,�����������:%s, ����ֵ:%s, �ɹ�", szPageName,szParamType, szParamValue);
	}
	else
		sprintf(szTmp, "��������:%s,�����������:%s, ����ֵ:%s, ʧ��,������:%d", szPageName,szParamType, szParamValue, nRet);
	
	time_t tmNow;
	time(&tmNow);
	tm *T_Now =  localtime(&tmNow);
	char szTimeNow[128]={0};
	sprintf(szTimeNow,"%d-%d-%d %d:%d:%d", 1900 + T_Now->tm_year, 1 + T_Now->tm_mon, T_Now->tm_mday,T_Now->tm_hour,T_Now->tm_min,T_Now->tm_sec);


	PKStringHelper::Safe_StrNCpy(szResult, szTmp, nResultBufLen);
	strcat(szResult,";");
	strcat(szResult,szTimeNow);
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
	m_redisPublish.finalize();
	return 0; //RDA_Release();
}
