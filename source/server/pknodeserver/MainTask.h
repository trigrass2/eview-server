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
#include "json/json.h"
#include "json/writer.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "NodeCommonDef.h"
#include "PKNodeServer.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include <map>
using namespace  std;
#define VERSION_NODESERVER_INITIAL						0
#define VERSION_NODESERVER_SUPPORT_MULTINODE			1
#define VERSION_NODESERVER_ALARM_HAVE_OBJDESCRIPTION	2
class CMainTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();

	map<string, CMyDriver *>		m_mapName2Driver;
	map<string, CMyDevice *>		m_mapName2Device;
	map<string, CMyClass *>			m_mapName2Class;
	map<string, CMyObject *>		m_mapName2Object;
	map<string, CAlarmType *>		m_mapName2AlarmType; // �������͵�ӳ��

	map<string, CDataTag *>			m_mapName2Tag;		// ����tag���Ƶ������map
	map<int, map<int, CLabel *> >	m_mapId2Label;
	map<string, CAlarmTag *>		m_mapName2AlarmTag; // �����ڱ��ļ���ʹ�ã�;
	
	// ������ͨ�ı���
	map<string, CDeviceTag *>		m_mapName2DeviceTag;	// �豸����tag��

	map<string, CCalcTag *>			m_mapName2CalcTag;
	map<string, CCalcTag *>			m_mapName2CalcTagLevel2;	// ���������������������ͨ�����������ۼ�ʱ��㡢�ۼƴ�����
	map<string, CCalcTag *>			m_mapName2CalcTagLevel3;	// ����������������������ͨ�����������ۼ�ʱ��㡢�ۼƴ�����
	map<string, CTagAccumulateTime *>			m_mapName2AccumlateTime;	// ����������������������ͨ�����������ۼ�ʱ��㡢�ۼƴ�����
	map<string, CTagAccumulateCount *>			m_mapName2AccumlateCount;	// ����������������������ͨ�����������ۼ�ʱ��㡢�ۼƴ�����


	map<string, CMemTag *>			m_mapName2MemTag;	// �����ڴ�tag���Ƶ������map;
	map<string, CSimTag *>			m_mapName2SimTag;	// �����ڴ�tag���Ƶ������map;
	map<string, CPredefTag *>		m_mapName2PredefTag;	// �����ڴ�tag���Ƶ������map;

	CMyDriverSimulate				*m_pSimDriver;	// ģ������豸
	CMyDriverMemory					*m_pMemDriver;	// �ڴ�����豸
	CMySimulateDevice				*m_pSimDevice;	// ģ������豸
	CMyMemoryDevice					*m_pMemDevice;	// �ڴ�����豸

	// CMemDBClientAsync	m_redisRW0;
	CMemDBClientSync	m_redisRW0;
	CMemDBClientAsync	m_redisRW1; 
	CMemDBClientAsync	m_redisRW2;
	
	CAlarmInfo			m_alarmInfo;
	int					m_nVersion; // ģ��汾��-1��ʾ�ް汾����

	unsigned int		m_nAllTagDataSize; // ����TAG��ռ�õ��ܵ��ڴ��С�����ݸ��Ե����ݳ��ȼ��㣩�����Ŀ����Ϊ��NodeServer���ٹ�����д�Сʹ��
	unsigned int		m_nNodeServerQueSize;	// ����tag��Ĵ�С������Ϊ���������ڴ�ʹ��
public:
	int Start();
	void Stop();
	
protected:
	bool			m_bStop;
	bool			m_bStopped;
	int				m_nTagNum;

protected:
	virtual int svc();
	// �̳߳�ʼ��
	int OnStart();
	// �߳���ֹͣ
	void OnStop();
	// �������ļ����غ����¼������е�����theTaskGroupList
	int LoadConfig();
	int CalcAllTagDataSize();
	int CalcDriverTagDataSize(CMyDriver *pDriver, unsigned int *pnTagCount, unsigned int *pnTagDataSize, unsigned int *pnTagMaxDataSize);
	int InitMemDBTags();
	void ClearMemDBTags();
	int CreateProcessQueue(); // ���������ڴ����
	Json::FastWriter  m_jsonWriter;

protected:
	int LoadDriverConfig(CPKEviewDbApi &PKEviewDbApi);
	int LoadDeviceConfig(CPKEviewDbApi &PKEviewDbApi);
	int LoadAlarmTypeConfig(CPKEviewDbApi &PKEviewDbApi);
	int LoadTagConfig(CPKEviewDbApi &PKEviewDbApi);
	int LoadTag_HisDataConfig(CPKEviewDbApi &PKEviewDbApi);
	int LoadLabelConfig(CPKEviewDbApi &PKEviewDbApi);
	int LoadTagAlarmConfig(CPKEviewDbApi &PKEviewDbApi);
	int LoadClassConfig(CPKEviewDbApi &PKEviewDbApi);
	int LoadClassPropConfig(CPKEviewDbApi &PKEviewDbApi);
	int LoadClassAlarmConfig(CPKEviewDbApi &PKEviewDbApi);
	int LoadObjectConfig(CPKEviewDbApi &PKEviewDbApi);
	int LoadObjectPropConfig(CPKEviewDbApi &PKEviewDbApi);
	int GenerateObjectAndPropAndAlarmsFromClass(CMyObject *pObject); // ���ض����������Ϣ
	int GenerateObjectAndPropAndAlarmsFromClass(CMyObject *pObject, CMyClass *pClass); // ���ݾ��������������
	int AddDeviceInternalTag();

	int CreateChannelClassAndObj();
	int AutoCalcLimitsOfTagAlarm();
	int SplitCalcTags();
	string ExtractObjectName(string strTagName);
	int ExtractObjectFromTags();
	int LoadPreviousRealAlarms(CPKEviewDbApi &PKEviewDbApi);
	int ClearPreviousRealAlarms(CPKEviewDbApi &PKEviewDbApi);
	int WriteTagConfigAndInitValToMemDB();
 	int ModifyObjectProp(CMyObjectProp * pObjectProp, string strPropKey, string strPropValue);
	int GenerateTagsFromObjProps();

	int ProcessTagConfig(map<string, CCalcTag *> &mapName2CalcTag);
	int LoadObjectPropAlarmConfig(CPKEviewDbApi &PKEviewDbApi);
	int ModifyObjectPropAlarm(CMyObjectProp * pObjectProp, string strJudgeMethod, string strPropFieldKey, string strPropFieldValue, int &nAddObjProp, int &nModifyObjProp);
	int GenerateTagAlarmsFromObjProp(CMyObjectProp *pObjProp, CDataTag *pDataTag);

	CPredefTag *AddPredefTag(string strTagName, int nTagDataType);
	int AddPredefineTagToConfig();
};

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _MAIN_TASK_H_
