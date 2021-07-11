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

// ����ֵ��ʾ������ַ�������洢��szResult��
// szTagNameListΪ���Ÿ�����һ��tag����:tag1,tag2,tag3
// szTagValue: tag���ֵ����cam1011
// szHoldSeconds��ռ��tag���ʱ��
extern "C" ACE_Svc_Export int ExecuteAction(char *szActionId, char *szTagNameList, char *szTagValue, char *szHoldSeconds, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen)
{
  return g_pdbAction.ExecuteAction(szActionId, szTagNameList, szTagValue, szHoldSeconds, szActionParam4, nPriority, szJsonEventParams, szResult, nResultBufLen);
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
	g_logger.SetLogFileName(PK_LINK_ACTIONLOG_NAME);
	int nPKMemDBListenPort = PKMEMDB_LISTEN_PORT;
	string strPKMemDBPassword = PKMEMDB_ACCESS_PASSWORD;
	GetPKMemDBPort(nPKMemDBListenPort, strPKMemDBPassword);

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
 * ����������. ����Ҫ�������ȼ�
 *
 * szTagNameListΪ���Ÿ�����һ��tag����:tag1,tag2,tag3
 * szTagValue: tag���ֵ����cam1011
 * szHoldSeconds��ռ��tag���ʱ��
 *
 */
int CLinkAction::ExecuteAction(char *szActionId, char *szTagNameList, char *szTagValue, char *szHoldSec, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen)
{
	memset(szResult, 0, nResultBufLen);
	LinkTagGroup *pLinkTagGroup = GetTagGroup(szTagNameList);
	if(!pLinkTagGroup)
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO,"cyclewritetag, �յ�ִ�ж�������ʧ�ܣ�����tag�����б�δ������tag��,tagnameӦ�Զ��Ÿ�����actionID��%s,ActionType:%s, TagName��%s, TagValue:%s",  szActionId, PK_LINK_ACTIONNAME, szTagNameList, szTagValue);
		return -1;
	}

	time_t tmNow;
	time(&tmNow);

	LinkTag *pSelTag = NULL;
	// ��һ�֣�����û�б�ռ�õ�
	for(vector<LinkTag *>::iterator itTag = pLinkTagGroup->vecTags.begin(); itTag != pLinkTagGroup->vecTags.end(); itTag ++)
	{
		LinkTag *pTag = *itTag;
		if(!pTag->bHold)
		{
			pSelTag = pTag;
			break;
		}
	}
	// �ڶ��֣��������ȼ��͵ġ���ʱӦ��ȫ����ռ����
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

	// �����֣�ռ��ʱ����ġ���ʱ���ȼ�Ӧ����ͬ�ģ��Ҷ���ռ����
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

	// �����֣������ȼ��ߣ���ռ�ó�ʱ��
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
	// ȷʵ�Ҳ������õ�
	if(!pSelTag)
	{
		sprintf(szTmp, "��ر���,ֵ:%s ʧ��,�Ҳ������õı���", szTagValue);
		g_logger.LogMessage(PK_LOGLEVEL_INFO,szTmp);
		PKStringHelper::Safe_StrNCpy(szResult, szTmp, nResultBufLen);
		return -2;
	}
	//����Ϊռ��
	pSelTag->bHold = true;
	pSelTag->nHoldSeconds = ::atoi(szHoldSec);
	pSelTag->nPriority = std::atoi(nPriority);
	pSelTag->tmHoldTime = tmNow;

	int nRet = pkControl(m_hPKData, (char *)pSelTag->strTagName.c_str(), TAG_FIELD_VALUE, szTagValue);
	g_logger.LogMessage(PK_LOGLEVEL_INFO,"cyclewritetag. �յ�ִ�ж�������ִ�п��Ʒ���ֵ:%d��ActionID��%s,ActionType:%s, TagName��%s, TagValue:%s", nRet, szActionId, PK_LINK_ACTIONNAME, pSelTag->strTagName.c_str(), szTagValue);
	if(nRet == 0)
      PKStringHelper::Snprintf(szResult, nResultBufLen, "ִ�ж���(����id:%s) ��ر���,����:%s, ֵ:%s, �ɹ�", szActionId, pSelTag->strTagName.c_str(), szTagValue);
	else
      PKStringHelper::Snprintf(szResult, nResultBufLen, "ִ�ж���(����id:%s) ��ر���,����:%s, ֵ:%s ʧ��,������:%d", szActionId, pSelTag->strTagName.c_str(), szTagValue, nRet);

	//tm *T_Now =  localtime(&tmNow);
	//char szTimeNow[128]={0};
	//sprintf(szTimeNow,"%d-%d-%d %d:%d:%d:", 1900 + T_Now->tm_year, 1 + T_Now->tm_mon, T_Now->tm_mday,T_Now->tm_hour,T_Now->tm_min,T_Now->tm_sec);

	//strcpy(szResult, szTimeNow);
	//strcat(szResult, szTmp);
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
//ȥ���ո�;
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
	// ��С��������
	return left.compare(right) < 0;
}
// ��TagNameList���Ϊ����tag�㣬�ٰ�����ĸ˳������Ȼ������md5��Ϊ���Ψһ��ʶ
// ���m_mapMd52TagGroup�ҵ���md5�򷵻ظ��飬����m_mapMd52TagGroup�в������
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
	// ƴ��һ���ַ���
	string strTagNameString = "";
	for(vector<string>::iterator itTagName = vecTagNames.begin();itTagName!=vecTagNames.end();itTagName++)
	{
		strTagNameString+=*itTagName;
	}
	//ȥ�ո�
	//strallcut((char*)strTagNameString.data());
	// �����ַ�����md5��
	char* szMD5 = (char*)strTagNameString.c_str();
	MD5 md5(szMD5);
	md5.GenerateMD5((unsigned char*)strTagNameString.c_str(), strTagNameString.length());
	string strMD5 = md5.ToString();
	map<string, LinkTagGroup *>::iterator itMap = m_mapMd52TagGroup.find(strMD5);
	//���ҵ���id
	if(itMap != m_mapMd52TagGroup.end())
	{
		LinkTagGroup *pLinkTagGroup = itMap->second;
		return pLinkTagGroup;
	}
	LinkTagGroup *pLinkTagGroup = new LinkTagGroup();
	pLinkTagGroup->strTagsMd5 = strMD5;
	// ��vecTagNames��ÿ���������
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
