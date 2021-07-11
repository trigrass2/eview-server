/************************************************************************/
/* 网关实体类：
初始化，连接，发布，订阅
*/
/************************************************************************/
#include "ace/OS.h"
#include "mqttImpl.h"
#include <memory.h>
#include "vector"
#include "pkcomm/pkcomm.h"
#include "pklog/pklog.h"
extern CPKLog g_logger;

CMqttImpl::~CMqttImpl()
{
	if (m_bInit != true)
		mqttCLient_UnInit();
}

CMqttImpl::CMqttImpl(const char* ip, unsigned short port, char* szClientID)
{
	memset(m_szAddrIP, 0x0, sizeof(m_szAddrIP));
	memset(m_szClientID, 0x0, sizeof(m_szClientID));

	memcpy(m_szAddrIP, ip, strlen(ip));
	if (szClientID)
		memcpy(m_szClientID, szClientID, strlen(szClientID));
	m_nPort = port;

	m_bInit = false;
	m_bConnected = false;
}

//  init sdk
//  new handle
//  connect
int CMqttImpl::mqttClient_Init(void* pObj)
{
	int nRet = 0;
	nRet = mosquitto_lib_init();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mosquitto_lib_init failed, ret:%d", nRet);
		return nRet;
	}

	m_hMqtt = mosquitto_new(m_szClientID, true, pObj);
	if (m_hMqtt == NULL)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mosquitto_new failed, retvalue == NULL, ret:%d", MQTT_NEW_HANDLE_ERROR);
		return MQTT_NEW_HANDLE_ERROR;
	}

	//init library;
	//return success;
	m_bInit = true;
	return nRet;
}
//mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain);
/*
* 	MOSQ_ERR_SUCCESS -      on success.
* 	MOSQ_ERR_INVAL -        if the input parameters were invalid.
* 	MOSQ_ERR_NOMEM -        if an out of memory condition occurred.
* 	MOSQ_ERR_NO_CONN -      if the client isn't connected to a broker.
*	MOSQ_ERR_PROTOCOL -     if there is a protocol error communicating with the
*                          broker.
* 	MOSQ_ERR_PAYLOAD_SIZE - if payloadlen is too large.
*/
int CMqttImpl::mqttClient_Pub(const char* topic, int *mid, int msgLen, const char* szMsg)
{
	if (m_bInit == false)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mqttClient_Pub failed, please init first");
		return MQTT_NO_INITED;
	}

	if (m_bConnected == false)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mqttClient_Pub failed, please connect first");
		return MQTT_NO_CONNECTED;
	}

	int nRet = mosquitto_publish(m_hMqtt, mid, topic, msgLen, szMsg, 0, false);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mosquitto_publish failed, return:%d", nRet);
		return MQTT_NO_CONNECTED;
	}

	return nRet;
}

int CMqttImpl::mqttCLient_UnInit()
{
	if (m_hMqtt != NULL)
		mosquitto_destroy(this->m_hMqtt);

	mosquitto_lib_cleanup();
	m_bInit = false;
	m_bConnected = false;
	return 0;
}

/*
* 	keepalive - the number of seconds after which the broker should send a PING
*              message to the client if no other messages have been exchanged
*              in that time.
* return : *
* MOSQ_ERR_SUCCESS - on success.
* 	MOSQ_ERR_INVAL -   if the input parameters were invalid.
* 	MOSQ_ERR_ERRNO -   if a system call returned an error. The variable errno
*                     contains the error code, even on Windows.
*                     Use strerror_r() where available or FormatMessage() on
*                     Windows.
*/
int CMqttImpl::mqttClient_Connect(int nKeepAlive, CBFuncConnect* CBConn, CBFuncDisConnect* CBDisConn, CBFuncPublish* CBPub, CBFuncSubscribe* CBSub)
{
	if (m_bInit == false)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mqttClient_Connect failed, please init first");
		return MQTT_NO_INITED;
	}

	mosquitto_connect_callback_set(m_hMqtt, CBConn);
	mosquitto_disconnect_callback_set(m_hMqtt, CBDisConn);
	mosquitto_publish_callback_set(m_hMqtt, CBPub);
	mosquitto_message_callback_set(m_hMqtt, CBSub);

	//连接mqtt
	int nRet = mosquitto_connect(m_hMqtt, m_szAddrIP, m_nPort, nKeepAlive);
	if (nRet == MOSQ_ERR_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "mosquitto_connect success, url:%s:%d, keepalive:%d", m_szAddrIP, m_nPort, nKeepAlive);
		m_bConnected = true;
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mosquitto_connect failed, return:%d, url:%s:%d, keepalive:%d", nRet, m_szAddrIP, m_nPort, nKeepAlive);
		m_bConnected = false;
	}
	return nRet;
}

//************************************
// Method:    mqttClient_Sub
// FullName:  CMqttPublish::mqttClient_Sub
// Access:    public 
// Returns:   int
// Qualifier: 订阅topic
// Parameter: char * topic 需要侦听的通道号，以; 分割
// Parameter: int * mid
//************************************
int CMqttImpl::mqttClient_mSub(vector<string> vecTopics, vector<string>& vecSucceed, int *mid)
{
	if (m_bInit == false)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mqttClient_Pub failed, please init first");
		return MQTT_NO_INITED;
	}

	if (m_bConnected == false)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mqttClient_Pub failed, please connect first");
		return MQTT_NO_CONNECTED;
	}

	ACE_Time_Value tv;
	tv.set_msec(200);

	for (int i = 0; i < vecTopics.size(); i++)
	{
		string &strTopic = vecTopics[i];
		int nRet = mosquitto_subscribe(m_hMqtt, mid, strTopic.c_str(), 0);
		if (nRet == MOSQ_ERR_SUCCESS)
		{
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "mosquitto_subscribe success, channel:%s", strTopic.c_str());
			m_bConnected = true;
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mosquitto_subscribe failed, return:%d, channel:%s", nRet, strTopic.c_str());
			m_bConnected = false;
		}

		ACE_OS::sleep(tv);
		if (nRet == 0)
		{
			vecSucceed.push_back(vecTopics[i]);
		}
	}
	// mosquitto_loop_start(m_hMqtt); // 每次都开启一个线程，还是一个m_hMqtt一个线程？
	return vecSucceed.size();
}


//************************************
// Method:    mqttClient_Sub
// FullName:  CMqttPublish::mqttClient_Sub
// Access:    public 
// Returns:   int
// Qualifier: 订阅topic
// Parameter: char * topic
// Parameter: int * mid
//************************************
int CMqttImpl::mqttClient_Sub(const char* topic, int *mid)
{
	if (m_bInit == false)
		return MQTT_NO_INITED;
	if (m_bConnected == false)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "连接mqttserver失败，未连接");
	}

	int nRet = mosquitto_subscribe(m_hMqtt, mid, topic, 0);

	return nRet;
}

int CMqttImpl::mqttClient_StartLoopWithNewThread()
{
	int nRet = mosquitto_loop_start(m_hMqtt);
	return nRet;
}

int CMqttImpl::mqttClient_LoopInCurrentThread()
{
	return mosquitto_loop(m_hMqtt, 1000, 10);
}
