/**************************************************************
*  Filename:    MainTask.cpp
*  Copyright:   Shanghai Peak Software Co., Ltd.
*
*  Description: ���ؿͻ��˷�������;
*
*  @author:    ycd;
*  @version    1.0.0
**************************************************************/
#include "ace/OS.h"
#include <ace/Default_Constants.h>
#include "map"

#include "math.h"
#ifdef _WIN32
#include "windows.h"
#else
#include "unistd.h"
#include "stdlib.h"
#include <iconv.h>

int Utf8ToGbk(char *src_str, size_t src_len, char *dst_str, size_t dst_len)
{
    iconv_t cd;
    char **pin = &src_str;
    char **pout = &dst_str;

    cd = iconv_open("gbk", "utf8");
    if (cd == 0)
        return -1;
    memset(dst_str, 0, dst_len);
    if (iconv(cd, pin, &src_len, pout, &dst_len) == -1)
        return -1;
    iconv_close(cd);
    *pout = '\0';

    return dst_len;
}

#endif

#include "pkcomm/pkcomm.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"
#include "pklog/pklog.h"
#include "vector"
#include "pkdbapi/pkdbapi.h"
#include "MainTask.h"
#include "mosquitto/mosquitto.h"
#include "eviewcomm/eviewcomm.h"
#include "md5.h"
using namespace std;

extern CPKLog g_logger;

string g_strThisNodeId = ""; // ���ڵ��ΨһID��ȡ��һ��������mac��ַ��

#define SPLIT_GW_MSG_CHAR	"@"

#define  CHECK_NULL_RETURN_ERR(X)  { if(!X) return EC_ICV_DA_CONFIG_FILE_STRUCTURE_ERROR; }
#define	DEFAULT_SLEEP_TIME			100	
#define NULLASSTRING(x) x==NULL?"":x
 
#define	DEFAULT_SLEEP_TIME		100	
#define SQLBUFFERSIZE			1024

static int message_count = 0;
int GetErrMsg(int nErrNum, char* szMsg)
{
	string strMsg = szMsg;
	switch (nErrNum) {
	case MQTT_SUCCESS:
		strMsg += "success";
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "%s", strMsg.c_str());
		break;
	case MQTT_NEW_HANDLE_ERROR:
		strMsg += "new mqtt error";
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "%s", strMsg.c_str());
		break;
	case MQTT_NO_INITED:
		strMsg += "has not inited";
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "%s", strMsg.c_str());
		break;
	default:
		strMsg += "mqtt error";
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "%s:%d", strMsg.c_str(), nErrNum);
		break;
	}
	return 0;
}
/************************************************************************/

/************************************************************************/
//************************************
// Method:    my_connect_callback
// FullName:  my_connect_callback
// Access:    public 
// Returns:   void
// Qualifier:/* �ظ�����ʱ���룬�����ｫ���ָ�����������
//1������ȫ������
// Parameter: struct mosquitto * mosq
// Parameter: void * obj  CMqttImpl* �� newһ��mosquittoʱ����
// Parameter: int rc
//************************************
void my_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	//�ָ���������Ҫ��֮ǰ�����ݷ���
	CUpperNodeInfo* pSuerpNode = (CUpperNodeInfo*)obj;
	// time(&pSuerpNode->m_tmLastData); // �ɹ�����ʱ��, �����̳߳�ͻ
	pSuerpNode->m_pMqttObj->m_bConnected = true;
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Connected to UpperNode:%s, IP:%s, Port:%d!", pSuerpNode->m_strName.c_str(), pSuerpNode->m_strIP.c_str(), pSuerpNode->m_nPort);

	int nMid = 0;
	int nRet = pSuerpNode->m_pMqttObj->mqttClient_Sub((char *)pSuerpNode->m_strTopicControl.c_str(), &nMid);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Fail to subscribe UpperNode(%s, %s:%d): recv control topic:%s, retcode:%d", pSuerpNode->m_strName.c_str(), pSuerpNode->m_strIP.c_str(), pSuerpNode->m_nPort, pSuerpNode->m_strTopicControl.c_str(), nRet);
	}
	else
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "Success to subscribe UpperNode(%s, %s:%d): recv control topic:%s", pSuerpNode->m_strName.c_str(), pSuerpNode->m_strIP.c_str(), pSuerpNode->m_nPort, pSuerpNode->m_strTopicControl.c_str());

	CMainTask *pMainTask = MAIN_TASK;
	pMainTask->PulibcAllTagData2Topic(pSuerpNode); // ����1��ȫ�����ݡ�����ᵼ���̳߳�ͻ����˾Ͳ�Ҫ�����ˣ��ȴ���ʱ���ڵ��˷��;Ϳ���
	//int nRet = MAIN_TASK->UpdateAllTagsConf();
	//if (nRet < 0)
	//{
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "UpdateAllTagsConf failed, ret:%d", nRet); //����ʧ��
	//}
	//else

	//	g_logger.LogMessage(PK_LOGLEVEL_INFO, "���ӷ���ɹ�,����������Ϣ:%d��", MAIN_TASK->m_vecTagsName.size());

	//nRet = MAIN_TASK->PulibcAllTagData2Topic();
	//if (nRet < 0)
	//{
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PulibcAllTagData2Topic failed, ret:%d", nRet); //����ʧ��
	//}
	//else
	//	g_logger.LogMessage(PK_LOGLEVEL_INFO, "���ӷ���ɹ�,����ʵʱ��Ϣ:%d��", MAIN_TASK->m_vecTagsName.size());
}
//�Ͽ����ӻص�
//************************************
// Method:    my_disconnect_callback
// FullName:  my_disconnect_callback
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: struct mosquitto * mosq
// Parameter: void * obj  CMqttImpl* �� newһ��mosquittoʱ����
// Parameter: int result
//************************************
void my_disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	//�Ͽ����ӣ���ʾ��Ҫ��ʼ�����Ժ��͵�����
	CUpperNodeInfo* pSuerpNode = (CUpperNodeInfo*)obj;
	// pSuerpNode->m_tmLastData = 0; // �ɹ�����ʱ��
	pSuerpNode->m_pMqttObj->m_bConnected = false;
	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "my_disconnect_callback, Disconnected to UpperNode:%s, IP:%s, Port:%d!", pSuerpNode->m_strName.c_str(), pSuerpNode->m_strIP.c_str(), pSuerpNode->m_nPort);
}

//�����ص� ֻ�з����ɹ��Ż��������Ͽ�����֮�󲻻��������
//���ԣ�ÿ����һ����Ϣ���У������浽���ͻ������У�������ͳɹ��������ｫ��ɾ����
//************************************
// Method:    my_publish_callback
// FullName:  my_publish_callback
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: struct mosquitto * mosq
// Parameter: void * obj
// Parameter: int mid
//************************************
void my_publish_callback(struct mosquitto *mosq, void *obj, int mid)
{
	CUpperNodeInfo* pSuerpNode = (CUpperNodeInfo*)obj;
	CMainTask* pMainTask =  MAIN_TASK;
	//pSuerpNode->m_mapLastSentTagNameVTQ.clear(); // ������ڷ��͵�ֵ�ö��У���������������
}

//************************************
// Method:    my_message_callback
// FullName:  my_message_callback
// Access:    public 
// Returns:   void
// Qualifier: ������Ϣ�ص������������ƶ˵Ŀ�����Ϣ
// Parameter: struct mosquitto * mosq
// Parameter: void * obj
// Parameter: const struct mosquitto_message * msg MSGID:MSG
//************************************
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	CUpperNodeInfo* pSuerpNode = (CUpperNodeInfo*)obj;
	CMainTask* pMainTask = MAIN_TASK;
	string strMessage = (char *)msg->payload;

	//����ǽ��յ��ƶ˷������Ŀ���ͨ������Ϣ
	if (pSuerpNode->m_strTopicControl.compare(msg->topic) != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "recv a message from UpperNode(%s, %s:%d) of topic:%s, content:%s, not control topic:%s!", pSuerpNode->m_strName.c_str(), pSuerpNode->m_strIP.c_str(), pSuerpNode->m_nPort, msg->topic, strMessage.c_str(), pSuerpNode->m_strTopicControl.c_str());
		return;
	}

	Json::Reader jsonReader;
	Json::Value jsonMsg;
	if (!jsonReader.parse(strMessage, jsonMsg, false))
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "recv a message from UpperNode(%s, %s:%d)  of topic:%s, content:%s, not json format!!", pSuerpNode->m_strName.c_str(), pSuerpNode->m_strIP.c_str(), pSuerpNode->m_nPort, msg->topic, strMessage.c_str());
		return;
	}

	Json::Value jsonAction = jsonMsg["action"];
	string strAction = "";
	if (!jsonAction.isNull())
	strAction = jsonAction.asString();

	if (strAction.compare("control") == 0)
	{
		pMainTask->SendControlMsg2LocalNodeServer(strMessage, jsonMsg);
	}
	else if (strAction.compare("version") == 0) //�������ð汾, δ��֤
	{
		pMainTask->PublishAllTagsConf(pSuerpNode);// , REQUIRE_CONFIG_VERSION);
	}
	else if (strAction.compare("tagsconfig") == 0) //�������õ�, δ��֤
	{
		pMainTask->PulibcAllTagData2Topic(pSuerpNode);
	}
	else if (strAction.compare("allrealdata") == 0) //����ȫ����Ϣ, δ��֤
	{
		pMainTask->PulibcAllTagData2Topic(pSuerpNode);
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "recv a message from UpperNode(%s, %s:%d) of topic:%s, content:%s, action:%s not support!!", pSuerpNode->m_strName.c_str(), pSuerpNode->m_strIP.c_str(), pSuerpNode->m_nPort, msg->topic, strMessage.c_str(), strAction.c_str());
		return;
	}

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "recv a message from UpperNode(%s, %s:%d) of topic:%s, content:%s", pSuerpNode->m_strName.c_str(), pSuerpNode->m_strIP.c_str(), pSuerpNode->m_nPort, msg->topic, strMessage.c_str());
}

CMainTask::CMainTask()
{
	m_bStop = false;
	m_nLastLogEvtCount = 0;
	m_nTotalEvtCount = 0;
	m_nTotalEvtFailCount = 0;	// �Ѿ���¼����־����
	time(&m_tmLastLogEvt);
	g_strThisNodeId = "";
	m_hPkData = NULL;
	m_pkAllTagDatas = NULL;
}

CMainTask::~CMainTask()
{
	m_bStop = true;
}

/**
*  ��ʼ�����������̺߳���ط���;
*  get all tags value and publish to topic
*  @return   0/1.
*
*  @version  10/14/2016	yanchaodong   Initial Version
*/
int CMainTask::PublishRealTagData2Topic(bool bAllData, CUpperNodeInfo *pUpperNode, vector<string> &vecTagsName, vector<string> &vecTagsVTQ)
{
	if (vecTagsName.size() != vecTagsVTQ.size())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "===%s PublishRealTagData2Topic, node code:%s, tagname count:%d, tagvalue count:%d", bAllData?"All Data":"IncrementData", pUpperNode->m_pMqttObj->m_szClientID, vecTagsName.size(), vecTagsVTQ.size());
		return -1;
	}

	ACE_Time_Value tvNow = ACE_OS::gettimeofday();
	char szSendTime[32] = { 0 };
	unsigned int nMS = tvNow.usec() / 1000;
	int nSecond = tvNow.sec();
	sprintf(szSendTime, "%10d%03d", nSecond, nMS);

	Json::Reader jsonReader;
	Json::Value jsonTagVTQsOnce;

	vector<string> vecTagNamesOnce;
	vector<string> vecTagsVTQOnce;
	vecTagNamesOnce.reserve(20000);
	vecTagsVTQOnce.reserve(20000); 

	int nRet = PK_SUCCESS;

	// pUpperNode->m_mapLastSentTagNameVTQ.clear();	// ����Ѿ����͵�����
	for (int i = 0; i < vecTagsName.size(); i++)
	{
		if (i > 0 && i % 20000 == 0)// ÿ����� ����1000����
		{
			nRet = PublishRealTagData2Topic_Json(bAllData, pUpperNode, jsonTagVTQsOnce, szSendTime, vecTagNamesOnce, vecTagsVTQOnce);
			vecTagNamesOnce.clear();
			vecTagsVTQOnce.clear();
			jsonTagVTQsOnce.clear(); // ���
			vecTagNamesOnce.reserve(20000);
			vecTagsVTQOnce.reserve(20000); 
		}

		string &strTagName = vecTagsName[i];
		string &strTagVTQ = vecTagsVTQ[i];
		Json::Value jsonTagVTQ;
		// ��ȡTAG���ʱ�䣬 bin
		bool bGetTime = false;
		string strTimeInt = "";
		if (jsonReader.parse(strTagVTQ, jsonTagVTQ, false))
		{
			Json::Value jsonTime = jsonTagVTQ["t"];
			if (!jsonTime.isNull())
			{
				string strTimeHMS = jsonTime.asString();
				unsigned int nMilSecond = 0;
				int nSecond = PKTimeHelper::String2HighResTime(strTimeHMS.c_str(), &nMilSecond);
				char szTimeShort[32] = { 0 };
				sprintf(szTimeShort, "%010d%03d", nSecond, nMilSecond);
				strTimeInt = szTimeShort;
				bGetTime = true;
			}
		}
		if (!bGetTime)
		{
			jsonTagVTQ["t"] = "";
		}
		jsonTagVTQ["t"] = strTimeInt; // ���޸�ʱ�����Ϊ��ֵ�ͣ�����Ϊ�˽�Լ�ռ�
		jsonTagVTQ["n"] = strTagName;

		jsonTagVTQsOnce.append(jsonTagVTQ);
		vecTagNamesOnce.push_back(strTagName);
		vecTagsVTQOnce.push_back(strTagVTQ);
    }

	if (jsonTagVTQsOnce.size() > 0)
	{
		int nRet = PublishRealTagData2Topic_Json(bAllData, pUpperNode, jsonTagVTQsOnce, szSendTime, vecTagNamesOnce, vecTagsVTQOnce);
	}

	vecTagNamesOnce.clear();
	vecTagsVTQOnce.clear();
	jsonTagVTQsOnce.clear(); // ���
	//if(nRet == 0)
	//	g_logger.LogMessage(PK_LOGLEVEL_INFO, "==thisnode:%s,send %s to UpperNode(%s,%s:%d) success,%d tags", bAllData ? "ALL DATA" : "IncrementData", g_strThisNodeId.c_str(), pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, m_vecTagsName.size());
	//else
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "==thisnode:%s, send %s to UpperNode(%s,%s:%d) fail,%d tags, retcode:%d", bAllData ? "ALL DATA" : "IncrementData", g_strThisNodeId.c_str(), pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, m_vecTagsName.size(), nRet);

    return nRet;
}

int CMainTask::PublishRealTagData2Topic_Json(bool bAllData, CUpperNodeInfo *pUpperNode, Json::Value jsonTagVTQs, char *szSendTimeInt, vector<string> &vecTagNamesOnce, vector<string> &vecTagVTQsOnce)
{
	Json::Value jsonGateWay;
	jsonGateWay["id"] = pUpperNode->m_pMqttObj->m_szClientID;
	jsonGateWay["data"] = jsonTagVTQs;
	jsonGateWay["t"] = szSendTimeInt;

    string strSendMsg = m_jsonFastWriter.write(jsonGateWay);
    // LINUX utf8 coding default, need to convert to GBK
#ifndef _WIN32
	// char *dst_gbk = new char[strSendMsg.length() + 1];
	// memset(dst_gbk, 0, strSendMsg.length() + 1);
	// Utf8ToGbk((char *)strSendMsg.c_str(), strSendMsg.length(), dst_gbk, strSendMsg.length());
	// strSendMsg = dst_gbk;
	// delete [] dst_gbk;
#endif

	int nSendLen = strSendMsg.length();
	
	ACE_Time_Value tvBegin = ACE_OS::gettimeofday(); 
	int nRet = pUpperNode->m_pMqttObj->mqttClient_Pub((char*)pUpperNode->m_strTopicRealData.c_str(), NULL, nSendLen, strSendMsg.c_str());
	ACE_Time_Value tvEnd = ACE_OS::gettimeofday();
	ACE_UINT64 nMSec = (tvEnd - tvBegin).get_msec();

    string strLogSendMsg;
    if (strSendMsg.length() > 200)
        strLogSendMsg = strSendMsg.substr(0, 200);
    else
        strLogSendMsg = strSendMsg;
	if (nRet == PK_SUCCESS)
	{ 
		// �����������Ա���������Ƚ�
		for (int iTmp = 0; iTmp < vecTagNamesOnce.size(); iTmp++)
		{
			const string &strTagNameTmp = vecTagNamesOnce[iTmp];
			pUpperNode->m_mapLastSentTagNameVTQ[strTagNameTmp] = vecTagVTQsOnce[iTmp];
		} 

        g_logger.LogMessage(PK_LOGLEVEL_INFO,"SendMsg part content:%s", strLogSendMsg.c_str());
		if(bAllData)
            g_logger.LogMessage(PK_LOGLEVEL_INFO, "===Succes to publish ALL DATA to UpperNode(%s,%s:%d),topic:%s, tagnum:%d, bytes:%d, consume:%d MS", 
			pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, pUpperNode->m_strTopicRealData.c_str(), jsonTagVTQs.size(), nSendLen, nMSec);
		else
            g_logger.LogMessage(PK_LOGLEVEL_INFO, "Succes to publish Increment realdata to UpperNode(%s,%s:%d),topic:%s,tagnum:%d, bytes:%d, consume:%d MS", 
			pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, pUpperNode->m_strTopicRealData.c_str(),jsonTagVTQs.size(), nSendLen, nMSec);
	}
	else
	{
        g_logger.LogMessage(PK_LOGLEVEL_ERROR,"SendMsg part content:%s", strLogSendMsg.c_str());
		if (bAllData) 
            g_logger.LogMessage(PK_LOGLEVEL_ERROR, "===fail to publish ALL DATA to UpperNode(%s,%s:%d),topic:%s,tagnum:%d, bytes:%d, ret:%d, consume:%d MS", 
			pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, pUpperNode->m_strTopicRealData.c_str(),jsonTagVTQs.size(), nSendLen, nRet, nMSec);
		else
            g_logger.LogMessage(PK_LOGLEVEL_ERROR, "fail to publish Increment Data to UpperNode(%s,%s:%d),topic:%s,tagnum:%d, bytes:%d, ret:%d, consume:%d MS", 
			pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, pUpperNode->m_strTopicRealData.c_str(), jsonTagVTQs.size(), nSendLen, nRet, nMSec);
	}

	return nRet;
}
//************************************
// Method:    svc
// FullName:  CMainTask::svc
// Access:    virtual public 
// Returns:   int
// Qualifier:	
/*
1������controlͨ����
2����ʱ��ѯredisֵ�仯���б仯����
3����ʱ��ѯ�����ļ�MD5�汾�Ƿ���ȷ;
*/
//************************************
int CMainTask::svc()
{
    //read all tags name from redis
	//��ȡ��ʼ��״̬

	int nRet = 0;
	nRet = OnStart();
	 
	while (!m_bStop)
	{
		ACE_Time_Value tvNow = ACE_OS::gettimeofday(); // ��ǰʱ�����

 		int nUpperNodeNumToSend = 0;
		for (int iNode = 0; iNode < m_vecUpperNode.size(); iNode++)
		{
			CUpperNodeInfo *pUpperNode = m_vecUpperNode[iNode];
			ACE_Time_Value tvSpan = tvNow - pUpperNode->m_tvLastSendData;
			unsigned int tmp = tvSpan.get_msec(); 
			ACE_UINT64 nTimeSpanMSec = labs(tvSpan.get_msec());

			if (nTimeSpanMSec >= pUpperNode->m_nIntervalMSec) // ����ʱ�䣡
			{
				nUpperNodeNumToSend ++;
			}
		}
		if (nUpperNodeNumToSend <= 0)
		{
			PKComm::Sleep(200);
			continue;
		}

		m_mutex.acquire(); // ����ͻص��������̳߳�ͻ
		//�����Ǵ���Ҫ�������ݵ����
		//1��ѯ�б仯��ֵ����
        vector<string> vecTagsVTQ;
		int nTagNum = m_vecTagsName.size();

		ACE_Time_Value tvBegin = ACE_OS::gettimeofday(); 
		int nRet = pkMGet(m_hPkData, m_pkAllTagDatas, nTagNum);
		ACE_Time_Value tvEnd = ACE_OS::gettimeofday();
		ACE_UINT64 nMSec = (tvEnd - tvBegin).get_msec();
		// g_logger.LogMessage(PK_LOGLEVEL_INFO, "thisnode:%s, have %d uppernode to send data, TagCount:%d, mget data consume:%d MS", g_strThisNodeId.c_str(), nUpperNodeNumToSend, nTagNum, nMSec);

		//�õ�num��
		int nBadCount = 0;
		int nGoodCount = 0;
		for (int i = 0; i < nTagNum; i++)
		{
			if (m_pkAllTagDatas[i].nStatus == 0)
			{
				vecTagsVTQ.push_back(m_pkAllTagDatas[i].szData);
				nGoodCount++;
			}
			else if (strlen(m_pkAllTagDatas[i].szData) > 0)
			{
				nBadCount++;
				vecTagsVTQ.push_back(m_pkAllTagDatas[i].szData);
			}
			else
			{
				nBadCount++;
				vecTagsVTQ.push_back(TAG_QUALITY_UNKNOWN_REASON_STRING);
			}
		}

		if(nRet == PK_SUCCESS)
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "thisnode:%s, GetData of TagCount:%d, bad qualitycount: %d, consume:%d MS", g_strThisNodeId.c_str(), nTagNum, nBadCount, nMSec);
		else
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "thisnode:%s, GetData of TagCount:%d, total retval:%d, bad qualitycount: %d, consume:%d MS", g_strThisNodeId.c_str(), nTagNum, nRet, nBadCount, nMSec);


		for (int iNode = 0; iNode < m_vecUpperNode.size(); iNode++)
		{
			CUpperNodeInfo *pUpperNode = m_vecUpperNode[iNode];

			ACE_Time_Value tvSpan = tvNow - pUpperNode->m_tvLastSendAllData;
			ACE_UINT64 nTimeSpanMSec = tvSpan.get_msec();
			if (nTimeSpanMSec < pUpperNode->m_nIntervalMSec) // ����ʱ�䣡
				continue;

			pUpperNode->m_tvLastSendData = tvNow;

			//first time
			if (nTimeSpanMSec > 60000) // ÿ60�뷢��1��ȫ������
			{
				nRet = PublishRealTagData2Topic(true, pUpperNode, m_vecTagsName, vecTagsVTQ);
				if (nRet == 0)
				{
					pUpperNode->m_tvLastSendAllData = tvNow;
					//g_logger.LogMessage(PK_LOGLEVEL_INFO, "===thisnode:%s,send ALL DATA to UpperNode(%s,%s:%d) success,%d tags", g_strThisNodeId.c_str(), pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, m_vecTagsName.size());
				}
				else
				{
					//g_logger.LogMessage(PK_LOGLEVEL_ERROR, "===thisnode:%s, send ALL DATA to UpperNode(%s,%s:%d) fail,%d tags, retcode:%d", g_strThisNodeId.c_str(), pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, m_vecTagsName.size(), nRet);
				}				
			}
			else    //��ѯ�仯ֵ����
			{
				vector<string> vecUpdateNames;
				vector<string> vecUpdateValues;
				//get time;
				for (int i = 0; i < m_vecTagsName.size(); i++)
				{
					map<string, string>::iterator itVTQ = pUpperNode->m_mapLastSentTagNameVTQ.find(m_vecTagsName[i]);
                    if (itVTQ==pUpperNode->m_mapLastSentTagNameVTQ.end() || vecTagsVTQ[i] != itVTQ->second) //value changed, or not found
					{
						vecUpdateNames.push_back(m_vecTagsName[i]);
						vecUpdateValues.push_back(vecTagsVTQ[i]);
					}
				}

				if (vecUpdateNames.size() > 0)
				{
					nRet = PublishRealTagData2Topic(false, pUpperNode, vecUpdateNames, vecUpdateValues);
					if (nRet == 0)
					{
						//g_logger.LogMessage(PK_LOGLEVEL_INFO, "thisnode:%s, send increment data to UpperNode(%s,%s:%d) success, %d tags", g_strThisNodeId.c_str(), pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, vecUpdateNames.size());
					}
					else
					{
						//g_logger.LogMessage(PK_LOGLEVEL_ERROR, "thisnode:%s, send increment data to UpperNode(%s,%s:%d) failed, %d tags", g_strThisNodeId.c_str(), pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, nRet);
					}
				}
				else
				{
					g_logger.LogMessage(PK_LOGLEVEL_INFO, "thisnode:%s, increment data, have no changed tag to UpperNode(%s,%s:%d), total: %d tags", g_strThisNodeId.c_str(), pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, nTagNum);
				}
				vecUpdateNames.clear();
				vecUpdateValues.clear();
			}//��ѯ�仯ֵ����
		}
		m_mutex.release();

		PKComm::Sleep(200); // ÿN�뷢��1��
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask Exit!");
	OnStop();
	return PK_SUCCESS;
}

// init mqtt publish
// init redis
// subscribe topic��gw-control ���ڽ����ƶ˿�������
//gw-data ����f��������
//gw-config	���ڷ�������
//���ܣ�mqtt������ ��ȡ������Ϣ
int CMainTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	int nRet = 0;

	nRet = LoadConfig();
	if (nRet < 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Read Gateway file failed,exit...");
		return -1;
	}

	for (int iNode = 0; iNode < m_vecUpperNode.size(); iNode++)
	{
		CUpperNodeInfo *pUpperNode = m_vecUpperNode[iNode];
        pUpperNode->m_pMqttObj = new CMqttImpl((char*)pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, (char*)g_strThisNodeId.c_str());
        nRet = pUpperNode->m_pMqttObj->mqttClient_Init(pUpperNode);
		char szTip[128];
		sprintf(szTip, "mqttClient_Init(%s, %d), thisnode:%s", pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, g_strThisNodeId.c_str());
		GetErrMsg(nRet, szTip);

		nRet = pUpperNode->m_pMqttObj->mqttClient_Connect(3000, my_connect_callback, my_disconnect_callback, my_publish_callback, my_message_callback);
		if (nRet != 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "OnStart,fail to connect to UpperNode(name:%s,%s:%d), retcode:%d", pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, nRet);
		}
		else
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "OnStart,success to connect to UpperNode(name:%s,%s:%d)",pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort);

		nRet = pUpperNode->m_pMqttObj->mqttClient_StartLoopWithNewThread();
		if (nRet != 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "OnStart,StartLoopWithNewThread to UpperNode(name:%s,%s:%d), task start failed, retcode:%d", pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, nRet);
		}
		else
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "OnStart,StartLoopWithNewThread to UpperNode(name:%s,%s:%d), subscribe task OK", pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort);
	}

	return nRet;
}

// ������ֹͣʱ;
int CMainTask::OnStop()
{
	pkExit(m_hPkData);
	for (int iNode = 0; iNode < m_vecUpperNode.size(); iNode++)
	{
		CUpperNodeInfo *pUpperNode = m_vecUpperNode[iNode];
		pUpperNode->m_pMqttObj->mqttCLient_UnInit();
		if (pUpperNode->m_pMqttObj != NULL)
		{
			delete pUpperNode->m_pMqttObj;
			pUpperNode->m_pMqttObj = NULL;
		}
	}
	return 0;
}

int CMainTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED);
}

void CMainTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();
}

//************************************
// Method:    PulibcAllTagData2Topic
// FullName:  CMainTask::PulibcAllTagData2Topic
// Access:    public 
// Returns:   int
// Qualifier:
// Parameter: char * szTopic 
//************************************
int CMainTask::PulibcAllTagData2Topic(CUpperNodeInfo *pUpperNode)
{
	m_mutex.acquire();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "PulibcAllTagData2Topic, upperNode:%s, tags count:%d...", pUpperNode->m_strId.c_str(), m_vecTagsName.size());

	if (m_vecTagsName.size() <= 0)
	{
		m_mutex.release();
		/*UpdateAllTagsConf(false);*/
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tags count == 0, wait for config to upload first.");
		return -1;
	}

	int nTagNum = m_vecTagsName.size();
	ACE_Time_Value tvBegin = ACE_OS::gettimeofday();
	int nRet = pkMGet(m_hPkData, m_pkAllTagDatas, nTagNum);
	ACE_Time_Value tvEnd = ACE_OS::gettimeofday();
	ACE_UINT64 nMSec = (tvEnd - tvBegin).get_msec();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "End GetData, upperNode:%s, tags count:%d, ret:%d, consumed:%d MS", pUpperNode->m_strId.c_str(), m_vecTagsName.size(), nRet, nMSec);

	//�õ�num��
	vector<string> vecTagNamesOnce;
	vector<string> vecTagsVTQ;
	for (int i = 0; i < nTagNum; i++)
	{
		string &strTagName = m_vecTagsName[i];
		const char *szTagVTQ = m_pkAllTagDatas[i].szData;

		vecTagNamesOnce.push_back(strTagName);

		if (m_pkAllTagDatas[i].nStatus == 0)
			vecTagsVTQ.push_back(szTagVTQ);
		else if (strlen(szTagVTQ) > 0)
			vecTagsVTQ.push_back(szTagVTQ);
		else
			vecTagsVTQ.push_back(TAG_QUALITY_UNKNOWN_REASON_STRING);
	}
	 
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "===Begin [All Data] PublishRealTagData2Topic, upperNode:%s, tags count:%d", pUpperNode->m_strId.c_str(), m_vecTagsName.size());
	tvBegin = ACE_OS::gettimeofday(); 
	nRet = PublishRealTagData2Topic(true, pUpperNode, vecTagNamesOnce, vecTagsVTQ);
	tvEnd = ACE_OS::gettimeofday();
	nMSec = (tvEnd - tvBegin).get_msec();
	vecTagNamesOnce.clear();
	vecTagsVTQ.clear();

	m_mutex.release();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "===End [All Data] PulibcAllTagData2Topic, upperNode:%s, tags count:%d, ret:%d, consume:%d MS", 
		pUpperNode->m_strId.c_str(), m_vecTagsName.size(), nRet, nMSec);

	return nRet;
}

//************************************
// Method:    UpdateAllTagsConf
// FullName:  CMainTask::UpdateAllTagsConf
// Access:    public 
// Returns:   int
// Qualifier: ��������������Ϣ���汾�����У�
//select tt.name from t_device_tag tt, t_device_list tl, t_device_driver td where(tt.device_id = tl.id and tl.driver_id = td.id)
//************************************
int CMainTask::PublishAllTagsConf(bool bConfVersion /*= false*/)
{
	string strTagsConfig;
	vector<string>::iterator it = m_vecTagsName.begin();
	for (; it != m_vecTagsName.end(); it++)
	{
		strTagsConfig += (*it);
		strTagsConfig += ";";
	}
	strTagsConfig[strTagsConfig.length() - 1] = '\0';
	
	// �����ַ�����md5��
	if (bConfVersion)
	{
		unsigned char* szMD5 = (unsigned char*)strTagsConfig.c_str();
		MD5 md5((const char*)szMD5);
		md5.GenerateMD5(szMD5, strTagsConfig.length());
		strTagsConfig = md5.ToString();
	}

	for (int iNode = 0; iNode < m_vecUpperNode.size(); iNode++)
	{
		CUpperNodeInfo *pUpperNode = m_vecUpperNode[iNode];
		strTagsConfig = pUpperNode->m_strName + SPLIT_GW_MSG_CHAR + strTagsConfig;
		int nRet = pUpperNode->m_pMqttObj->mqttClient_Pub((char*)pUpperNode->m_strTopicConfig.c_str(), NULL, strTagsConfig.length(), strTagsConfig.c_str());
		if(nRet == 0)
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "LowNode:%s, success to send all tag config to UpperNode(%s,%s:%d)", g_strThisNodeId.c_str(), pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort);
		else
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "LowNode:%s, fail to send all tag config to UpperNode(%s,%s:%d)", g_strThisNodeId.c_str(), pUpperNode->m_strName.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort);

	}

	return 0;
}

//************************************
// Method:    SendControlMsg2Driver
// FullName:  CMainTask::SendControlMsg2Driver
// Access:    public 
// Returns:   int
// Qualifier: ���յ��ƶ˷��������������Ƶ���Ϣ�����͵�����
//strCtrlMsg : name:value
//************************************
int CMainTask::SendControlMsg2LocalNodeServer(string strCtrlMsg, Json::Value &jsonMsg)
{
	if (jsonMsg["name"].isNull() || jsonMsg["value"].isNull())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���յ��Ŀ�������:%s, ������name��value, �Ƿ�!", strCtrlMsg.c_str());
		return -1;
	}

	string strTagName = jsonMsg["name"].asString();
	string strTagValue = jsonMsg["value"].asString();
 
	int nRet = pkControl(m_hPkData, strTagName.c_str(), "v", strTagValue.c_str());
	if (nRet == 0)
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���յ���������:%s, ���·�����(v)�ɹ�!", strCtrlMsg.c_str());
	else
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���յ���������:%s, �·�����(v)ʧ��,ret:%d!", strCtrlMsg.c_str(), nRet);
	return 0;
}

int CMainTask::ReadAllTags()
{
	m_vecTagsName.clear();
	// ---- ��ȡ���е�TAG��----.��Ҫ���ݶ���ģʽ�����豸ģʽ���ж�ȡ!
	vector<vector<string> > vecRows;
	int nRet = 0;
	char szSql[1024] = { 0 };
	// �豸ģʽ�µı���
	sprintf(szSql, "select tt.name from t_device_tag tt ,t_device_list tl, t_device_driver td where (tt.device_id = tl.id and tl.driver_id = td.id and (tl.enable is null or tl.enable <> 0 ))");
	string strError;
	nRet = m_dbEview.SQLExecute(szSql, vecRows, &strError);
	if (nRet == 0)
	{
		for (int i = 0; i < vecRows.size(); i++)
		{
			string strTagName = vecRows[i][0];
			m_vecTagsName.push_back(strTagName);
			vecRows[i].clear();
		}
		vecRows.clear();
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��ѯTAG�б�ʧ�ܣ��豸ģʽ��, sql:%s, ����:%d, error:%s", szSql, nRet, strError.c_str());
	}

	// ����ģʽ�¶��������б�
	if (true)
	{
		sprintf(szSql, "select obj.name as objname,prop.name as propname from t_class_list cls, t_class_prop prop,t_object_list obj \
				where prop.class_id = cls.id and obj.class_id = cls.id and (OBJ.enable is NULL or OBJ.enable<>0 or OBJ.enable='') and (obj.device_id = null or obj.device_id in(select id from t_device_list where (t_device_list.enable is null or t_device_list.enable <>0 or t_device_list.enable='') ))");
		string strError;
		vecRows.clear();
		nRet = m_dbEview.SQLExecute(szSql, vecRows, &strError);
		if (nRet == 0)
		{
			for (int i = 0; i < vecRows.size(); i++)
			{
				vector<string> &vecRow = vecRows[i];
				string strObjName = vecRow[0];
				string strPropName = vecRow[1];
				string strTagName = strObjName + "." + strPropName;
				m_vecTagsName.push_back(strTagName);
				vecRow.clear();
			}
			vecRows.clear();
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��ѯ����������б�ʧ��: %s, ����:%d, error:%s", szSql, nRet, strError.c_str());
		}
	}

	int nTagNum = m_vecTagsName.size();
	m_pkAllTagDatas = new PKDATA[nTagNum]();
	for (int i = 0; i < m_vecTagsName.size(); i++)
	{
		PKStringHelper::Safe_StrNCpy(m_pkAllTagDatas[i].szObjectPropName, m_vecTagsName[i].c_str(), sizeof(m_pkAllTagDatas[i].szObjectPropName));
		strcpy(m_pkAllTagDatas[i].szFieldName, "");
	}

	// ��ʼ���ڴ����ݿ�
	if (m_hPkData == NULL)
	{
		m_hPkData = pkInit("127.0.0.1", NULL, NULL);
		if (m_hPkData == NULL)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Init PKData failed");
			return -1;
		}
	}
	return 0;
}

//************************************
// Method:    GetGwConfig
// FullName:  CMainTask::GetGwConfig
// Access:    public 
// Returns:   int
// Qualifier: ��ȡ����������Ϣ���ڱ� t_gateway_conf��
//************************************
int CMainTask::LoadConfig()
{
	m_dbEview.InitFromConfigFile("db.conf", "database");
	char szSql[] = "select id,name,description,ip,port,auth,period from t_node_upper_list where enable=1 or enable is NULL or enable=''";
	GetGateWayId();

	vector<vector<string> > vecRows;
	string strError;
	int nRet = m_dbEview.SQLExecute(szSql, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "t_node_upper_list query Node EXCEPTION, sql: %s, error:%s", szSql, strError.c_str());
		m_dbEview.UnInitialize();
		return -1;
	}

	//��ʾû�в�ѯ��
	if (vecRows.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "t_node_upper_list has NO UpperNode record, sql: %s", szSql);
		m_dbEview.UnInitialize();
		return -1;
	}

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "t_node_upper_list query %d UpperNode record", vecRows.size());

	for (int iRow = 0; iRow < vecRows.size(); iRow++)
	{

		vector<string> &vecRow = vecRows[iRow];

		int iCol = 0;
		string strId = NULLASSTRING(vecRow[iCol].c_str());
		iCol++;
		string strNodeName = NULLASSTRING(vecRow[iCol].c_str());
		iCol++;
		string strDescription = NULLASSTRING(vecRow[iCol].c_str());
		iCol++;
		string strIp = NULLASSTRING(vecRow[iCol].c_str());
		iCol++;
		string strPort = NULLASSTRING(vecRow[iCol].c_str());
		iCol++;
		string strAuth = NULLASSTRING(vecRow[iCol].c_str());
		iCol++;
		string strPeriod = NULLASSTRING(vecRow[iCol].c_str()); // ����Ϊ��λ��ȱʡΪ3000��ʾ3��
		iCol++;
		vecRow.clear();

		if (strIp.empty())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "t_node_upper_list query IP is empty(), id:%s, sql: %s", strId.c_str(),  szSql, strError.c_str());
			continue;
		}

		CUpperNodeInfo *pUpperNode = new CUpperNodeInfo();
		pUpperNode->m_strId = strId;
		pUpperNode->m_strName = strNodeName;
		pUpperNode->m_strDesc = strDescription;
		pUpperNode->m_strIP = strIp;
		pUpperNode->m_nPort = ::atoi(strPort.c_str());
		pUpperNode->m_strAuthInfo = strAuth;
		if (pUpperNode->m_nPort <= 0)
			pUpperNode->m_nPort = UpperNode_LISTEN_PORT;
		pUpperNode->m_nIntervalMSec = ::atoi(strPeriod.c_str());
		if (pUpperNode->m_nIntervalMSec <= 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "t_node_upper_list query UpperNode(id:%s,ip:%s,port:%d,name:%s), interval:%d <= 0, change to %d seconds", strId.c_str(), 
				pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, pUpperNode->m_strName.c_str(), pUpperNode->m_nIntervalMSec, DEFAULT_PUBLISHDATA_INTERVAL_MSEC);
			pUpperNode->m_nIntervalMSec = DEFAULT_PUBLISHDATA_INTERVAL_MSEC;
		}

		if (pUpperNode->m_nIntervalMSec > 60 * 60 * 1000)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "t_node_upper_list query UpperNode(id:%s,ip:%s,port:%d,name:%s), interval:%d > 60 * 60, change to %d seconds", strId.c_str(),
				pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, pUpperNode->m_strName.c_str(), pUpperNode->m_nIntervalMSec, DEFAULT_PUBLISHDATA_INTERVAL_MSEC);
			pUpperNode->m_nIntervalMSec = DEFAULT_PUBLISHDATA_INTERVAL_MSEC;
		}

		pUpperNode->m_strTopicRealData = TOPIC_REALDATA_PREFIX;
		pUpperNode->m_strTopicRealData = pUpperNode->m_strTopicRealData + "/" + g_strThisNodeId;

		// uppernode/control/ABCDABCDABCD
		pUpperNode->m_strTopicControl = TOPIC_CONTROL_PREFIX;
		pUpperNode->m_strTopicControl = pUpperNode->m_strTopicControl + "/" + g_strThisNodeId;
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "UpperNode(id:%s,ip:%s,port:%d,name:%s), topic", strId.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, pUpperNode->m_strName.c_str());

		m_vecUpperNode.push_back(pUpperNode);
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "t_node_upper_list query UpperNode(id:%s,ip:%s,port:%d,name:%s), topicControl:%s, topicReal:%s", 
			strId.c_str(), pUpperNode->m_strIP.c_str(), pUpperNode->m_nPort, pUpperNode->m_strName.c_str(), pUpperNode->m_strTopicControl.c_str(), pUpperNode->m_strTopicRealData.c_str());
	}
	vecRows.clear();

	nRet = ReadAllTags();
	m_dbEview.UnInitialize();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "LoadConfig end, uppernode count:%d, tagnum:%d", m_vecUpperNode.size(), m_vecTagsName.size());
	return nRet;
}

int CMainTask::GetGateWayId()
{
	ACE_OS::macaddr_node_t macaddress;
	int result = ACE_OS::getmacaddress(&macaddress); // ��windows���ǵ�һ�����ڣ�����������������mac��ַ
	if (result != -1)
	{
		char szMacAddr[32] = { 0 };
		sprintf(szMacAddr, "%02X%02X%02X%02X%02X%02X",
			macaddress.node[0],
			macaddress.node[1],
			macaddress.node[2],
			macaddress.node[3],
			macaddress.node[4],
			macaddress.node[5]);
		g_strThisNodeId = szMacAddr;
		return 0;
	}
	else
	{
		g_strThisNodeId = "";
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "GetGatewayIdByMacAddress failed! Please Check!!!!!!");
		return -100;
	}
}

