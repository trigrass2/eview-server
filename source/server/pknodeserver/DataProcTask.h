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

#ifndef _RAWDATA_PROC_TASK_H_
#define _RAWDATA_PROC_TASK_H_

#include <ace/Task.h>
#include "pkmemdbapi/pkmemdbapi.h"
#include "shmqueue/ProcessQueue.h"
#include "VarCalculator.h"
#include "EAProcessor.h"
#include "json/json.h"
#include "json/writer.h"
#include <map>
using namespace  std;

//typedef struct _TAGDATA_JSONSTR{
//	string strName;	// TAG点名称
//	string strVTQ;	// TAG点数据、质量
//	string strTime4Debug; // TAG点的时间戳
//}TAGDATA_JSONSTR;

class CDataProcTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CDataProcTask, ACE_Thread_Mutex>;
public:
	CDataProcTask();
	virtual ~CDataProcTask();

	int Start();
	void Stop();

public:
	CEAProcessor	m_eaProcessor;
	Json::FastWriter            m_jsonWriter;

protected:
	bool			m_bStop;
	bool			m_bStopped;

	char *			m_pQueRecBuffer;	// 一次发送多个TAG点的内存指针
	unsigned int	m_nQueRecBufferLen;	// 一次发送多个TAG点的记录内存大小
	char *			m_pQueTagVTQBuffer;	// 一次发送多个TAG点的内存指针
	unsigned int	m_nQueTagVTQBufferLen;	// 一次发送多个TAG点的记录内存大小

	unsigned int	m_nPackNumRecvRecent;
	unsigned int	m_nPackNumRecvFromStart;
	ACE_Time_Value  m_tvRecentRecvPack;		// 上次打印
	ACE_Time_Value  m_tvFirstRecvPack;		// 第一次接收到数据包
	unsigned int	m_nTimeoutPackNumFromStart; // 从程序启动以来超时的包的个数
	unsigned int	m_nTimeoutMaxSecFromStart;	// 从程序启动以来超时的包的最大超时时间
	unsigned int	m_nTimeoutPackNumRecent;	// 最近超时的包的个数
	unsigned int	m_nTimeoutMaxSecRecent;		// 最近超时的包的最大超时时间

	unsigned int	m_nRecvTagCountRecent;	// 一段时间内收到的驱动来的数据总tag数
	unsigned int	m_nValidTagCountRecent;	// 一段时间内收到的驱动来的数据有效tag数
	
	string			m_strMinTagTimestampRecent;	// 最近一段时间的最小时间戳
	string			m_strMaxTagTimestampRecent; // 最近一段时间的最大时间戳
	unsigned int	m_nCommitTagNumRecent;		// 最近一段时间提交的tag数量
	unsigned int	m_nCommitTimesRecent;		// 最近一段时间提交的次数
	string			m_strCommitDetail;			// 每次提交的数量
	ACE_Time_Value	m_tvLastCommitTagData;		// 上次提交tag点到内存数据库的时间
	ACE_Time_Value	m_tvLastCaclTags;			// 上次计算点计算的时间

	ACE_Time_Value	m_tvLastCheckDriverAlive;	// 上次检查驱动是否在运行的时间	
	void CheckDriverAlive(ACE_Time_Value tvNow);
	bool IsDriverAlive(string strDrvName);
	// 更新驱动所有tag点质量为BAD（驱动关闭了）
	int UpdateTagsOfDriverToBad(CMyDriver *pDrivers);
public:
	vector<string> split(string str,string pattern);
	//vector<TAGDATA_JSONSTR>	m_vecTagDataCached;
	vector<string>			m_vecTagNameCached;  // 批量写redis用
	vector<string>			m_vecTagVTQCached;   // 批量写redis用
	//vector<string>			m_vecTagTime4DebugCached;

protected:
	virtual int svc();

	// 线程初始化
	int OnStart();
	// 线程中停止
	void OnStop();
	int ClearMDBDataOfDriver(char *szDriverName);
	int OnConfirmAlarm(char *szTagName, int nAlarmCatNo, int nIndexInCat);
	int ConvertTagData2JsonAndCache(CDataTag *pTag, unsigned int nSec, unsigned short mSec, int nQuality);
	void PrintDataPackStatInfo(ACE_Time_Value &tvNow, ACE_Time_Value &tvPack);
	int	CacheTagsData(Json::Value &jsonListTags, int nPackTimeStampSec, int nPackTimeStampMSec);
	void CheckAndCommitTagData(ACE_Time_Value tvNow );
	void DoCalcTags(ACE_Time_Value tvNow, map<string, CCalcTag*> &mapName2CalcTag);
	void DoCalcTags(ACE_Time_Value tvNow);
};

#define DATAPROC_TASK ACE_Singleton<CDataProcTask, ACE_Thread_Mutex>::instance()

#endif  // _RAWDATA_PROC_TASK_H_
