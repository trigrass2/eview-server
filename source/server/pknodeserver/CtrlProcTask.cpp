/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 控制命令处理类.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  确认和删除报警也放在控制里面进行执行。这里也是二道贩子，将其转发给数据处理就完事
**************************************************************/
#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include "math.h"
#include "CtrlProcTask.h"
#include "pkcomm/pkcomm.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/eviewdefine.h"
#include "pklog/pklog.h"
#include "PKNodeServer.h"
#include "DataProcTask.h"
#include "boost/regex.hpp"
#include "MainTask.h"
#include "TagProcessTask.h"
#include "UserAuthTask.h"

extern CPKNodeServer* g_pPKNodeServer;
extern CProcessQueue * g_pQueData2NodeSrv;
extern CPKLog g_logger;
int GetPKMemDBPort(int &nListenPort, string &strPassword);

#define	DEFAULT_SLEEP_TIME		100	

// 如果json是object那么asString会异常
// 如果json是string，toStyledString会多出双引号，故而需要写个函数实现
int Json2String(Json::Value &json, string &strJson)
{
	strJson = "";
	if (json.isNull())
	{
		strJson = "";
	}
	else if (json.isString())
	{
		strJson = json.asString();
		return 0;
	}
	else if (json.isDouble())
	{
		char szTmp[32] = { 0 };
		sprintf(szTmp, "%f", json.asDouble());
		strJson = szTmp;
	}
	else if (json.isInt() || json.isNumeric())
	{
		char szTmp[32] = { 0 };
		sprintf(szTmp, "%d", json.asInt());
		strJson = szTmp;
	}
	else if (json.isUInt())
	{
		char szTmp[32] = { 0 };
		sprintf(szTmp, "%d", json.asUInt());
		strJson = szTmp;
	}
	else if (json.isBool())
	{
		char szTmp[32] = { 0 };
		sprintf(szTmp, "%d", json.asBool() ? 1 : 0);
		strJson = szTmp;
	}
	else if (json.isObject() || json.isArray())
	{
		strJson = CONTROLPROCESS_TASK->m_jsonWriter.write(json); // 性能更高
		// strJson = json.toStyledString();
	}

	return 0;
}

// 抛消息给处理线程。 这是在另外一个线程中执行的!
/*
void OnChannel_ControlMsg(const char *szChannelName, const char *szMessage, void *pCallbackParam) // 发送的回调函数。当需要分批发送时（比如中间需要返回进度，需要设定每个包的长度
{
	CCtrlProcTask *pControlProcess = (CCtrlProcTask *)pCallbackParam;
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "CtrlProcTask recv an controls msg:%s",szMessage);

	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(szMessage, root, false))
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask recv an msg(%s), invalid!",szMessage);
		// g_pPbServer->UpdateServerStatus(-2,"control msg format invalid");
		return;
	}

	if (!root.isObject() && !root.isArray())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask recv an msg(%s), not an object or array!",szMessage);
		// g_pPbServer->UpdateServerStatus(-2,"control msg format invalid");
		return;
	}

	// 根据tag名称，找到对应的驱动对应的订阅通道，发给该驱动即结束 【确认报警和控制都在这里处理了】
	pControlProcess->RelayControlCmd2Driver(szMessage, root);
}
*/


/**
 *  构造函数.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CCtrlProcTask::CCtrlProcTask()
{
	m_bStop = false;
}

/**
 *  析构函数.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CCtrlProcTask::~CCtrlProcTask()
{
	
}

/**
 *  虚函数，继承自ACE_Task.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
int CCtrlProcTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();

	int nRet = 0;
	nRet = OnStart();

	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	while(!m_bStop)
	{
		//ACE_Time_Value tvSleep;
		//tvSleep.msec(DEFAULT_SLEEP_TIME); // 100ms
		//ACE_OS::sleep(tvSleep);		

		// channelname:simensdrv,value:[{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]
		string strChannel;
		string strMessage;
		nRet = m_redisSub.receivemessage(strChannel, strMessage, 500); // 500 ms
		// 更新驱动的状态
		if(nRet != 0)
		{
			PKComm::Sleep(200); // receivermessage may not sleep!
			continue;
		}

		if (strChannel.compare(CHANNELNAME_CONTROL) != 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "CtrlProcTask recv an message but not CONTROL msg!!channel:%s, message:%s", strChannel.c_str(), strMessage.c_str());
			continue;
		}

		//g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "pknodeserver 收到命令,内容:%s", strMessage.c_str());

		Json::Reader reader;
		Json::Value root;
		if (!reader.parse(strMessage, root, false))
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask recv an msg(%s), not json format, invalid!", strMessage.c_str());
			continue;
		}

		if (!root.isObject() && !root.isArray())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask recv an msg . type invalid:%d,not an object or array.message:%s!", root.type(), strMessage.c_str());
			continue;
		}

		// 根据tag名称，找到对应的驱动对应的订阅通道，发给该驱动即结束 【确认报警和控制都在这里处理了】
		this->RelayControlCmd2Driver(strMessage.c_str(), root);
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"nodeserver CtrlProcTask exit normally!");
	OnStop();

	return PK_SUCCESS;	
}


int CCtrlProcTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CCtrlProcTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();
}


int CCtrlProcTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	int nPKMemDBListenPort = PKMEMDB_LISTEN_PORT;
	string strPKMemDBPassword = PKMEMDB_ACCESS_PASSWORD;
	GetPKMemDBPort(nPKMemDBListenPort, strPKMemDBPassword);

	nStatus = m_redisPub.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str(), 0);	
	if(nStatus != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Control Process Task, Initialize RW failed, ret:%d", nStatus);
	}	
	
	nStatus = m_redisSub.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str());
	if(nStatus != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Control Process Task, Initialize Sub failed, ret:%d", nStatus);
	}

	nStatus = m_redisSub.subscribe(CHANNELNAME_CONTROL);
	
	return nStatus;
}

// 本任务停止时
void CCtrlProcTask::OnStop()
{
	m_redisSub.finalize();
	m_redisPub.finalize();
}

// 发送数据给事件服务进行记录
int CCtrlProcTask::LogControl2Event(const char *szUserName, CDataTag *pDataTag, const char *szTagVal, const char *szTime)
{
	Json::Value	jsonCtrl;
	
	string strValueLabel = GetControlLabel(pDataTag->strName.c_str(), atoi(szTagVal));
	if (strValueLabel.length() == 0) // 如果没有设置 控制值对应的显示内容（label），取缺省内容
	{
		strValueLabel =  szTagVal;
	}
	string strContent = pDataTag->strName;
	strContent = strContent + "," + strValueLabel;

	jsonCtrl[EVENT_TYPE_MSGID] = EVENT_TYPE_CONTROL;
	jsonCtrl["username"] = szUserName;
	jsonCtrl["name"] = pDataTag->strName.c_str();
	jsonCtrl["value"] = szTagVal;

	jsonCtrl["content"] = strContent;
	jsonCtrl["rectime"] = szTime;
	jsonCtrl["subsys"] = pDataTag->strSubsysName.c_str();
	string strLogCtrl = m_jsonWriter.write(jsonCtrl); // 性能更高

	m_redisPub.publish(CHANNELNAME_EVENT, strLogCtrl.c_str());
	m_redisPub.commit();
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "变量控制日志, tagname:%s, value:%s, username:%s to log", pDataTag->strName.c_str(), szTagVal, szUserName);
	return 0;
}

// Json中数据为：[{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]  {\"name\":\"tag1\",\"value\":\"100\"}
int CCtrlProcTask::RelayControlCmd2Driver(const char *szCtrlCmd, Json::Value &root)
{
	// g_logger.LogMessage(PK_LOGLEVEL_INFO,"pknodeserver收到命令：%s", szCtrlCmd);
	int nRet = PK_SUCCESS;
	vector<Json::Value *> vectorCmds;
	if (root.isObject())
	{
		vectorCmds.push_back(&root);
	}
	else if(root.isArray())
	{
		for(unsigned int i = 0; i < root.size(); i ++)
			vectorCmds.push_back(&root[i]);
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ctrls(%s),format error，应为json array or object：{\"type\":\"control\",\"data\":{\"name\":\"tag1\",\"value\":\"100\"}",szCtrlCmd);
		//g_pPKNodeServer->UpdateServerStatus(-12,"control format invalid");
		return -12;
	}

	for(vector<Json::Value *>::iterator itCmd = vectorCmds.begin(); itCmd != vectorCmds.end(); itCmd ++)
	{
		Json::Value &jsonCmd = **itCmd;

		string strActionType = "";
		Json::Value jsonActionType = jsonCmd["action"]; // 区分控制还是确认报警
		// 报警单独处理，其余的认为是控制
		if(!jsonActionType.isNull())
			Json2String(jsonActionType, strActionType);	
		
		if(PKStringHelper::StriCmp(strActionType.c_str(), ACTIONNAME_CLI2SERVER_ALARM_CONFIRM) == 0)
		{
			// 确认的报警类型,(现在确认报警的消息格式是这样的),发送命令到control通道：
			// {"action" : "confirmalarm","tagname" : "hk_nvr220_chn1_status","judgemethod" : "HH", "username" : "admin2",  "process" : "how to process?"}
			string strUserName, strTagFilter, strJudgemethod, strLevelFilter, strProduceTime, strHowToProcess;
			Json::Value jsonTagFilter = jsonCmd["tagname"];
			Json::Value jsonJudgemethod = jsonCmd["judgemethod"];
			Json::Value jsonLevelFilter = jsonCmd["level"];
			Json::Value jsonUserName = jsonCmd["username"];
			Json::Value jsonProduceTime = jsonCmd["producetime"]; // 怎么处理
			//Json::Value jsonHowToProcess = jsonCmd["process"]; // 怎么处理
			Json2String(jsonUserName, strUserName);	
			Json2String(jsonTagFilter, strTagFilter);	
			Json2String(jsonJudgemethod, strJudgemethod);	
			Json2String(jsonLevelFilter, strLevelFilter);	
			Json2String(jsonProduceTime, strProduceTime);
			//Json2String(jsonHowToProcess, strHowToProcess);

			ConfirmAlarm(strTagFilter, strJudgemethod, strLevelFilter, strProduceTime, strUserName, strHowToProcess); // 只有报警过滤过去，其他认为是控制
			continue;
		}
		else if (PKStringHelper::StriCmp(strActionType.c_str(), ACTIONNAME_CLI2SERVER_ALARM_SET_PARAM) == 0) // 修改tag点的某个报警的参数
		{
			// 修改报警参数
			string strUserName, strTagName, strJudgemethod, strAlarmParam;
			Json::Value jsonTagName = jsonCmd["tagname"];
			Json::Value jsonJudgemethod = jsonCmd["judgemethod"];
			Json::Value jsonAlarmParam = jsonCmd["alarmparam"];
			Json::Value jsonUserName = jsonCmd["username"];
			Json2String(jsonUserName, strUserName);
			Json2String(jsonTagName, strTagName);
			Json2String(jsonJudgemethod, strJudgemethod);
			Json2String(jsonAlarmParam, strAlarmParam);

			this->m_eaprocessor.ProcessAlarmParamChange(strTagName, strJudgemethod, strAlarmParam, strUserName); // 修改报警参数
			continue;
		}
		else if (PKStringHelper::StriCmp(strActionType.c_str(), ACTIONNAME_CLI2SERVER_ALARM_SUPPRESS) == 0) // 抑制报警一段时间
		{
			// 发送命令到control通道，格式如下：
			// {"action":"suppressalarm", "judgemethod" : "HH", "tagname" : "hk_nvr220_chn1_status", "suppresstime" : "2018-01-28 19:15:00" }

			// 抑制报警一段时间
			string strUserName, strTagName, strJudgemethod, strProduceTime, strSuppressTime;
			Json::Value jsonTagName = jsonCmd["tagname"];
			Json::Value jsonJudgemethod = jsonCmd["judgemethod"];
			Json::Value jsonProuceTime = jsonCmd["producetime"];
			Json::Value jsonSuppressTime = jsonCmd["suppresstime"];
			Json::Value jsonUserName = jsonCmd["username"];
			Json2String(jsonUserName, strUserName);
			Json2String(jsonTagName, strTagName);
			Json2String(jsonJudgemethod, strJudgemethod);
			Json2String(jsonSuppressTime, strSuppressTime);
			Json2String(jsonProuceTime, strProduceTime);

			SuppressAlarm(strTagName, strJudgemethod, strProduceTime, strSuppressTime, strUserName); // 修改报警参数
			continue;
		}

		// 根据地址得到设备信息和其余地址串。以冒号:隔开
		Json::Value tagName = jsonCmd["name"];
		string strTagNames = tagName.asString();

		// 刑星说上墙要将上墙的摄像头id写到点cycle_TV_wall0，也记不清具体的逻辑了，先暂作修改，只针对南宁的问题！！！之后应该是，消息里面增加type字段，类型是上墙。
		// [{"name":"cycle_TV_wall0","value":{"cameras":[{"name":"testVideo","id":"1"}],"operatePriority":"","operateTimeout":""},"username":"admin"}]
		Json::Value tagValue = jsonCmd["value"];
		Json::Value jsonUserName = jsonCmd["username"];
		if(tagName.isNull() || tagValue.isNull()){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver，cannot find name and value attributes in data, ctrl:%s", szCtrlCmd);
			nRet = -2;
			continue;
		}

		string strTagName,strTagValue;
		Json2String(tagName, strTagName);	
		Json2String(tagValue, strTagValue);	
		string strUserName = "系统";
		if(!jsonUserName.isNull())
			Json2String(jsonUserName, strUserName);	

		// 根据tag名称找到tag点对应的驱动
		map<string, CDataTag *>::iterator itMap = MAIN_TASK->m_mapName2Tag.find(strTagName);
		if (itMap == MAIN_TASK->m_mapName2Tag.end()){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver，cannot find taginfo by name:%s, ctrl:%s", strTagName.c_str(), szCtrlCmd);
			nRet = -3;
			continue;
		}

		CDataTag *pDataTag = itMap->second;
		if(pDataTag == NULL){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver, find taginfo by name:%s, pServerTag==null, ctrl:%s", strTagName.c_str(), szCtrlCmd);
			nRet = -4;
			continue;
		}

		// 内存变量、预定义变量、模拟变量三种类型变量的控制命令处理
		if (pDataTag->nTagType == VARIABLE_TYPE_MEMVAR || pDataTag->nTagType == VARIABLE_TYPE_PREDEFINE || pDataTag->nTagType == VARIABLE_TYPE_SIMULATE)
		{
			nRet = TAGPROC_TASK->PostWriteTagCmd(pDataTag, strTagValue);
			continue;
		}

		if (pDataTag->nTagType == VARIABLE_TYPE_ACCUMULATE_TIME || pDataTag->nTagType == VARIABLE_TYPE_ACCUMULATE_COUNT)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver, username:%s, tagname:%s, value:%s, 累计变量不能控制!", strUserName.c_str(), strTagName.c_str(), szCtrlCmd);
			continue;
		}

		if (pDataTag->nTagType != VARIABLE_TYPE_DEVICE)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver, username:%s, tagname:%s, value:%s, 未知变量类型:%d, 不能控制!", strUserName.c_str(), strTagName.c_str(), szCtrlCmd, pDataTag->nTagType);
			continue;
		}

		if (!USERAUTH_TASK->HasAuthByTagAndLoginName(strUserName, pDataTag))
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver, username:%s, tagname:%s, value:%s no auth", strUserName.c_str(), strTagName.c_str(), szCtrlCmd);
			continue;
		}

		CDeviceTag *pDeviceTag = (CDeviceTag *)pDataTag;
		char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
		PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
		Json::Value	jsonCtrl;
		jsonCtrl["action"] = strActionType;
		jsonCtrl["name"] = strTagName.c_str();
		jsonCtrl["value"] = strTagValue.c_str();
		jsonCtrl["username"] = strUserName.c_str();

		CMyDriverPhysicalDevice *pDriver = (CMyDriverPhysicalDevice *)pDeviceTag->pDevice->m_pDriver;
		CProcessQueue * pDrvProcQue = pDriver->m_pDrvShm;
		if(!pDrvProcQue)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver, driver shm by name:%s == NLL,ctrl:%s", pDriver->m_strName.c_str(), szCtrlCmd);
			nRet = -5;
			continue;
		}

		// 实际设备的驱动，需要发送给设备执行命令
		string strNewCtrl = m_jsonWriter.write(jsonCtrl); // 性能更高
		nRet = pDrvProcQue->EnQueue(strNewCtrl.c_str(), strNewCtrl.length());
		if(nRet != 0){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "收到控制命令, 转发给驱动: ctrl to :%s shm failed, tagname:%s, is null,ctrl:%s", pDriver->m_strName.c_str(), strTagName.c_str(), szCtrlCmd);
			nRet = -6;
			continue;
		}

		m_redisPub.commit();
		LogControl2Event(strUserName.c_str(), pDataTag, strTagValue.c_str(), szTime);
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "收到控制命令, tagname:%s, value:%s, 转分给 驱动:%s successfully!", strTagName.c_str(), strTagValue.c_str(), pDriver->m_strName.c_str());
	}
	return nRet;
}

string CCtrlProcTask::GetControlLabel(string strTagName, int nControlVal)
{
	string strLabel = "";
	map<string, CDataTag *>::iterator itmapTag = MAIN_TASK->m_mapName2Tag.find(strTagName);
	// 根据tag点名找到labelid
	if (itmapTag != MAIN_TASK->m_mapName2Tag.end())
	{
		CDataTag *pTag = itmapTag->second;
		// 根据labelid 找到=相应的map
		map<int, map<int, CLabel *> >::iterator itmapLabelMap = MAIN_TASK->m_mapId2Label.find(pTag->nLabelID);
		if (itmapLabelMap != MAIN_TASK->m_mapId2Label.end())
		{
			// 在map的map里面查找当前控制值对应的label
			map<int, CLabel *> mapLabel = itmapLabelMap->second;
			map<int, CLabel *>::iterator itmapLabel = mapLabel.find(nControlVal);
			if (itmapLabel != mapLabel.end())
			{
				CLabel *pLabel = itmapLabel->second;
				strLabel = pLabel->strLabelName;
			}	
		}
	}
	return strLabel;	
}

void string_replace(string&s1,const string&s2,const string&s3)
{
	string::size_type pos=0;
	string::size_type a=s2.size();
	string::size_type b=s3.size();
	while((pos=s1.find(s2,pos))!=string::npos)
	{
		s1.replace(pos,a,s3);
		pos+=b;
	}
}

// 处理确认报警
int CCtrlProcTask::ConfirmAlarm(string &strTagFilter, string &strJudgeMethod, string &strLevelFilter, string strProduceTime, string &strUsername, string &strHowToProcess)
{
	int nRet = 0;
	int nMatchPos = strTagFilter.find("*");

	// 精确匹配的tag点
	if (nMatchPos == string::npos) // 未找到 * 号，是绝对匹配，进一步判断是确认所有级别还是确认某个级别
	{
		string strTagName = strTagFilter; // 仅仅是一个tag点
		map<string, CDataTag *>::iterator itMapTag = MAIN_TASK->m_mapName2Tag.find(strTagName);
		if (itMapTag == MAIN_TASK->m_mapName2Tag.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "确认点的报警时，未找到对应的点:%s,judgemethod:%s", strTagName.c_str(), strJudgeMethod.c_str());
			return -1;
		}

		CDataTag *pTag = itMapTag->second;

		bool hasAlarmConfirmed = false;
		// 遍历点的所有报警点，对正在报警的点或已经恢复的点进行确认
		for (int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
		{
			CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];
			if (strJudgeMethod.compare("*") != 0 && PKStringHelper::StriCmp(strJudgeMethod.c_str(), pAlarmTag->strJudgeMethod.c_str()) != 0)
				continue;

			for (int i = 0; i < pAlarmTag->vecAlarming.size(); i++)
			{
				CAlarmingInfo *pAlarmingInfo = pAlarmTag->vecAlarming[i];
				char szProduceTime1[32] = { 0 };
				PKTimeHelper::HighResTime2String(szProduceTime1, sizeof(szProduceTime1), pAlarmingInfo->tmProduce.nSeconds, pAlarmingInfo->tmProduce.nMilSeconds);
				if (!strProduceTime.empty() && strProduceTime.compare(szProduceTime1) != 0)
					continue;

				if (pAlarmingInfo->bConfirmed)// 已确认. 20170807：出现过内存中已经确认了，但内存数据库并未确认的情况
				{
					g_logger.LogMessage(PK_LOGLEVEL_WARN, "确认点的报警时，发现内存中该点已经确认过了.保险期间，继续确认.对应的TAG点:%s,judgemethod:%s", strTagName.c_str(), strJudgeMethod.c_str());
					//continue;
				}

				pAlarmingInfo->strConfirmUserName = strUsername;
				//pAlarmTag->strHowToProcess = strHowToProcess; // 确认时的说明
				m_eaprocessor.OnEAConfirm(pAlarmTag, szProduceTime1, (char *)strUsername.c_str(), true, 0, 0);
				hasAlarmConfirmed = true;
			}
		}

		return 0;
	}

	/*
	// 模糊匹配的tag点
	// 仅支持包含一个*号,*XX或XX*。以*为间隔分为两部分
	string strAlmTagPrefix =""; // *的前面部分，nms1.*aaa，对象nms1下的所有变量，nms1.
	string strAlmTagSuffix = ""; // *的后面部分，nms1.*aaa，对象nms1下的所有变量，aaa
	*/

 	string strPattern = strTagFilter;
    string_replace(strPattern, "*", "[0-9a-zA-Z\\.]*"); // 正则表达式化
    boost::regex pattern(strPattern);//,regex_constants::extended);
	map<string, CDataTag *>::iterator itMapTag = MAIN_TASK->m_mapName2Tag.begin();
	for (; itMapTag != MAIN_TASK->m_mapName2Tag.end(); itMapTag++)
	{
	 	CDataTag *pTag = itMapTag->second;
	 	string strTagName = pTag->strName;
        std::string::const_iterator start = strTagName.begin();
        std::string::const_iterator end   = strTagName.end();
        boost::smatch result;
        //boost::match_results<std::string::const_iterator> result;
        bool bValid = false;
        while (boost::regex_search(strTagName, result, pattern))
        {
            bValid = true;
            break;
        }
        //bool bValid = boost::regex_match(strTagName,result,pattern);
		if(!bValid)
			continue;

		bool hasAlarmConfirmed = false;
		for (int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
		{
			CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];
			if (PKStringHelper::StriCmp(strJudgeMethod.c_str(), pAlarmTag->strJudgeMethod.c_str()) != 0 && strJudgeMethod.compare("*") != 0)
			{
				continue;
			}
				
			for (int i = 0; i < pAlarmTag->vecAlarming.size(); i++)
			{
				CAlarmingInfo *pAlarmingInfo = pAlarmTag->vecAlarming[i];
				char szProduceTime1[32] = { 0 };
				PKTimeHelper::HighResTime2String(szProduceTime1, sizeof(szProduceTime1), pAlarmingInfo->tmProduce.nSeconds, pAlarmingInfo->tmProduce.nMilSeconds);
				if (!strProduceTime.empty() && strProduceTime.compare(szProduceTime1) != 0)
					continue;

				m_eaprocessor.OnEAConfirmOne(pAlarmingInfo, strUsername.c_str(), true, 0, 0);
				hasAlarmConfirmed = true;
			}
		}
	}
	return nRet;
}

// 当抑制报警时
int CCtrlProcTask::SuppressAlarm(string strTagName, string strJudgeMethod, string strProuceTime, string  strSuppressTime, string strUserName)
{
	int nRet = 0;
	// 根据tag名称找到tag点对应的驱动
	map<string, CDataTag *>::iterator itMap = MAIN_TASK->m_mapName2Tag.find(strTagName);
	if (itMap == MAIN_TASK->m_mapName2Tag.end()){
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "OnSuppressAlarm(%s,%s,%s,%s)，cannot find taginfo by name",
			strTagName.c_str(), strJudgeMethod.c_str(), strSuppressTime.c_str(), strUserName.c_str());
		nRet = -3;
		return nRet;
	}

	CDataTag *pTag = itMap->second;
	// 遍历点的所有报警点，对正在报警的点或已经恢复的点进行确认
	for (int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
	{
		CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];

		// 未确认的报警，如果判别类型为*、空或者和该报警相等
		if (strJudgeMethod.compare("*") != 0 && PKStringHelper::StriCmp(strJudgeMethod.c_str(), pAlarmTag->strJudgeMethod.c_str()) != 0)
			continue;

		m_eaprocessor.OnSuppressAlarm(pAlarmTag, (char *)strProuceTime.c_str(), strSuppressTime, strUserName);
	}
	return 0;
}