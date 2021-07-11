/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  shijunpu  Initial Version
 *  @version	9/17/2012  shijunpu  ���ݿ������޸�Ϊ�ַ������ͣ������޸��������ݿ�ʱ�Ľӿ�
 *  @version	3/1/2013   shijunpu  �����ɵ����ɿ���߼������豸��û���������ݿ�ʱ��ֱ���ɵ��������ݿ� .
**************************************************************/

#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"

#include "math.h"
#include "pkcomm/pkcomm.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/eviewdefine.h"
#include "pklog/pklog.h"
#include <sstream>
#include "pkcomm/pkcomm.h" 
#include "RedisFieldDefine.h"
#include "DataProcTask.h"
#include "MainTask.h"

extern CPKLog g_logger;
extern CProcessQueue * g_pQueData2NodeSrv;

int TagValFromBin2String(char *szTagName, int nTagDataType,const char *szInBinData, int nBinDataBufLen, string &strTagValue);

#define	DEFAULT_SLEEP_TIME		100	

/**
 *  ���캯��.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CDataProcTask::CDataProcTask()
{
	m_bStop = false;
	m_bStopped = false;

	m_nPackNumRecvRecent = 0;
	m_nPackNumRecvFromStart = 0;

	m_tvRecentRecvPack = m_tvFirstRecvPack = ACE_OS::gettimeofday();	// �ϴδ�ӡ
	m_nTimeoutPackNumFromStart = 0;
	m_nTimeoutPackNumRecent = 0;
	m_nTimeoutMaxSecFromStart = 0;
	m_nTimeoutMaxSecRecent = 0;

	m_nRecvTagCountRecent = 0;	// һ��ʱ�����յ�����������������tag��
	m_nValidTagCountRecent = 0;	// һ��ʱ�����յ�����������������Чtag��
	m_strMinTagTimestampRecent = "";	// ���һ��ʱ�����Сʱ���
	m_strMaxTagTimestampRecent = ""; // ���һ��ʱ������ʱ���
	m_nCommitTagNumRecent = 0;		// ���һ��ʱ���ύ��tag����
	m_nCommitTimesRecent = 0;		// ���һ��ʱ���ύ�Ĵ���
	m_strCommitDetail = "";			// ÿ���ύ������
	m_tvLastPrint = ACE_Time_Value::zero;
}

/**
 *  ��������.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CDataProcTask::~CDataProcTask()
{
	
}

void CDataProcTask::PrintDataPackStatInfo(ACE_Time_Value &tvNow, ACE_Time_Value &tvPack)
{
	m_nPackNumRecvFromStart ++;
	m_nPackNumRecvRecent ++;

	// ������û�г�ʱ
	ACE_Time_Value tvPackSpan = tvNow - tvPack;
	if(labs(tvPackSpan.sec()) > 2) // ���ͺͽ���ʱ�������2�룡
	{
		m_nTimeoutPackNumFromStart ++;
		m_nTimeoutPackNumRecent ++;
		if (labs(tvPackSpan.sec()) > m_nTimeoutMaxSecRecent)
			m_nTimeoutMaxSecRecent = labs(tvPackSpan.sec());
		if (labs(tvPackSpan.sec()) > m_nTimeoutMaxSecFromStart)
			m_nTimeoutMaxSecFromStart = labs(tvPackSpan.sec());
	}

	ACE_Time_Value tvSpanRecentRecv = tvNow - m_tvRecentRecvPack;
	int nSecSpan = labs(tvSpanRecentRecv.sec());
	if(nSecSpan > 30) // ��ʱ����Ҫ��ӡ��Ϣ��
	{
#ifdef _WIN32
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�����������յ�:%d��, ����tag��:%d��, ��Чtag��:%d��",
			m_nPackNumRecvFromStart, m_nRecvTagCountRecent, m_nRecvTagCountRecent - m_nValidTagCountRecent);			// ÿ���ύ������);

        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "���:%d��: ���� %d ����,ƽ�� %f����/��. �ύ����:%d, tag����:%d��,ƽ��ÿ��%.f��, tag��Сʱ��:%s, ���ʱ��:%s. ",
			nSecSpan, m_nPackNumRecvRecent, m_nPackNumRecvRecent * 1.0/nSecSpan, m_nCommitTimesRecent, m_nCommitTagNumRecent, m_nCommitTagNumRecent*1.0/nSecSpan,
			 m_strMinTagTimestampRecent.c_str(), m_strMaxTagTimestampRecent.c_str());	
#else
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "recv:%d packets from start, total tagnum:%d, invalid tagnum:%d",
            m_nPackNumRecvFromStart, m_nRecvTagCountRecent, m_nRecvTagCountRecent - m_nValidTagCountRecent);			// ÿ���ύ������);

        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "recent:%d seconds: recv %d packets,avg %f packets/second. commit count:%d, tagnum:%d,avg %.f tagnum/second, tag mintime:%s, maxtime:%s. ",
            nSecSpan, m_nPackNumRecvRecent, m_nPackNumRecvRecent * 1.0/nSecSpan, m_nCommitTimesRecent, m_nCommitTagNumRecent, m_nCommitTagNumRecent*1.0/nSecSpan,
             m_strMinTagTimestampRecent.c_str(), m_strMaxTagTimestampRecent.c_str());
#endif
		//g_logger.LogMessage(PK_LOGLEVEL_INFO, "���:%d��: �ύ��ϸ:%s", nSecSpan, m_strCommitDetail.c_str());	

		if(m_nTimeoutPackNumRecent > 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "!!����(performance)����: ���%d���ڣ����պͷ��ͼ������>2��(�������%d�룩��%d����. ������������, ����2��(�������%d�룩�� %d ����",
				nSecSpan, m_nTimeoutMaxSecRecent, m_nTimeoutPackNumRecent, m_nTimeoutMaxSecFromStart, m_nTimeoutPackNumFromStart);
        }
		m_nPackNumRecvRecent = 0;
		m_nTimeoutPackNumRecent = 0;
		m_nTimeoutMaxSecRecent = 0;
		m_tvRecentRecvPack = tvNow;

		m_nRecvTagCountRecent = 0;
		m_nValidTagCountRecent = 0;

		m_nCommitTagNumRecent = 0;
		m_strMinTagTimestampRecent = "";
		m_strMaxTagTimestampRecent = "";

		m_strCommitDetail = "";
		m_nCommitTimesRecent = 0;	
	}
}

/**
 *  �麯�����̳���ACE_Task.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
int CDataProcTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();
	int nRet = 0;
	nRet = OnStart();

	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	while(!m_bStop)
	{
		PKComm::Sleep(50);
		ACE_Time_Value tvNow = ACE_OS::gettimeofday();
 		CheckAndCommitTagData(tvNow);
		CheckAndPrintLowerNodeStatus(tvNow);
	} // while(!m_bStop)
		
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"supernodeserver DataProcTask exit normally!");
	OnStop();

	m_bStopped = true;
	return PK_SUCCESS;	
}

vector<string> CDataProcTask::split(string str,string pattern)
{
	string::size_type pos;  
	vector<string> result;  
	str+=pattern;     //��չ�ַ����Է������  
	int size=str.size();  

	for(int i=0; i<size; i++)  
	{  
		pos=str.find(pattern,i);  
		if(pos<size)  
		{  
			std::string s=str.substr(i,pos-i);  
			result.push_back(s);  
			i=pos+pattern.size()-1;  
		}  
	}  
	return result;  
}

// �̳߳�ʼ��
int CDataProcTask::OnStart()
{
	return 0;
}

// ������ֹͣʱ
void CDataProcTask::OnStop()
{
}

int CDataProcTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CDataProcTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}

// ������������tag������Ϊ����ֹͣ��BAD�������ر��ˣ�
int CDataProcTask::UpdateTagsOfNodeToBad(CNodeInfo *pNode)
{
	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "UpdateTagsOfNodeToBad,driver:%s, has devicenum:%d", pNode->m_strName.c_str(), pNode->m_vecTags.size());

	m_mutexTagDataCache.acquire(); // ������ֹm_listTagDataCachedͬʱ����д
	for (vector<CNodeTag *>::iterator itTag = pNode->m_vecTags.begin(); itTag != pNode->m_vecTags.end(); itTag++)
	{
		CNodeTag *pTag = *itTag;
		string strTagValue = "";
			// ��ַΪconnstatus�ı�����ʾ������״̬������������ڲ����������ܹ�����Ϊ***�ţ�
		// �����ֵ��Ϊ�գ�Ҳ���ܸı�Ϊ**** 

		if (pTag->m_strAddress.compare("connstatus") == 0) // ����tag��
		{
			strTagValue = "0"; // δ����״̬
		} 
		else
		{
			strTagValue = TAG_QUALITY_UNKNOWN_REASON_STRING;  // δ����״̬
		}

		string strJsonValue; 
 		Json::Value jsonOneTag;
		jsonOneTag[JSONFIELD_TIME] = szTime;
		jsonOneTag[JSONFIELD_QUALITY] = TAG_QUALITY_UNKNOWN_REASON;
 		jsonOneTag[JSONFIELD_VALUE] = strTagValue;
		strJsonValue = m_jsonWriter.write(jsonOneTag);

		TAGDATA_JSONSTR tagDataJson;
		tagDataJson.strName = pTag->m_strName;
		tagDataJson.strVTQ = strJsonValue;
		m_vecTagDataCached.push_back(tagDataJson); 
	}
	m_mutexTagDataCache.release(); 

	return 0;
}

void CDataProcTask::CheckAndCommitTagData(ACE_Time_Value tvNow)
{
	ACE_Time_Value tvSpan = tvNow - m_tvLastCommitTagData;
	ACE_UINT64 nSpanAbsMS = tvSpan.get_msec();
	if (nSpanAbsMS < 50)
		return;

	m_tvLastCommitTagData = tvNow;

	int nCacheDataCount = m_vecTagDataCached.size();
	if(nCacheDataCount <= 0)
		return;

	m_nCommitTimesRecent ++;
	char szTmp[32] = {0};
	sprintf(szTmp, "%d", nCacheDataCount);
	if(m_strCommitDetail.length() == 0)
		m_strCommitDetail = szTmp;
	else
		m_strCommitDetail = m_strCommitDetail + "," + szTmp;

	// 
    int nCommitSuccessNum = 0;
    int nCommitFailNum = 0;
	vector<string> vecKeys;
	vector<string> vecValues;
	m_mutexTagDataCache.acquire(); // ������ֹm_listTagDataCachedͬʱ����д
	list<TAGDATA_JSONSTR>::iterator itTag = m_vecTagDataCached.begin();
	for(; itTag != m_vecTagDataCached.end(); itTag ++)
	{
		TAGDATA_JSONSTR & tagDataJson = *itTag;
		string strTagName = tagDataJson.strName;
		string strTagValues = tagDataJson.strVTQ;

		int nRet = MAIN_TASK->m_redisRW0.set(strTagName.c_str(), strTagValues.c_str());
        if(nRet == 0)
            nCommitSuccessNum ++;
        else
            nCommitFailNum ++;

		// ������Ϣ
		m_nCommitTagNumRecent ++;
		if(m_strMinTagTimestampRecent.empty())
			m_strMinTagTimestampRecent = tagDataJson.strTime4Debug;
		else if(m_strMinTagTimestampRecent.compare(tagDataJson.strTime4Debug) > 0)
			m_strMinTagTimestampRecent = tagDataJson.strTime4Debug;

		if(m_strMaxTagTimestampRecent.empty())
			m_strMaxTagTimestampRecent = tagDataJson.strTime4Debug;
		else if(m_strMaxTagTimestampRecent.compare(tagDataJson.strTime4Debug) < 0)
			m_strMaxTagTimestampRecent = tagDataJson.strTime4Debug;
	}
	m_mutexTagDataCache.release();

	MAIN_TASK->m_redisRW0.commit(); // �ύ��
    //g_logger.LogMessage(PK_LOGLEVEL_INFO, "commit %d tags, success:%d, fail:%d", nCommitSuccessNum + nCommitFailNum, nCommitSuccessNum, nCommitFailNum);

	if(vecValues.size() > 0)
	{
		//int nRet = m_redisRW.mset(vecKeys, vecValues);
	}
	vecKeys.clear();
	vecValues.clear();

	m_mutexTagDataCache.acquire();
	m_vecTagDataCached.clear();
	m_mutexTagDataCache.release();
}


int CDataProcTask::CheckAndPrintLowerNodeStatus(ACE_Time_Value tvNow)
{
	ACE_Time_Value tvSpan = tvNow - m_tvLastPrint;
	ACE_UINT64 nSpanAbsMS = tvSpan.get_msec();
	if (nSpanAbsMS < 180000)
		return -1;

	m_tvLastPrint = tvNow;
	map<string, CNodeInfo *> &mapId2Node = MAIN_TASK->m_mapId2Node;
	if (mapId2Node.size() == 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "δ�����¼��ڵ�!");
		return 0;
	}

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "�ϼ��������õ�%d���¼��ڵ�, ״̬����:", mapId2Node.size());
	for (map<string, CNodeInfo *>::iterator itNode = mapId2Node.begin(); itNode != mapId2Node.end(); itNode++)
	{
		CNodeInfo * pLowerNode = itNode->second;

		ACE_Time_Value tvSpanAlive = tvNow - pLowerNode->m_tvLastData;
		ACE_UINT64 nSpanAliveMS = tvSpanAlive.get_msec();
		if (nSpanAliveMS < 180000)
			pLowerNode->m_bAlive = true;
		else
		{
			if (pLowerNode->m_bAlive)
			{
				this->UpdateTagsOfNodeToBad(pLowerNode); // �ýڵ�����е����������ΪBAD
			}
			pLowerNode->m_bAlive = false;
		}

		char szDateTime[32] = { 0 };
		PKTimeHelper::HighResTime2String(szDateTime, sizeof(szDateTime), pLowerNode->m_tvLastData.sec(), 0);
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�¼��ڵ�,id:%s,code:%s,name:%s,status:%s,last data time:%s, ����������:%d��",
			pLowerNode->m_strId.c_str(), pLowerNode->m_strCode.c_str(), pLowerNode->m_strName.c_str(), pLowerNode->m_bAlive ? "alive" : "dead", szDateTime, pLowerNode->m_vecTags.size());
	}
	return 0;
}
