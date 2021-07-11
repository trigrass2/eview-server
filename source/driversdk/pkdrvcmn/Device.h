/**************************************************************
 *  Filename:    Device.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 设备信息实体类.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
**************************************************************/

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "drvframework.h"
#include "UserTimer.h"
#include "PythonHelper.h"
#include "pkDriverCommon.h"
#include "pkcommunicate/pkcommunicate.h"

#include <vector>
#include <map>
#include <string>

#define CONN_STATUS_SUCCESS			0		// 和设备连接正常
#define CONN_STATUS_FAIL			1		// 和设备连接失败
#define CONN_STATUS_INIT_CODE		2		// 初始化尚未连接

#define DRIVER_STATUS_INIT_CODE		2		// 驱动连接状态
#define TASK_STATUS_INIT_CODE		2		// 任务连接状态

#define STATUS_INIT_TEXT			"init"	

using namespace std;

class CTaskGroup;
class CDevice: public CDrvObjectBase,public ACE_Event_Handler
{
public:
	PKCONNECTHANDLE		m_hPKCommunicate;	// 设备连接句柄
	int					m_nConnectType;		// 连接类型
	int					m_nPollRate;	// 轮询时间
	string				m_strConnType;	// 连接类型
	string				m_strConnParam;	// 连接参数
	int					m_nRecvTimeOut;	// 毫秒为单位
	int					m_nConnTimeOut; // 连接超时，毫秒为单位
	bool				m_bMultiLink;	// 设备是否支持多连接
	string				m_strTaskID;	// 任务号
	bool				m_bDeviceBigEndian;	// 是否是大端，缺省为小端（intel 系列，如三菱PLC），西门子则为大端顺序

public:
	CDevice(CTaskGroup *pTaskGrp, const char *pszDeviceName);
	virtual ~CDevice();

	CDevice &operator=(CDevice &theDevice);
	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);	// collect data
	void Start();
	void StartCheckConnectTimer();
	void DestroyTimers();
	void Stop();
    int SendToDevice(const char *szBuffer, int lBufLen, int lTimeOutMS );
	int RecvFromDevice(char *szBuff, int lBufLen, int lTimeOutMS); 

	void PostWriteCmdToTask(string strDataBlkAddr, int nDataEguType, char *szCmdData, int nCmdDataLen);
	void OnWriteCommand(PKTAG *pTag, PKTAGDATA *pTagData);
	void ClearRecvBuffer();
	int GetMultiLink(char *szConnParam);

	int GetCachedTagData(PKTAG *pTag, char *szValue, int nValueBufLen, int *pnRetValueLen, unsigned int *pnTimeSec, unsigned short *pnTimeMilSec, int *pnQuality);
	CUserTimer * CreateAndStartTimer( PKTIMER * timerInfo );
	PKTIMER * GetTimers( int * pnTimerNum );
	int StopAndDestroyTimer(CUserTimer * pTimer );
	int UpdateTagsData(PKTAG **pTags, int nTagNum);
	int AddTags2InitDataShm();

	int DisconnectFromDevice();
	int ReConnectDevice(int nTimeOutMS);
	int CheckAndConnect(int nConnectTimeOutMS);
public:
	void CopyDeviceInfo2OutDevice();

	void CallInitDeviceFunc();
	int InitConnectToDevice();
	char * GetTagTimeString(PKTAG *pTag);
	void processByteOrder(PKTAG *pTag, char *szBinData, int nBinLen);
	int InitShmDeviceInfo();
	int GetTagDataBuffer(PKTAG *pTag, char *szTagBuffer, int nBufLen, unsigned int nCurSec, unsigned int nCurMSec);
	int GetTagsByAddr(const char * szTagAddress, PKTAG **pTags, int nMaxTagNum);

	int SetConnectOKTimeout(int nConnStatusTimeoutMS);
	int m_nConnStatusTimeoutMS;
	ACE_Time_Value m_tvLastConnOK;
	void SetConnectOK(); // 收到数据时调用1次该接口
public:
	int SetDeviceConnected(bool bDevConnected);
	bool m_bLastConnStatus;
	time_t	m_tmLastDisconnect;
	unsigned int m_nDisconnectConfirmSeconds; // 最大的连接断开判断时间

	PKTAG *m_pTagEnableConnect;	// 是否允许连接的内置tag点
	PKTAG *m_pTagConnStatus;	// 设备连接状态的内置点
	bool GetEnableConnect();	// 返回是否允许连接
	void SetEnableConnect(bool bEnableConnect);	// 设置是否允许连接
public:
	PKDEVICE	*m_pOutDeviceInfo;	// 用于具体设备驱动的配置信息

	CTaskGroup *m_pTaskGroup;			// 所属设备组指针
	map<string, string>	m_mapDeviceIPsOfTcpServer;	// tcp连接时的设备IP，用于判断是否允许该IP连接

	vector<CUserTimer*> m_vecTimers;	// 定时器对象数组，可用于数据块、设备本身
	PKTIMER	*			m_pTimers;
	int					m_nTimerNum;
	ACE_Time_Value		m_tvTimerBegin;

	long				m_nConnCount;	// 连接次数
	char				m_szLastConnTime[PK_NAME_MAXLEN];	// 上次连接时间
	vector<PKTAG *>		m_vecAllTag;
	std::map<string, PKTAG *>		m_mapName2Tags;	// 根据tag名查找tag对象，以便在控制时可以快速检索到tag信息
	std::map<string, PKTAGDATA *>	m_mapName2CachedTagsData;
	std::map<string, vector<PKTAG*> *> m_mapAddr2TagVec;	// 将地址映射到变量数组，便于根据地址快速查找到变量！

private:
	//char				m_szTmpTagValue[PK_TAGDATA_MAXLEN]; // 临时变量，避免每次都分配内存的开销
	char				m_szTmpTagTimeWithMS[PK_NAME_MAXLEN]; // 临时变量，避免每次都分配内存的开销

	// python驱动的支持类
public:
#ifdef DRV_SUPPORT_PYTHON
	PyObject	*m_pyDevice;		// 用于驱动的配置信息
#endif
};

#endif  // _DEVICE_H_
