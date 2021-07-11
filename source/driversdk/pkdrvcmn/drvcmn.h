/**************************************************************
 *  Filename:    CtrlProcessTask.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
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
	// CRedisProxy m_redisSub; // ����
	// CRedisProxy m_redisRW;  // ��д

	int Start();
	void Stop();

	int ModifyTaskGrps(map<string, CTaskGroup*> &mapModifyTaskGrp);
	void ModifyDevices(map<string, CDevice*> &mapCurDevices, map<string, CDevice*> &mapModifyDevices);
	// ��ǰ���е��豸��Ϣ
	CDriver		m_driverInfo;

	void TriggerRefreshConfig();
protected:
	bool m_bStop;
	bool m_bStopped;
	bool m_bManualRefreshConfig;	// �ֹ�������������һ��
protected:
	virtual int svc();

	// �̳߳�ʼ��
	int OnStart();

	// �߳���ֹͣ
	int OnStop();

	// ����д��������
	int ProcessWriteOneCmd(Json::Value &ctrlInfo);
	int ProcessWriteCmds(string &strCmds, Json::Value &root);


	int LoadConfig(CDriver &drive);
	// �������ļ���ʽ��sqliteת��Ϊxml��ʽ
	int ConvertConfigFromSqliteToXml(char *szDriverName, char *szCfgPathFileName);

	// ����ĳ���豸�µ��������ݿ鵽pDevice��
	int LoadTagsOfDevice(CDriver *pDriver, CDevice* pDevice, TiXmlElement* pNodeDevice);

	// ����ĳ���豸���µ������豸��Ϣ��pTaskGrp
	int LoadDevicesOfGroup(CDriver *pDriver, CTaskGroup* pTaskGrp, TiXmlElement* pNodeGrpDevice);

	// ����ĳ���豸��Ϣ��pDevice
	int LoadDeviceInfo(CDriver *pDriver, CTaskGroup* pTaskGrp, TiXmlElement* pNodeDevice);

	// �������¼�������
	void HandleOnlineConfig();

	//�����豸���ѯ�ַ������ҵ�datablock
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
