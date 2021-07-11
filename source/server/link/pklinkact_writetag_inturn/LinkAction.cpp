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
#include "common/eviewdefine.h"
#include "md5.h"
#include "json/json.h"

#define PK_LINK_ACTIONNAME			"writetag_inturn"
#define PK_LINK_ACTIONLOG_NAME		"pklinkact_writetag_inturn"

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

// 返回值表示结果，字符串结果存储在szResult中
// szTagNameList为逗号隔开的一组tag点名:tag1,tag2,tag3
// szTagValue: tag点的值，如cam1011
// szHoldSeconds，占有tag点的时间
extern "C" ACE_Svc_Export int ExecuteAction(char *szActionId, char *szTagNameList, char *szTagValue, char *szHoldSeconds, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen)
{
  return g_pdbAction.ExecuteAction(szActionId, szTagNameList, szTagValue, szHoldSeconds, szActionParam4, nPriority, szJsonEventParams, szResult, nResultBufLen);
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
	g_logger.SetLogFileName(PK_LINK_ACTIONLOG_NAME);
	int nPKMemDBListenPort = PKMEMDB_LISTEN_PORT;
	string strPKMemDBPassword = PKMEMDB_ACCESS_PASSWORD;
	GetPKMemDBPort(nPKMemDBListenPort, strPKMemDBPassword);

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
 * 动作处理函数. 不需要考虑优先级
 *
 * szTagNameList为逗号隔开的一组tag点名:tag1,tag2,tag3
 * szTagValue: tag点的值，如cam1011
 * szHoldSeconds，占有tag点的时间
 *
 */
int CLinkAction::ExecuteAction(char *szActionId, char *szTagNameList, char *szTagValue, char *szHoldSec, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen)
{
	memset(szResult, 0, nResultBufLen);
	LinkTagGroup *pLinkTagGroup = GetTagGroup(szTagNameList);
	if(!pLinkTagGroup)
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO,"cyclewritetag, 收到执行动作命令失败！根据tag名称列表未解析出tag组,tagname应以逗号隔开。actionID：%s,ActionType:%s, TagName：%s, TagValue:%s",  szActionId, PK_LINK_ACTIONNAME, szTagNameList, szTagValue);
		return -1;
	}

	time_t tmNow;
	time(&tmNow);

	LinkTag *pSelTag = NULL;
	// 第一轮，先找没有被占用的
	for(vector<LinkTag *>::iterator itTag = pLinkTagGroup->vecTags.begin(); itTag != pLinkTagGroup->vecTags.end(); itTag ++)
	{
		LinkTag *pTag = *itTag;
		if(!pTag->bHold)
		{
			pSelTag = pTag;
			break;
		}
	}
	// 第二轮，再找优先级低的。此时应该全部被占用了
	if(!pSelTag)
	{
		for(vector<LinkTag *>::iterator itTag = pLinkTagGroup->vecTags.begin(); itTag != pLinkTagGroup->vecTags.end(); itTag ++)
		{
			LinkTag *pTag = *itTag;
			if(pTag->nPriority < std::atoi(nPriority))
			{
				pSelTag = pTag;
				break;
			}
		}
	}

	// 第三轮，占用时间最长的。此时优先级应该相同的，且都被占用了
	if(!pSelTag)
	{
		time_t tmNow;
		time(&tmNow);
		for(vector<LinkTag *>::iterator itTag = pLinkTagGroup->vecTags.begin(); itTag != pLinkTagGroup->vecTags.end(); itTag ++)
		{
			LinkTag *pTag = *itTag;
			int nTimeSpan = labs(tmNow - pTag->tmHoldTime);
			if(pTag->nPriority == std::atoi(nPriority) && nTimeSpan > pTag->nHoldSeconds)
			{
				pSelTag = pTag;
				break;
			}
		}
	}

	// 第四轮，再优先级高，但占用超时的
	if(!pSelTag)
	{
		for(vector<LinkTag *>::iterator itTag = pLinkTagGroup->vecTags.begin(); itTag != pLinkTagGroup->vecTags.end(); itTag ++)
		{
			LinkTag *pTag = *itTag;
			int nTimeSpan = labs(tmNow - pTag->tmHoldTime);
			if(nTimeSpan > pTag->nHoldSeconds)
			{
				pSelTag = pTag;
				break;
			}
		}
	}

	char szTmp[1024] = {0};
	// 确实找不到可用的
	if(!pSelTag)
	{
		sprintf(szTmp, "组控变量,值:%s 失败,找不到可用的变量", szTagValue);
		g_logger.LogMessage(PK_LOGLEVEL_INFO,szTmp);
		PKStringHelper::Safe_StrNCpy(szResult, szTmp, nResultBufLen);
		return -2;
	}
	//设置为占用
	pSelTag->bHold = true;
	pSelTag->nHoldSeconds = ::atoi(szHoldSec);
	pSelTag->nPriority = std::atoi(nPriority);
	pSelTag->tmHoldTime = tmNow;

	int nRet = pkControl(m_hPKData, (char *)pSelTag->strTagName.c_str(), TAG_FIELD_VALUE, szTagValue);
	g_logger.LogMessage(PK_LOGLEVEL_INFO,"cyclewritetag. 收到执行动作命令执行控制返回值:%d。ActionID：%s,ActionType:%s, TagName：%s, TagValue:%s", nRet, szActionId, PK_LINK_ACTIONNAME, pSelTag->strTagName.c_str(), szTagValue);
	if(nRet == 0)
      PKStringHelper::Snprintf(szResult, nResultBufLen, "执行动作(动作id:%s) 组控变量,变量:%s, 值:%s, 成功", szActionId, pSelTag->strTagName.c_str(), szTagValue);
	else
      PKStringHelper::Snprintf(szResult, nResultBufLen, "执行动作(动作id:%s) 组控变量,变量:%s, 值:%s 失败,返回码:%d", szActionId, pSelTag->strTagName.c_str(), szTagValue, nRet);

	//tm *T_Now =  localtime(&tmNow);
	//char szTimeNow[128]={0};
	//sprintf(szTimeNow,"%d-%d-%d %d:%d:%d:", 1900 + T_Now->tm_year, 1 + T_Now->tm_mon, T_Now->tm_mday,T_Now->tm_hour,T_Now->tm_min,T_Now->tm_sec);

	//strcpy(szResult, szTimeNow);
	//strcat(szResult, szTmp);
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
//去掉空格;
void strallcut(char *str)
{
	int i,j=0;
	char sp[512];
	for (i = 0; *(str + i) != '\0'; i++) {
		if (*(str + i) == ' ' )
			continue;
		sp[j++]=*(str + i);
	}
	sp[j] = 0;
	strcpy(str, sp);
}
bool compareLinkTag(string left, string right)
{
	// 从小到大排序
	return left.compare(right) < 0;
}
// 从TagNameList拆分为各个tag点，再按照字母顺序排序，然后生成md5作为组的唯一标识
// 如果m_mapMd52TagGroup找到该md5则返回该组，否则m_mapMd52TagGroup中插入该组
LinkTagGroup * CLinkAction::GetTagGroup( char *szTagNameList )
{
	string strTagNameList = szTagNameList;
	vector<string> vecTagNamesWithEmpty = PKStringHelper::StriSplit(strTagNameList,",");
	vector<string> vecTagNames;
	for (vector<string>::iterator itTagName = vecTagNamesWithEmpty.begin(); itTagName != vecTagNamesWithEmpty.end(); itTagName++)
	{
		string strTagName = *itTagName;
		if (strTagName.empty())
			continue;
		vecTagNames.push_back(strTagName);
	}
	vecTagNamesWithEmpty.clear();
	
	std::sort(vecTagNames.begin(), vecTagNames.end(),compareLinkTag); 
	// 拼成一个字符串
	string strTagNameString = "";
	for(vector<string>::iterator itTagName = vecTagNames.begin();itTagName!=vecTagNames.end();itTagName++)
	{
		strTagNameString+=*itTagName;
	}
	//去空格
	//strallcut((char*)strTagNameString.data());
	// 计算字符串的md5码
	char* szMD5 = (char*)strTagNameString.c_str();
	MD5 md5(szMD5);
	md5.GenerateMD5((unsigned char*)strTagNameString.c_str(), strTagNameString.length());
	string strMD5 = md5.ToString();
	map<string, LinkTagGroup *>::iterator itMap = m_mapMd52TagGroup.find(strMD5);
	//查找到该id
	if(itMap != m_mapMd52TagGroup.end())
	{
		LinkTagGroup *pLinkTagGroup = itMap->second;
		return pLinkTagGroup;
	}
	LinkTagGroup *pLinkTagGroup = new LinkTagGroup();
	pLinkTagGroup->strTagsMd5 = strMD5;
	// 将vecTagNames中每个逐个加入
	for(vector<string>::iterator itTagName = vecTagNames.begin();itTagName!=vecTagNames.end();itTagName++)
	{
		LinkTag *vec_tagTmp=new LinkTag ;
		vec_tagTmp->strTagName = *itTagName;
		vec_tagTmp->bHold = false;
		vec_tagTmp->nPriority = 0;
		vec_tagTmp->tmHoldTime = 0;
		vec_tagTmp->nHoldSeconds = 0;
		(pLinkTagGroup->vecTags).push_back(vec_tagTmp);
	}
	m_mapMd52TagGroup[pLinkTagGroup->strTagsMd5] = pLinkTagGroup;
	return pLinkTagGroup;
}
