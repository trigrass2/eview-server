/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 控制命令处理类.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  shijunpu  Initial Version
 *  @version	9/17/2012  shijunpu  数据块类型修改为字符串类型，并且修改增加数据块时的接口
 *  @version	3/1/2013   shijunpu  增加由点生成块的逻辑，当设备下没有配置数据块时，直接由点生成数据块 .
**************************************************************/

#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include "math.h"
#include "MainTask.h"
#include "pkcomm/pkcomm.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/eviewdefine.h"
#include "pklog/pklog.h"

extern CPKLog PKLog;

#define	DEFAULT_SLEEP_TIME		100	


#include <fstream>  
#include <streambuf>  
#include <iostream>  

// 得到PKMemDB的端口号PKMemDB的密码
int GetPKMemDBPort(int &nListenPort, string &strPassword)
{
	nListenPort = PKMEMDB_LISTEN_PORT;
	strPassword = PKMEMDB_ACCESS_PASSWORD;

	string strConfigPath = PKComm::GetConfigPath();
	strConfigPath = strConfigPath + PK_OS_DIR_SEPARATOR + "pkmemdb.conf";
	std::ifstream textFile(strConfigPath.c_str());
	if (!textFile.is_open())  // 不存在该文件，则认为是sqlite：eview.db
	{
		// PKLog.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s not exist", strConfigPath.c_str());
		return -1;
	}

	// 计算文件大小  
	textFile.seekg(0, std::ios::end);
	std::streampos len = textFile.tellg();
	textFile.seekg(0, std::ios::beg);

	// 方法1  
	std::string strConfigData((std::istreambuf_iterator<char>(textFile)), std::istreambuf_iterator<char>());
	textFile.close();
	vector<string> vecLine = PKStringHelper::StriSplit(strConfigData, "\n");
	for (int iLine = 0; iLine < vecLine.size(); iLine++)
	{
		string strLine = vecLine[iLine];
		vector<string> vecContent = PKStringHelper::StriSplit(strLine, " ");
		if (vecContent.size() < 2)
			continue;
		string strKey = vecContent[0];
		string strValue = vecContent[1];
		if (strKey.compare("port") == 0)
			nListenPort = ::atoi(strValue.c_str());
		if (strKey.compare("requirepass") == 0)
			strPassword = strValue.c_str();
	}
	return 0;
}

void UpdateServerStatus(int, char *)
{

}

extern int PushStringToBuffer(char *&pSendBuf, int &nLeftBufLen, const char *szStr, int nStrLen);
extern int PushInt32ToBuffer(char *&pBufer, int &nLeftBufLen, int nValue);
extern int PopInt32FromBuffer(char *&pBufer, int &nLeftBufLen, int &nValue);
extern int PopStringFromBuffer(char *&pBufer, int &nLeftBufLen, string &strValue);

// 抛消息给处理线程。 这是在另外一个线程中执行的!
/*
void OnChannel_PMControlMsg(const char *szChannelName, const char *szMessage, void *pCallbackParam) // 发送的回调函数。当需要分批发送时（比如中间需要返回进度，需要设定每个包的长度
{
	CMainTask *pMainTask = (CMainTask *)pCallbackParam;
	int nMsgLen = sizeof(int) + sizeof(int) * 2 + strlen(szChannelName) + strlen(szMessage);
	ACE_Message_Block *pMsgBlk = new ACE_Message_Block(nMsgLen);
	char *pSendBuf = pMsgBlk->wr_ptr();
	int nLeftBufLen = nMsgLen;
	char *pBufBegin = pMsgBlk->wr_ptr();
	int nCmdNo = 0;
	PushInt32ToBuffer(pSendBuf, nLeftBufLen, nCmdNo);
	PushStringToBuffer(pSendBuf, nLeftBufLen, szChannelName, strlen(szChannelName));
	PushStringToBuffer(pSendBuf, nLeftBufLen, szMessage, strlen(szMessage));
	pMsgBlk->wr_ptr(pSendBuf - pBufBegin);
	pMainTask->putq(pMsgBlk);

	PKLog.LogMessage(PK_LOGLEVEL_INFO, "CtrlProcTask recv an controls msg:%s",szMessage);
}
*/

/**
 *  构造函数.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CMainTask::CMainTask()
{
	m_bStop = false;
	m_bStopped = false;
}

/**
 *  析构函数.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CMainTask::~CMainTask()
{
	
}

/**
 *  虚函数，继承自ACE_Task.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
int CMainTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();

	int nRet = 0;
	nRet = OnStart();

	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	// 必须先清空队列里面已有的所有PM消息，避免缓存的退出消息导致程序无法启动
	if (!m_cvProcMgr.IsGeneralPM())
	{
		while (true){
			string strChannel;
			string strMessage;
			nRet = m_memDBSubSync.receivemessage(strChannel, strMessage, 500); // 500 ms
			// 更新驱动的状态
			if (nRet != 0)
				break;
		}
	}

	while(!m_bStop)
	{
		// 检查是否有进程退出
		m_cvProcMgr.MaintanceProcess();

		//ACE_Time_Value tvTimes = ACE_OS::gettimeofday() + ACE_Time_Value(0, 100 * 1000);
		string strChannel;
		string strMessage;
		if (m_cvProcMgr.IsGeneralPM())
			ACE_OS::sleep(1);
		else
		{
			nRet = m_memDBSubSync.receivemessage(strChannel, strMessage, 500); // 500 ms
			// 更新驱动的状态
			if (nRet != 0)
				continue;

			if (strChannel.compare(CHANNELNAME_PM_CONTROL) != 0)
			{
				PKLog.LogMessage(PK_LOGLEVEL_INFO, "CtrlProcTask recv a msg but not %s msg.channel:%s, message:%s", CHANNELNAME_PM_CONTROL, strChannel.c_str(), strMessage.c_str());
				continue;
			}

			Json::Reader reader;
			Json::Value jsonRoot;
			if (!reader.parse(strMessage, jsonRoot, false))
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "pkServerManger recv an msg(%s), invalid!", strMessage.c_str());
				continue;
			}

			if (!jsonRoot.isObject() && !jsonRoot.isArray())
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "pkServerManger recv an msg(%s), not an object or array!", strMessage.c_str());
				continue;
			}

			OnRemoteCmd(strMessage, jsonRoot);
			PKLog.LogMessage(PK_LOGLEVEL_INFO, "PKServerMgr recv an controls msg:%s", strMessage.c_str());
		}
	}//while(!m_bStop)

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"Process Manager Maintask exit normally！");
	OnStop();

	m_bStopped = true;
	return PK_SUCCESS;	
}

int CMainTask::InitAndClearPMTags()
{
	if (m_cvProcMgr.IsGeneralPM())
		return 0;

#ifndef DSIABLE_PM_REMOTE_API
	// 不能清除！否则db0的pm都会被清掉，导致程序状态获取不到
	int nRet = m_memDBClientAsync.flushall();
#ifdef _WIN32
	PKLog.LogMessage(PK_LOGLEVEL_DEBUG, "init to delete all db！初始化清除所有的DB！！！");
#else
	PKLog.LogMessage(PK_LOGLEVEL_DEBUG, "init to delete all db!");
#endif
	//m_redisRW0.flushdb();
	//nRet = m_redisRW2.flushdb();
	//PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "初始化清除DB2");

	string strKeys = m_memDBClientAsync.keys("keys pm.*");
	vector<string> vecKeys = PKStringHelper::StriSplit(strKeys, "&");

	for(vector<string>::iterator itKey = vecKeys.begin(); itKey != vecKeys.end(); itKey ++)
	{
		string strKey = *itKey;
		m_memDBClientAsync.del(strKey.c_str());
		//PKLog.LogMessage(PK_LOGLEVEL_INFO, "del pm.%s", strKey.c_str());
	}
	m_memDBClientAsync.commit();
	vecKeys.clear();
#endif
	return 0;
}

int CMainTask::OnStart()
{
	int nRet = PK_SUCCESS;
	long lRet = m_cvProcMgr.InitProcessMgr();
	if (lRet != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_CRITICAL, ("Failed to initialize process manager, error code %d"), lRet);
		return lRet;
	}

	printf("+============================================================+\n");
	// x先启动过程数据库
	m_cvProcMgr.RestartPdb(); // 重启pdb，且pdb停止中不需要更新状态

	ACE_OS::sleep(1);	// 等待内存数据库启动完毕先！

	if (!m_cvProcMgr.IsGeneralPM())
	{
		int nPKMemDBListenPort = PKMEMDB_LISTEN_PORT;
		string strPKMemDBPassword = PKMEMDB_ACCESS_PASSWORD;
		GetPKMemDBPort(nPKMemDBListenPort, strPKMemDBPassword);

		nRet = m_memDBClientAsync.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str(), 0);
		if (nRet != PK_SUCCESS)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "procmgr Main Task, Initialize RW failed, ret:%d", nRet);
		}

		nRet = m_memDBSubSync.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str());
		if (nRet != PK_SUCCESS)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "procmgr Main Task, Initialize Sub failed, ret:%d", nRet);
		}
		m_memDBSubSync.subscribe(CHANNELNAME_PM_CONTROL);// , OnChannel_PMControlMsg, this);

		// 删除所有pm:的tag点
		InitAndClearPMTags();

		m_cvProcMgr.InitLogConfTagsInPdb();
	}

	// 再启动所有其他的进程
	m_cvProcMgr.StopAllProcess(true,false, false, false); // 先停止再重启

	printf("+============================================================+\n\n");
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "已经重启所有可重启的进程!");
	printf("+============================================================+\n\n");
	return nRet;
}

// 本任务停止时
void CMainTask::OnStop()
{
	m_cvProcMgr.StopAllProcess(false,true, false, true);
	m_cvProcMgr.DelManageTagsInPdb(); // 删除管理tag点
#ifndef DSIABLE_PM_REMOTE_API
	m_memDBSubSync.finalize();
	m_memDBClientAsync.finalize();
#endif
}

int CMainTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CMainTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();
}


// 处理远程调用的命令。包括：启停所有驱动、启停某个驱动...
// 格式：{"name":"pm.服务英文名称","value":"0/1/2/10/11/12"}, 0表示停止，1表示启动，2表示重启;或者start、stop、restart、startall、stopall、restartall、
int CMainTask::OnRemoteCmd(string &strCtrlCmd, Json::Value &jsonRoot)
{
	int nRet = PK_SUCCESS;
	vector<Json::Value *> vectorCmds;
	if (jsonRoot.isObject())
	{
		vectorCmds.push_back(&jsonRoot);
	}
	else if(jsonRoot.isArray())
	{
		for(unsigned int i = 0; i < jsonRoot.size(); i ++)
			vectorCmds.push_back(&jsonRoot[i]);
	}
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "pkServerManager,ctrls(%s),format error，应为json array or object：{\"name\":\"processname\",\"value\":\"start\"}",strCtrlCmd.c_str());
		return -12;
	}

	for(vector<Json::Value *>::iterator itCmd = vectorCmds.begin(); itCmd != vectorCmds.end(); itCmd ++)
	{
		Json::Value &jsonCmd = **itCmd;
		Json::Value jsonCommand = jsonCmd["command"]; // tag值，0,1,2,或者start、stop、restart、startall、stopall、restartall
		Json::Value jsonProcess = jsonCmd["process"]; // pm:服务英文名称.也可能是进程号（显示隐藏时为序号！）
		Json::Value jsonParam = jsonCmd["param"]; // 停止进程时的超时时间。其他命令则没有用处
		Json::Value jsonUserName = jsonCmd["username"];

		if (jsonCommand.isNull() || jsonProcess.isNull()){
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "pkServerManager, command and process attributes must exist!");
			nRet = -2;
			continue;
		}

		string strCommand = jsonCommand.asString();
		string strProcess = jsonProcess.asString();
		string strParam = jsonParam.asString(); // 停止进程时的超时时间。其他命令则没有用处
		string strUserName = "未知用户";
		if(!jsonUserName.isNull())
			strUserName = jsonUserName.asString();
        std::transform(strCommand.begin(), strCommand.end(), strCommand.begin(), ::tolower); // 变为小写

		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "pkServerManager recv command: %s", strCtrlCmd.c_str());
		if(PKStringHelper::StriCmp(strCommand.c_str(), PMCONTROL_PROCESS_CMD_PRINTLIC) == 0)
		{
			m_cvProcMgr.PrintLicenseState();
			continue;
		}
		else if(PKStringHelper::StriCmp(strCommand.c_str(), PMCONTROL_PROCESS_CMD_PRINTPROC) == 0)
		{
			m_cvProcMgr.PrintAllProcessState();
			continue;
		}
		else if(PKStringHelper::StriCmp(strCommand.c_str(), PMCONTROL_PROCESS_CMD_SHOWHIDE) == 0)
		{
			int nProcNo = ::atoi(strProcess.c_str()); // 进程号
			m_cvProcMgr.ToggleShow(nProcNo);
			continue;
		}

		//  下面进行进程启停判断
		if(strCommand.compare(PMCONTROL_PROCESS_CMD_START) == 0 || strCommand.compare(PMCONTROL_PROCESS_CMD_STOP) == 0 || 
			strCommand.compare(PMCONTROL_PROCESS_CMD_RESTART) == 0 || strCommand.compare(PMCONTROL_PROCESS_CMD_RESRESH) == 0)
		{
			// 判断是不是对所有的程序进行控制（不包含tomcat自身）
			if(PKStringHelper::StriCmp(strProcess.c_str(), "all") == 0 || PKStringHelper::StriCmp(strProcess.c_str(), "all") == 0)
			{
				if(strCommand.compare(PMCONTROL_PROCESS_CMD_START) == 0)
				{
					m_cvProcMgr.StartAllProcess(true);
					continue;
				}
				else if(strCommand.compare(PMCONTROL_PROCESS_CMD_STOP) == 0)
				{
					m_cvProcMgr.StopAllProcess(false, false, true, true);
					continue;
				}
				else if(strCommand.compare(PMCONTROL_PROCESS_CMD_RESTART) == 0)
				{
					m_cvProcMgr.StopAllProcess(true, false, false, true);
					continue;
				}
				else if(strCommand.compare(PMCONTROL_PROCESS_CMD_RESRESH) == 0)
				{
                    m_cvProcMgr.NotifyAllProcessRefresh(false);
					continue;
				}
			}


			// 找到进程名
			string strProcessName;
			int nPosComma = strProcess.find_first_of(':');
			if(nPosComma >= 0)
				strProcessName = strProcess.substr(nPosComma + 1);
			else
				strProcessName = strProcess;

			CPKProcess *pProcess = m_cvProcMgr.FindProcessByName(strProcessName.c_str());
			if(!pProcess)
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "pkServerManager根据名称找不到进程：%s, 无法执行命令:%s", strProcessName.c_str(), strCtrlCmd.c_str());
				continue;
			}

			// 执行单个进程的任务
			if(strCommand.compare(PMCONTROL_PROCESS_CMD_START) == 0)
				pProcess->StartProcess();
			else if(strCommand.compare(PMCONTROL_PROCESS_CMD_STOP) == 0)
			{
				int nWaitSeconds = ::atoi(strParam.c_str());
				if(nWaitSeconds <= 0)
					nWaitSeconds = 10; // 缺省等待10秒
				pProcess->ManualStopProcess(nWaitSeconds); // 手工停止，且禁止启动, 并且
			}
			else if(strCommand.compare(PMCONTROL_PROCESS_CMD_RESTART) == 0)
				pProcess->RestartProcess();
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "pkServerManager已执行进程:%s的%s命令", strProcessName.c_str(), strCtrlCmd.c_str());
		}
	}

	return 0;
}

// 发送命令给MainTask执行
// 在主线程main.cpp中调用
int CMainTask::SignalCmd2MainTask(char *szCmdName, char *szProcess)
{
	int nRet = PK_SUCCESS;
#ifndef DSIABLE_PM_REMOTE_API
	char szCmdBuff[PK_TAGDATA_MAXLEN] = {0};
	PKStringHelper::Snprintf(szCmdBuff, sizeof(szCmdBuff), "{\"command\":\"%s\",\"process\":\"%s\"}", szCmdName, szProcess);

	nRet = m_memDBClientAsync.publish(CHANNELNAME_PM_CONTROL, szCmdBuff);
	if(nRet != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "SignalCmd2MainTask Task, PubMsg to:%s, content:%s failed, ret:%d", CHANNELNAME_PM_CONTROL, szCmdBuff, nRet);
		//redisPub.finalize();
		return -2;
	}
	m_memDBClientAsync.commit();
	//PKLog.LogMessage(PK_LOGLEVEL_INFO, "发送通知成功SignalCmd2MainTask Task, PubMsg to:%s, content:%s success",CHANNELNAME_PM_CONTROL, szCmd);
	//redisPub.finalize();
#endif
	return nRet;
}

int CMainTask::Signal2StopAllProcess( bool bRestartProcesses )
{
	if(bRestartProcesses)
		return SignalCmd2MainTask(PMCONTROL_PROCESS_CMD_RESTART, "all");
	else
		return SignalCmd2MainTask(PMCONTROL_PROCESS_CMD_STOP, "all");
}

int CMainTask::Signal2NotifyAllProcessRefresh()
{
	return SignalCmd2MainTask(PMCONTROL_PROCESS_CMD_RESTART, "all");
}

int CMainTask::Signal2PrintAllProcessState()
{
	return SignalCmd2MainTask(PMCONTROL_PROCESS_CMD_PRINTPROC, "all");
}

int CMainTask::Signal2PrintLicenseState()
{
	return SignalCmd2MainTask(PMCONTROL_PROCESS_CMD_PRINTLIC, "all");
}

int CMainTask::Signal2ToggleProcessShowHide(int nProcNo)
{
	char szProcNo[32] = {0};
	sprintf(szProcNo, "%d", nProcNo);
	return SignalCmd2MainTask(PMCONTROL_PROCESS_CMD_SHOWHIDE, szProcNo);
}
