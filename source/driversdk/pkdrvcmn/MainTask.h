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
#include "drvframework.h"
#include "tinyxml2/tinyxml2.h"
#include "Driver.h"
#include "json/json.h"
#include "redisproxy/redisproxy.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include <map>
using namespace  std;

extern int PublishKVSimple2NodeServer(int nActionType, const char *szKey, char *szValue, int nValueLen);

class CDrvTag
{
public:
	string	m_strTagName;
	string	m_strAddress;
	string	m_strDataType;
	string	m_strDataLen;
	string	m_strPollRateMS;
	string	m_strByteOrder;
	string	m_strDesc;
	string	m_strParam;
	CDrvTag()
	{
		m_strTagName = m_strAddress = m_strDataType = "";
		m_strDataLen = "";
		m_strPollRateMS = "";
		m_strByteOrder = "";
		m_strDesc = m_strParam = "";
	}
};

class CTaskGroup;
class CDevice;
class CMainTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();

	int Start();
	void Stop();

	int ModifyTaskGrps(map<string, CTaskGroup*> &mapModifyTaskGrp);
	void ModifyDevices(map<string, CDevice*> &mapCurDevices, map<string, CDevice*> &mapModifyDevices);
	
	// 当前运行的设备信息
	CDriver		m_driverInfo;
	int			m_nTagNum; // 该驱动的tag点个数
	int			m_nDeviceNum; // 该驱动的设备数量
	void TriggerRefreshConfig();
protected:
	bool m_bStop;
	bool m_bStopped;
	bool m_bManualRefreshConfig;	// 手工触发在线配置一次

	int CalcDriverTagDataSize(unsigned int *pnTagCount, unsigned int *pnTagDataSize, unsigned int *pnTagMaxDataSize); // 计算驱动的tag点的大小

	unsigned int	m_nQueRecordLen;
	char *			m_pQueRecordBuffer;
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
	int ConvertConfigFromDbToXml(char *szDriverName, char *szCfgPathFileName);

	// 加载某个设备下的所有数据块到pDevice中
	int LoadTagsOfDevice(CDriver *pDriver, CDevice* pDevice, tinyxml2::XMLElement* pNodeDevice);

	// 加载某个设备组下的所有设备信息到pTaskGrp
	int LoadDevicesOfGroup(CDriver *pDriver, CTaskGroup* pTaskGrp, tinyxml2::XMLElement* pNodeGrpDevice);

	// 加载某个设备信息到pDevice
	int LoadDeviceInfo(CDriver *pDriver, CTaskGroup* pTaskGrp, tinyxml2::XMLElement* pNodeDevice);

	// 在线重新加载配置
	void HandleOnlineConfig();

	int DelTaskGrps(map<string, CTaskGroup*> &mapDelTaskGrp);
	bool IsDeviceConfigChanged(CDevice* pCurActiveDev, CDevice* pNewDev);
	CTaskGroup * AssureGetTaskGroup(CDriver *pDriver, string strTaskGrpName);
	CDevice * AssureGetDevice(CDriver *pDriver, CTaskGroup *pTaskGroup, string strDeviceName, CDevice *pDevice);
	bool GetTaskGroupAndDeviceByTagName(string &strTagName, CTaskGroup *&pTaskGroup, CDevice *&pDevice, PKTAG *&pTag);

	int InitShmQueues();

	int InitQueWithNodeServer();
	int UnInitProcQues();
	int Publish2NodeServer(char *szContent);
	int ReadTagsInfoFromDb(CPKEviewDbApi &PKEviewDbApi, string strDevId, vector<CDrvTag *> &vecTags);
};

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _MAIN_TASK_H_
