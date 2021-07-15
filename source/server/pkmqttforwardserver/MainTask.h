/**************************************************************
*  Filename:    MainTask.h
*  Copyright:   XinHong Software Co., Ltd.
*
*  Description: mqtt read redis and publish to topic.
*
*  @author:     xingxing
*  @version     07/06/2021  xingxing  Initial Version
**************************************************************/
#ifndef _EVIEWFORWARD_MAINTASK_H_
#define _EVIEWFORWARD_MAINTASK_H_
#include <ace/Task.h>
#include <string>
#include <map>
#include <vector>
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"
#include "pkcomm/pkcomm.h"
#include "sqlite/CppSQLite3.h"
#include "MqttForwardServer.h"
#include "mqttImpl.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "pkdata/pkdata.h"
#include <algorithm>
#include <list>

//MQTT ��������Ӳ�����Ϣ;
class GATEWAYCONF
{
public:
	string	m_strForwardName;
	string  m_strDescription;
	string  m_strConnParam;
	string	m_strMqqtIP;
	string	m_strPort;
	string	m_strUserName;
	string	m_strPassword;
	GATEWAYCONF()
	{
		m_strForwardName = m_strDescription = m_strConnParam = m_strMqqtIP = m_strPort = m_strUserName = m_strPassword = "";
	}
};

class CMainTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	bool m_bconnected;			//����״̬;
	CMqttImpl	*m_pMqttObj;	//Mqtt������Ϣ;
	map<string, string>	m_mapUpdatingValues; //ȫ������;

	typedef struct _TAGINFO	//tag����Ϣ;
	{
		string strTagName;	//tag������;
		string showType;	//Json�����ݷ���;
		string outputType;	//ʵ���������������;
		double calcValue;	//��������;
		int precision;		//�������;
		int deviceID;
		_TAGINFO()
		{
			strTagName = showType = outputType = "";
			calcValue = 0.0;
			deviceID = precision = 0;
		}
		bool operator ==(const _TAGINFO &info) const
		{
			return (info.strTagName == strTagName);
		}
	}TAGINFO;

	typedef struct _DEVICEINFO//�豸��Ϣ;
	{
		int deviceID;
		int driverID;
		string strName;
		string strGatewayName;
		string strSubObjectName;
		_DEVICEINFO(){
			deviceID = 0;
			driverID = 0;	//Ĭ���¼�����Ϊ20000;
			strName = "";
			strGatewayName = "";
			strSubObjectName = "";
		}
	}DEVICEINFO;

	typedef struct _EVENTINFO//�¼���Ϣ;
	{
		int deviceID;
		int code;
		string name;
		string action;
		string classes;
		string protecttype;
		string source;
		_EVENTINFO()
		{
			deviceID = code = 0;
			name = action = classes = protecttype = source = "";
		}
		bool operator ==(const _EVENTINFO &info) const //���رȽ������;
		{
			return ((info.code == this->code) && (info.deviceID ==this->deviceID));
		}
	}EVENTINFO;

private:
    bool m_bStop;
	time_t m_tmLastExecute;				// �ϴ�ִ�е�ʱ��;
	GATEWAYCONF		m_gateWayConf;
    CPKEviewDbApi   m_dbEview;
	PKHANDLE		m_hPkData;
	PKDATA *		m_pkAllTagDatas;

	vector<TAGINFO>     m_vecTagsInfo;	//�洢���е�tag����Ϣ;
	vector<DEVICEINFO>	m_deviceCommon;	//ͳ��Common�豸��Ϣ;
	vector<DEVICEINFO>	m_deviceEvent;	//ͳ�ư����¼����豸;

	list<EVENTINFO>		m_eventInfo;				//ͳ�������¼�����Ϣ;
	map<string, string>	m_mapTagsName2UpdateTime;	//ֵ�仯������;


public:
	CMainTask();
	virtual ~CMainTask();
	int Start();
	void Stop();
	int SendControlMsg2LocalNodeServer(string strMsg, Json::Value &jsonMsg); //����ָ�����ز���;
	int	UpdateAllTags2Topic(); //���µ�λ��Ϣ;

private:
	int svc();
	int OnStart();
	int OnStop();
	int	LoadConfig();

	int GetGateWayId();	  //��ȡ��ǰ��Ե��������Mac��ַ;
	int ReadAllTags();	  //��ȡ�ڴ����ݿ������е�λ��Ϣ;

	int GetEventJsonValue(Json::Value &curDevice, int deviceID, int code, int state);   //��ȡ�¼�Json��;
	int	PublishRealTagData2Topic(vector<TAGINFO>, vector<string>, bool bEvent = false);	//����ʵʱ���ݵ�Mqtt���Ķ�;
	int GetCommonJsonValue(vector<string> &vecTagsValue, vector<TAGINFO> &vecTagsName, Json::Value &deviceInfo); //��ȡ��������Json��;
	int GetEventDetails(vector<TAGINFO> &vecEventTag, vector<string> &vecEventTagValues, map<string, int> &mState, map<string, int> &mCode, vector<string> vecTagsValue); //��ȡ�¼���ص���ϸ��Ϣ;
	int GetCodeAndState(TAGINFO &TagsCode, TAGINFO &TagsState, string &vecTagsValueCode, string &vecTagsValueState, int &code, int &state);	//��ȡ�豸��Ӧ���¼����ƺ�״̬;

};
// ����tag������������Ϣ,����¼��־��ʹ��;
#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _ALARM_ROUTE_TASK_H_
