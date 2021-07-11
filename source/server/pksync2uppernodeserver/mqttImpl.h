#ifndef MQTTCLIENT_IMPL_H
#define MQTTCLIENT_IMPL_H
#include <mosquitto/mosquitto.h>
#ifdef WIN32
#include "windows.h"
#else
#include <unistd.h>
#include "sys/stat.h"
#include <sys/ioctl.h>
#endif

#include <vector>
#include <string>
using namespace std;

#define MQTT_SUCCESS            0
#define MQTT_NEW_HANDLE_ERROR   -1
#define MQTT_NO_INITED          -2
#define MQTT_NO_CONNECTED		-3

typedef void(CBFuncConnect)(struct mosquitto *mosq, void *obj, int rc);
typedef void(CBFuncDisConnect)(struct mosquitto *mosq, void *obj, int result);
typedef void(CBFuncPublish)(struct mosquitto *mosq, void *obj, int mid);
typedef void(CBFuncSubscribe)(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);

//mqtt client
class  CMqttImpl
{
public:
	CMqttImpl(const char* szIP, unsigned short port, char* szClientID);
    ~CMqttImpl();
    struct mosquitto *m_hMqtt;
public:
    int mqttClient_Init(void* pObj);
	int mqttClient_Connect(int nKeepAlive, CBFuncConnect* CBConn = NULL, CBFuncDisConnect* CBDisConn = NULL, CBFuncPublish* CBPub = NULL, CBFuncSubscribe* CBSub = NULL);
    int mqttCLient_UnInit();
    int mqttClient_Pub(const char* topic, int *mid, int msgLen, const char* szMsg);
	int mqttClient_mSub(vector<string> vecTopics, vector<string>& vecSucceed, int *mid);
	int mqttClient_Sub(const char* topic, int *mid);
	int mqttClient_LoopInCurrentThread();
	int mqttClient_StartLoopWithNewThread();

public:
    bool    m_bInit;
    bool    m_bConnected;
	char    m_szClientID[16];
private:
    char    m_szAddrIP[32] ;
    unsigned short   m_nPort;
    
};

#endif // MQTTCLIENT_IMPL_H
