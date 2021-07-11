/**************************************************************
 *  Filename:    CtrlProcessTask.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 控制命令处理类.
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
#define TOPIC_REALDATA_PREFIX			"uppernode/realdata"			// 实时数据发送通道, peak/realdata/{gatewayID}
#define TOPIC_CONFIG_PREFIX				"uppernode/config"				// 配置通道, peak/config/{gatewayID}
#define TOPIC_CONTROL_PREFIX			"uppernode/control"				// 控制通道, peak/control/{gatewayID}

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
	CMqttImpl	*m_pMqttObj;		// 表示和某个上级节点的一个连接
	string		m_strMqttListenIp;	// 127.0.0.1
	unsigned short		m_nMqttListenPort;	// MQTT 侦听端口
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
	// 线程初始化
	int OnStart();
	// 线程中停止
	void OnStop();
	// 从配置文件加载和重新加载所有的配置theTaskGroupList
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
