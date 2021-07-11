/**************************************************************
 *  Filename:    Device.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �豸��Ϣʵ����.
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

#define CONN_STATUS_SUCCESS			0		// ���豸��������
#define CONN_STATUS_FAIL			1		// ���豸����ʧ��
#define CONN_STATUS_INIT_CODE		2		// ��ʼ����δ����

#define DRIVER_STATUS_INIT_CODE		2		// ��������״̬
#define TASK_STATUS_INIT_CODE		2		// ��������״̬

#define STATUS_INIT_TEXT			"init"	

using namespace std;

class CTaskGroup;
class CDevice: public CDrvObjectBase,public ACE_Event_Handler
{
public:
	PKCONNECTHANDLE		m_hPKCommunicate;	// �豸���Ӿ��
	int					m_nConnectType;		// ��������
	int					m_nPollRate;	// ��ѯʱ��
	string				m_strConnType;	// ��������
	string				m_strConnParam;	// ���Ӳ���
	int					m_nRecvTimeOut;	// ����Ϊ��λ
	int					m_nConnTimeOut; // ���ӳ�ʱ������Ϊ��λ
	bool				m_bMultiLink;	// �豸�Ƿ�֧�ֶ�����
	string				m_strTaskID;	// �����
	bool				m_bDeviceBigEndian;	// �Ƿ��Ǵ�ˣ�ȱʡΪС�ˣ�intel ϵ�У�������PLC������������Ϊ���˳��

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
	void SetConnectOK(); // �յ�����ʱ����1�θýӿ�
public:
	int SetDeviceConnected(bool bDevConnected);
	bool m_bLastConnStatus;
	time_t	m_tmLastDisconnect;
	unsigned int m_nDisconnectConfirmSeconds; // �������ӶϿ��ж�ʱ��

	PKTAG *m_pTagEnableConnect;	// �Ƿ��������ӵ�����tag��
	PKTAG *m_pTagConnStatus;	// �豸����״̬�����õ�
	bool GetEnableConnect();	// �����Ƿ���������
	void SetEnableConnect(bool bEnableConnect);	// �����Ƿ���������
public:
	PKDEVICE	*m_pOutDeviceInfo;	// ���ھ����豸������������Ϣ

	CTaskGroup *m_pTaskGroup;			// �����豸��ָ��
	map<string, string>	m_mapDeviceIPsOfTcpServer;	// tcp����ʱ���豸IP�������ж��Ƿ������IP����

	vector<CUserTimer*> m_vecTimers;	// ��ʱ���������飬���������ݿ顢�豸����
	PKTIMER	*			m_pTimers;
	int					m_nTimerNum;
	ACE_Time_Value		m_tvTimerBegin;

	long				m_nConnCount;	// ���Ӵ���
	char				m_szLastConnTime[PK_NAME_MAXLEN];	// �ϴ�����ʱ��
	vector<PKTAG *>		m_vecAllTag;
	std::map<string, PKTAG *>		m_mapName2Tags;	// ����tag������tag�����Ա��ڿ���ʱ���Կ��ټ�����tag��Ϣ
	std::map<string, PKTAGDATA *>	m_mapName2CachedTagsData;
	std::map<string, vector<PKTAG*> *> m_mapAddr2TagVec;	// ����ַӳ�䵽�������飬���ڸ��ݵ�ַ���ٲ��ҵ�������

private:
	//char				m_szTmpTagValue[PK_TAGDATA_MAXLEN]; // ��ʱ����������ÿ�ζ������ڴ�Ŀ���
	char				m_szTmpTagTimeWithMS[PK_NAME_MAXLEN]; // ��ʱ����������ÿ�ζ������ڴ�Ŀ���

	// python������֧����
public:
#ifdef DRV_SUPPORT_PYTHON
	PyObject	*m_pyDevice;		// ����������������Ϣ
#endif
};

#endif  // _DEVICE_H_
