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
#define DEFAULT_PUBLISHDATA_INTERVAL_MSEC	5000						// 缺省每3秒发送1次
#define TOPIC_REALDATA_PREFIX			"uppernode/realdata"				// 实时数据发送通道, peak/realdata/{gatewayID}
#define TOPIC_CONFIG_PREFIX				"uppernode/config"				// 配置通道, peak/config/{gatewayID}
#define TOPIC_CONTROL_PREFIX			"uppernode/control"				// 控制通道, peak/control/{gatewayID}


/************************************************************************/
/* 接收来自云端的消息类型：
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

////设备驱动信息 
class CUpperNodeInfo
{
public: // 静态配置信息
	string m_strId;	// 没有意义
	string	m_strName;
	string  m_strDesc;
	string  m_strIP;
	unsigned short	m_nPort; // 缺省为1834
	string m_strAuthInfo; // 输入的鉴权信息，可能是用户密码获得的唯一鉴权信息
	int		m_nIntervalMSec;			// 发送周期，单位为：秒，缺省为3秒
public:
	string	m_strTopicRealData;		// 发送实时数据的主题
 	string	m_strTopicControl;		// 接收控制命令的主题
	string m_strTopicConfig;		// 发送配置到上级节点的topic

public:
	time_t		m_tmLastData;		// 上次收发数据的成功时间，包括：发送成功，收到命令等
	ACE_Time_Value m_tvLastSendData;	// 上次收发数据的成功时间，包括：发送成功或失败
	ACE_Time_Value m_tvLastSendAllData;	// 上次收发所有数据的成功时间，包括：发送成功或失败

	CMqttImpl	*m_pMqttObj;		// 表示和某个上级节点的一个连接

	//上次发送内容，用于比较是否需要重新发送，保存所有点发送值
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

	time_t m_tmLastLogEvt; // 上次打印事件日志的事件
	long m_nLastLogEvtCount;	// 已经记录的日志条数
	long m_nTotalEvtCount;
	long m_nTotalEvtFailCount;

	Json::FastWriter    m_jsonFastWriter;
	vector<CUpperNodeInfo *>		m_vecUpperNode;	// 要传送的多个超级节点的信息

    CPKDbApi			m_dbEview;
	//用于数据读写
	PKHANDLE			m_hPkData;
	PKDATA *			m_pkAllTagDatas;

	int GetGateWayId();
	ACE_Thread_Mutex	m_mutex;
public:
	virtual int svc();

	// 线程初始化
	int OnStart();
	// 线程中停止
	int OnStop();

	int		LoadConfig();
    int		PublishRealTagData2Topic(bool bAllData, CUpperNodeInfo *pUpperNode, vector<string> &vecTagName, vector<string> &vecTagVTQ);
	int		PublishRealTagData2Topic_Json(bool bAllData, CUpperNodeInfo *pUpperNode, Json::Value jsonTagVTQs, char *szSendTimeInt, vector<string> &vecTagNamesOnce, vector<string> &vecTagVTQsOnce);
	int		PulibcAllTagData2Topic(CUpperNodeInfo *pUpperNode);
	//发送数据版本号
	//发送所有配置数据
	int		PublishAllTagsConf(bool bConfVersion = false);

	//发送控制信号到redis control通道
	int SendControlMsg2LocalNodeServer(string strMsg, Json::Value &jsonMsg);
	int ReadAllTags();
 
public:	
	vector<string>          m_vecTagsName;
};

// 保存tag点的所有相关信息,供记录日志是使用

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()
