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
	
	// ��ǰ���е��豸��Ϣ
	CDriver		m_driverInfo;
	int			m_nTagNum; // ��������tag�����
	int			m_nDeviceNum; // ���������豸����
	void TriggerRefreshConfig();
protected:
	bool m_bStop;
	bool m_bStopped;
	bool m_bManualRefreshConfig;	// �ֹ�������������һ��

	int CalcDriverTagDataSize(unsigned int *pnTagCount, unsigned int *pnTagDataSize, unsigned int *pnTagMaxDataSize); // ����������tag��Ĵ�С

	unsigned int	m_nQueRecordLen;
	char *			m_pQueRecordBuffer;
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
	int ConvertConfigFromDbToXml(char *szDriverName, char *szCfgPathFileName);

	// ����ĳ���豸�µ��������ݿ鵽pDevice��
	int LoadTagsOfDevice(CDriver *pDriver, CDevice* pDevice, tinyxml2::XMLElement* pNodeDevice);

	// ����ĳ���豸���µ������豸��Ϣ��pTaskGrp
	int LoadDevicesOfGroup(CDriver *pDriver, CTaskGroup* pTaskGrp, tinyxml2::XMLElement* pNodeGrpDevice);

	// ����ĳ���豸��Ϣ��pDevice
	int LoadDeviceInfo(CDriver *pDriver, CTaskGroup* pTaskGrp, tinyxml2::XMLElement* pNodeDevice);

	// �������¼�������
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
