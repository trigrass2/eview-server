/**************************************************************
*  Filename:    MainTask.h
*  Copyright:   Shanghai peakinfo Software Co., Ltd.
*
*  Description: mqtt read redis and publish to topic.
*
*  @author:     yanchaodong
*  @version     04/16/2018  yanchaodong  Initial Version
**************************************************************/
#pragma once

#include <ace/Task.h>
#include <ace/Thread_Mutex.h>
#include <string>
#include <map>
#include <vector>
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"
#include "pkcomm/pkcomm.h"
#include "sqlite/CppSQLite3.h"
#include "Sync2UpperNodeServer.h"
#include "mqttImpl.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "pkdata/pkdata.h"

#define UpperNode_LISTEN_PORT			1883
#define DEFAULT_PUBLISHDATA_INTERVAL_MSEC	5000						// ȱʡÿ3�뷢��1��
#define TOPIC_REALDATA_PREFIX			"uppernode/realdata"				// ʵʱ���ݷ���ͨ��, peak/realdata/{gatewayID}
#define TOPIC_CONFIG_PREFIX				"uppernode/config"				// ����ͨ��, peak/config/{gatewayID}
#define TOPIC_CONTROL_PREFIX			"uppernode/control"				// ����ͨ��, peak/control/{gatewayID}


/************************************************************************/
/* ���������ƶ˵���Ϣ���ͣ�
*/
/************************************************************************/
enum _MsgTypeFromEview
{
	REQUIRE_CONTROL = 0,
	REQUIRE_CONFIG_VERSION = 1,
	REQUIRE_CONFIG_TAGS,
	REQUIRE_ALL_TAGS_VALUE,
	REQUEST_OF_MID
};

////�豸������Ϣ 
class CUpperNodeInfo
{
public: // ��̬������Ϣ
	string m_strId;	// û������
	string	m_strName;
	string  m_strDesc;
	string  m_strIP;
	unsigned short	m_nPort; // ȱʡΪ1834
	string m_strAuthInfo; // ����ļ�Ȩ��Ϣ���������û������õ�Ψһ��Ȩ��Ϣ
	int		m_nIntervalMSec;			// �������ڣ���λΪ���룬ȱʡΪ3��
public:
	string	m_strTopicRealData;		// ����ʵʱ���ݵ�����
 	string	m_strTopicControl;		// ���տ������������
	string m_strTopicConfig;		// �������õ��ϼ��ڵ��topic

public:
	time_t		m_tmLastData;		// �ϴ��շ����ݵĳɹ�ʱ�䣬���������ͳɹ����յ������
	ACE_Time_Value m_tvLastSendData;	// �ϴ��շ����ݵĳɹ�ʱ�䣬���������ͳɹ���ʧ��
	ACE_Time_Value m_tvLastSendAllData;	// �ϴ��շ��������ݵĳɹ�ʱ�䣬���������ͳɹ���ʧ��

	CMqttImpl	*m_pMqttObj;		// ��ʾ��ĳ���ϼ��ڵ��һ������

	//�ϴη������ݣ����ڱȽ��Ƿ���Ҫ���·��ͣ��������е㷢��ֵ
	map<string, string>     m_mapLastSentTagNameVTQ; 

public:
	CUpperNodeInfo()
	{
		m_strName = m_strDesc = m_strIP = m_strAuthInfo = m_strId = "";
		m_strTopicRealData = m_strTopicControl = "";
		m_nPort = UpperNode_LISTEN_PORT; //
		m_nIntervalMSec = DEFAULT_PUBLISHDATA_INTERVAL_MSEC;
		m_tmLastData = 0;
		m_tvLastSendData = m_tvLastSendAllData = ACE_Time_Value(0);
		m_pMqttObj = NULL;
	}
};

class CMainTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();

	int Start();
	void Stop();
protected:
    bool	m_bStop;

	time_t m_tmLastLogEvt; // �ϴδ�ӡ�¼���־���¼�
	long m_nLastLogEvtCount;	// �Ѿ���¼����־����
	long m_nTotalEvtCount;
	long m_nTotalEvtFailCount;

	Json::FastWriter    m_jsonFastWriter;
	vector<CUpperNodeInfo *>		m_vecUpperNode;	// Ҫ���͵Ķ�������ڵ����Ϣ

    CPKDbApi			m_dbEview;
	//�������ݶ�д
	PKHANDLE			m_hPkData;
	PKDATA *			m_pkAllTagDatas;

	int GetGateWayId();
	ACE_Thread_Mutex	m_mutex;
public:
	virtual int svc();

	// �̳߳�ʼ��
	int OnStart();
	// �߳���ֹͣ
	int OnStop();

	int		LoadConfig();
    int		PublishRealTagData2Topic(bool bAllData, CUpperNodeInfo *pUpperNode, vector<string> &vecTagName, vector<string> &vecTagVTQ);
	int		PublishRealTagData2Topic_Json(bool bAllData, CUpperNodeInfo *pUpperNode, Json::Value jsonTagVTQs, char *szSendTimeInt, vector<string> &vecTagNamesOnce, vector<string> &vecTagVTQsOnce);
	int		PulibcAllTagData2Topic(CUpperNodeInfo *pUpperNode);
	//�������ݰ汾��
	//����������������
	int		PublishAllTagsConf(bool bConfVersion = false);

	//���Ϳ����źŵ�redis controlͨ��
	int SendControlMsg2LocalNodeServer(string strMsg, Json::Value &jsonMsg);
	int ReadAllTags();
 
public:	
	vector<string>          m_vecTagsName;
};

// ����tag������������Ϣ,����¼��־��ʹ��

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()
