/**************************************************************
 *  Filename:    CtrlProcessTask.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
 *  @version     01/28/2012  wanyingjie  Initial Version
**************************************************************/

#ifndef _MAIN_TASK_H_
#define _MAIN_TASK_H_

#include <ace/Task.h>
#include "json/json.h"
#include "json/writer.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "NodeCommonDef.h"
#include "PKSuperNodeServer.h"
#include "pkdbapi/pkdbapi.h"
#include <map>
using namespace  std;

#define DEFAULT_MQTT_PORT				1883
#define TOPIC_REALDATA_PREFIX			"uppernode/realdata"			// ʵʱ���ݷ���ͨ��, peak/realdata/{gatewayID}
#define TOPIC_CONFIG_PREFIX				"uppernode/config"				// ����ͨ��, peak/config/{gatewayID}
#define TOPIC_CONTROL_PREFIX			"uppernode/control"				// ����ͨ��, peak/control/{gatewayID}

class CMainTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();
	map<string, CNodeInfo *>		m_mapCode2Node;
	map<string, CNodeInfo *>		m_mapId2Node;
	map<string, CNodeTag *>			m_mapName2Tag; 

	CMemDBClientAsync	m_redisRW0;
	CMemDBClientAsync	m_redisRW1; 
	CMemDBClientAsync	m_redisRW2;
	
public:
	CMqttImpl	*m_pMqttObj;		// ��ʾ��ĳ���ϼ��ڵ��һ������
	string		m_strMqttListenIp;	// 127.0.0.1
	unsigned short		m_nMqttListenPort;	// MQTT �����˿�
	int StartMQTT();
	int CheckConnectToMqttServer();
	string Json2String(Json::Value &jsonVal);
	Json::FastWriter            m_jsonWriter;

public:
	int Start();
	void Stop();

protected:
	bool			m_bStop;
	bool			m_bStopped;
	int				m_nTagNum;

protected:
	virtual int svc();
	// �̳߳�ʼ��
	int OnStart();
	// �߳���ֹͣ
	void OnStop();
	// �������ļ����غ����¼������е�����theTaskGroupList
	int LoadConfig();

	int InitMemDBTags();
	void ClearMemDBTags();
 protected:
	int LoadInitNodeConfig(CPKDbApi &pkdbApi);
	int LoadInitTagConfig(CPKDbApi &pkdbApi);
	int WriteTagConfigAndInitValToMemDB();
};

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _MAIN_TASK_H_
