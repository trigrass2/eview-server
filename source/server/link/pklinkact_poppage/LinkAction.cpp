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
#include "pkcomm/pkcomm.h"
#include "pkdata/pkdata.h"
#include "json/json.h"
#include "common/eviewdefine.h"

#define PK_LINK_ACTIONNAME_OPENPAGE	"pklinkact_poppage"
#define ACTION_TAGDATA_MAXLEN		4096	// 动作的tag数据不会太长！
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
extern "C" ACE_Svc_Export int ExecuteAction(char *szActionId, char *szActionParam, char *szActionParam2, char *szActionParam3, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen)
{
  return g_pdbAction.ExecuteAction(szActionId, szActionParam, szActionParam2, szActionParam3, szActionParam4, nPriority, szJsonEventParams, szResult, nResultBufLen);
}


/**
 *  构造函数.
 *
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
CLinkAction::CLinkAction()
{
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

#include <fstream>  
#include <streambuf>  
#include <iostream>  

// 得到PKMemDB的端口号PKMemDB的密码
int GetPKMemDBPort(int &nListenPort, string &strPassword)
{
	nListenPort = PKMEMDB_LISTEN_PORT;
	strPassword = PKMEMDB_ACCESS_PASSWORD;

	string strConfigPath = PKComm::GetConfigPath();
	strConfigPath = strConfigPath + PK_OS_DIR_SEPARATOR + "pkmemdb.conf";
	std::ifstream textFile(strConfigPath.c_str());
	if (!textFile.is_open())  // 不存在该文件，则认为是sqlite：eview.db
	{
		// g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s not exist", strConfigPath.c_str());
		return -1;
	}

	// 计算文件大小  
	textFile.seekg(0, std::ios::end);
	std::streampos len = textFile.tellg();
	textFile.seekg(0, std::ios::beg);

	// 方法1  
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
 *  动作执行类初始化函数.
 *
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
int CLinkAction::InitAction()
{
	//设定动作处理函数的日志名称
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
	//返回
	return PK_SUCCESS;
}

/**
 *  动作处理函数. 不需要考虑优先级
 *
 *  @param  -[in]  szActionParam: [画面名称]
 *  @param  -[in]  szActionParam2: [画面参数]
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
int CLinkAction::ExecuteAction(char *szActionId, char *szPageName, char *szParamType, char *szParamValue, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen)
{
	Json::Value jsonAction;
	char szTmLink[ACTION_TAGDATA_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurTimeString(szTmLink,sizeof(szTmLink));
	memset(szResult, 0, nResultBufLen);
	g_logger.LogMessage(PK_LOGLEVEL_INFO,"CLinkAction::ExecuteAction 执行动作，ActionID：%s,ActionType:%s, PageName：%s, ParamType:%s, ParamValue:%s", 
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
		sprintf(szTmp, "弹出画面:%s,画面参数类型:%s, 参数值:%s, 成功", szPageName,szParamType, szParamValue);
	}
	else
		sprintf(szTmp, "弹出画面:%s,画面参数类型:%s, 参数值:%s, 失败,返回码:%d", szPageName,szParamType, szParamValue, nRet);
	
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
 *  资源释放函数.
 *
 *
 *  @version     08/22/2008  caichunlei  Initial Version.
 */
int CLinkAction::ExitAction()
{
	m_redisPublish.finalize();
	return 0; //RDA_Release();
}
