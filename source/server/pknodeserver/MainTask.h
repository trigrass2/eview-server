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
	map<string, CAlarmType *>		m_mapName2AlarmType; // 报警类型的映射

	map<string, CDataTag *>			m_mapName2Tag;		// 所有tag名称到对象的map
	map<int, map<int, CLabel *> >	m_mapId2Label;
	map<string, CAlarmTag *>		m_mapName2AlarmTag; // 仅仅在本文件中使用！;
	
	// 几类普通的变量
	map<string, CDeviceTag *>		m_mapName2DeviceTag;	// 设备数据tag点

	map<string, CCalcTag *>			m_mapName2CalcTag;
	map<string, CCalcTag *>			m_mapName2CalcTagLevel2;	// 计算二级变量，包括：普通二级变量、累计时间点、累计次数点
	map<string, CCalcTag *>			m_mapName2CalcTagLevel3;	// 计算三级变量，包括：普通二级变量、累计时间点、累计次数点
	map<string, CTagAccumulateTime *>			m_mapName2AccumlateTime;	// 计算三级变量，包括：普通二级变量、累计时间点、累计次数点
	map<string, CTagAccumulateCount *>			m_mapName2AccumlateCount;	// 计算三级变量，包括：普通二级变量、累计时间点、累计次数点


	map<string, CMemTag *>			m_mapName2MemTag;	// 所有内存tag名称到对象的map;
	map<string, CSimTag *>			m_mapName2SimTag;	// 所有内存tag名称到对象的map;
	map<string, CPredefTag *>		m_mapName2PredefTag;	// 所有内存tag名称到对象的map;

	CMyDriverSimulate				*m_pSimDriver;	// 模拟变量设备
	CMyDriverMemory					*m_pMemDriver;	// 内存变量设备
	CMySimulateDevice				*m_pSimDevice;	// 模拟变量设备
	CMyMemoryDevice					*m_pMemDevice;	// 内存变量设备

	// CMemDBClientAsync	m_redisRW0;
	CMemDBClientSync	m_redisRW0;
	CMemDBClientAsync	m_redisRW1; 
	CMemDBClientAsync	m_redisRW2;
	
	CAlarmInfo			m_alarmInfo;
	int					m_nVersion; // 模块版本，-1表示无版本控制

	unsigned int		m_nAllTagDataSize; // 所有TAG点占用的总的内存大小（根据各自的数据长度计算）。这个目的是为了NodeServer开辟共享队列大小使用
	unsigned int		m_nNodeServerQueSize;	// 最大的tag点的大小，这是为驱动开辟内存使用
public:
	int Start();
	void Stop();
	
protected:
	bool			m_bStop;
	bool			m_bStopped;
	int				m_nTagNum;

protected:
	virtual int svc();
	// 线程初始化
	int OnStart();
	// 线程中停止
	void OnStop();
	// 从配置文件加载和重新加载所有的配置theTaskGroupList
	int LoadConfig();
	int CalcAllTagDataSize();
	int CalcDriverTagDataSize(CMyDriver *pDriver, unsigned int *pnTagCount, unsigned int *pnTagDataSize, unsigned int *pnTagMaxDataSize);
	int InitMemDBTags();
	void ClearMemDBTags();
	int CreateProcessQueue(); // 创建共享内存队列
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
	int GenerateObjectAndPropAndAlarmsFromClass(CMyObject *pObject); // 加载对象的所有信息
	int GenerateObjectAndPropAndAlarmsFromClass(CMyObject *pObject, CMyClass *pClass); // 根据具体的类生成属性
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
