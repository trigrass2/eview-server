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

#ifndef _CTRL_PROCESS_TASK_H_
#define _CTRL_PROCESS_TASK_H_

#include <ace/Task.h>
#include "drvframework.h"
#include "tinyxml/tinyxml.h"
#include "Driver.h"
#include "json/json.h"
#include "redisproxy/redisproxy.h"
#include <map>
using namespace  std;

#define STATUS_INIT_CODE		10000
#define STATUS_INIT_TEXT		"init"	

extern int PublishTag2PbServer(const char *szTagName, const char *szTagData, int nTagDataLen);

class CTaskGroup;
class CDevice;
class CMainTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();
	// CRedisProxy m_redisSub; // 订阅
	// CRedisProxy m_redisRW;  // 读写

	int Start();
	void Stop();

	int ModifyTaskGrps(map<string, CTaskGroup*> &mapModifyTaskGrp);
	void ModifyDevices(map<string, CDevice*> &mapCurDevices, map<string, CDevice*> &mapModifyDevices);
	// 当前运行的设备信息
	CDriver		m_driverInfo;

	void TriggerRefreshConfig();
protected:
	bool m_bStop;
	bool m_bStopped;
	bool m_bManualRefreshConfig;	// 手工触发在线配置一次
protected:
	virtual int svc();

	// 线程初始化
	int OnStart();

	// 线程中停止
	int OnStop();

	// 处理写控制命令
	int ProcessWriteOneCmd(Json::Value &ctrlInfo);
	int ProcessWriteCmds(string &strCmds, Json::Value &root);


	int LoadConfig(CDriver &drive);
	// 将配置文件格式从sqlite转换为xml格式
	int ConvertConfigFromSqliteToXml(char *szDriverName, char *szCfgPathFileName);

	// 加载某个设备下的所有数据块到pDevice中
	int LoadTagsOfDevice(CDriver *pDriver, CDevice* pDevice, TiXmlElement* pNodeDevice);

	// 加载某个设备组下的所有设备信息到pTaskGrp
	int LoadDevicesOfGroup(CDriver *pDriver, CTaskGroup* pTaskGrp, TiXmlElement* pNodeGrpDevice);

	// 加载某个设备信息到pDevice
	int LoadDeviceInfo(CDriver *pDriver, CTaskGroup* pTaskGrp, TiXmlElement* pNodeDevice);

	// 在线重新加载配置
	void HandleOnlineConfig();

	//根据设备块查询字符串查找到datablock
	int AddTaskGrps(map<string, CTaskGroup*> &mapAddTaskGrp);

	int DelTaskGrps(map<string, CTaskGroup*> &mapDelTaskGrp);
	bool IsDeviceConfigChanged(CDevice* pCurActiveDev, CDevice* pNewDev);
	CTaskGroup * AssureGetTaskGroup(CDriver *pDriver, string strTaskGrpName);
	CDevice * AssureGetDevice(CDriver *pDriver, CTaskGroup *pTaskGroup, string strDeviceName, CDevice *pDevice);
	bool GetTaskGroupAndDeviceByTagName(string &strTagName, CTaskGroup *&pTaskGroup, CDevice *&pDevice, PKTAG *&pTag);

	int InitShmQueues();
	int ClearShmDataOfDriver();
	int InitDriverShmData();
	int InitProcQues();
	int UnInitProcQues();
	int Publish2PbServer(char *szContent);
};

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _CTRL_PROCESS_TASK_H_
