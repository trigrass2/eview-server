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

//MQTT 服务端连接参数信息;
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
	bool m_bconnected;			//连接状态;
	CMqttImpl	*m_pMqttObj;	//Mqtt连接信息;
	map<string, string>	m_mapUpdatingValues; //全量数据;

	typedef struct _TAGINFO	//tag点信息;
	{
		string strTagName;	//tag点名称;
		string showType;	//Json中数据分类;
		string outputType;	//实际输出的数据类型;
		double calcValue;	//计算因子;
		int precision;		//输出精度;
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

	typedef struct _DEVICEINFO//设备信息;
	{
		int deviceID;
		int driverID;
		string strName;
		string strGatewayName;
		string strSubObjectName;
		_DEVICEINFO(){
			deviceID = 0;
			driverID = 0;	//默认事件驱动为20000;
			strName = "";
			strGatewayName = "";
			strSubObjectName = "";
		}
	}DEVICEINFO;

	typedef struct _EVENTINFO//事件信息;
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
		bool operator ==(const _EVENTINFO &info) const //重载比较运算符;
		{
			return ((info.code == this->code) && (info.deviceID ==this->deviceID));
		}
	}EVENTINFO;

private:
    bool m_bStop;
	time_t m_tmLastExecute;				// 上次执行的时间;
	GATEWAYCONF		m_gateWayConf;
    CPKEviewDbApi   m_dbEview;
	PKHANDLE		m_hPkData;
	PKDATA *		m_pkAllTagDatas;

	vector<TAGINFO>     m_vecTagsInfo;	//存储所有的tag点信息;
	vector<DEVICEINFO>	m_deviceCommon;	//统计Common设备信息;
	vector<DEVICEINFO>	m_deviceEvent;	//统计包含事件的设备;

	list<EVENTINFO>		m_eventInfo;				//统计所有事件的信息;
	map<string, string>	m_mapTagsName2UpdateTime;	//值变化的数据;


public:
	CMainTask();
	virtual ~CMainTask();
	int Start();
	void Stop();
	int SendControlMsg2LocalNodeServer(string strMsg, Json::Value &jsonMsg); //控制指令的相关操作;
	int	UpdateAllTags2Topic(); //更新点位信息;

private:
	int svc();
	int OnStart();
	int OnStop();
	int	LoadConfig();

	int GetGateWayId();	  //获取当前边缘服务器的Mac地址;
	int ReadAllTags();	  //读取内存数据库中所有点位信息;

	int GetEventJsonValue(Json::Value &curDevice, int deviceID, int code, int state);   //获取事件Json串;
	int	PublishRealTagData2Topic(vector<TAGINFO>, vector<string>, bool bEvent = false);	//发送实时数据到Mqtt订阅端;
	int GetCommonJsonValue(vector<string> &vecTagsValue, vector<TAGINFO> &vecTagsName, Json::Value &deviceInfo); //获取常规数据Json串;
	int GetEventDetails(vector<TAGINFO> &vecEventTag, vector<string> &vecEventTagValues, map<string, int> &mState, map<string, int> &mCode, vector<string> vecTagsValue); //获取事件相关的详细信息;
	int GetCodeAndState(TAGINFO &TagsCode, TAGINFO &TagsState, string &vecTagsValueCode, string &vecTagsValueState, int &code, int &state);	//获取设备对应的事件名称和状态;

};
// 保存tag点的所有相关信息,供记录日志是使用;
#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _ALARM_ROUTE_TASK_H_
