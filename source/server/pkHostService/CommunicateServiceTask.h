#ifndef _COMMUNICATE_SERVICE_TASK_H_
#define _COMMUNICATE_SERVICE_TASK_H_


#include <ace/Singleton.h>
#include <ace/Null_Mutex.h>
#include <ace/Task.h>

#include "DataAcquisition.h"
#include "pksock/pksockserver.h"

#include <vector>
#include <map>

using namespace std;

#define NODE_LISTENPORT				"ListenPort"
#define NODE_REFRESHRATE			"RefreshRate"

#define TOTAL_SECONDS_M1			(1 * 60) 
#define TOTAL_SECONDS_M10			(1 * 60 * 10) 
#define TOTAL_SECONDS_M30			(1 * 60 * 30)
#define TOTAL_SECONDS_M60			(1 * 60 * 60)
#define TOTAL_SECONDS_H3			(1 * 60 * 60 * 3)
#define TOTAL_SECONDS_H6			(1 * 60 * 60 * 6)
#define TOTAL_SECONDS_H12			(1 * 60 * 60 * 12)
#define TOTAL_SECONDS_H24			(1 * 60 * 60 * 24)

// 从头部得到长度。同时校验合法性
//extern int GetMsgLength(char *pHeader, int nLen);

class CommunicateServiceTask : public ACE_Task<ACE_NULL_SYNCH>
{
friend class ACE_Singleton<CommunicateServiceTask, ACE_Null_Mutex>;
public:
	CommunicateServiceTask(void);
	virtual ~CommunicateServiceTask(void); 
		
public:
	static int		OnTimeOutDo(void* pContext);

public:
	int				StartTask();
	int				StopTask();
	virtual int		svc();
	 
protected:
	int				OnStart();
	int				OnStop();

protected:
	int				Receive4AndSend2Client();
	int				ReadConfig(const char* szFilePath);

private:
	int				FillCpuData(unsigned int nTypeValue, char* &szFillData);
	int				FillMemoryData(unsigned int nTypeValue, char* &szFillData);
	int				FillDriveData(unsigned int nTypeValue, char* &szFillData, char* szParameter);
	int				FillProcessData(char* &szFillData, char *szParameter);
	int				ContorlRunTheFile(char* szReceive);
	int				ContorlProcessKill(char* szReceive);

private:
	int				LengthResolve(char* szReceive, unsigned int &nLength); 
	int				ProtocolCodeResolve(char* szReceive, unsigned short &nProcotolCode);
	int				TypeCountResolve(char* szReceive, unsigned short &nTypeCount);
	int				TypeResolve(char* szReceive, unsigned short &nType);
	int				TypeValueResolve(char* szReceive, unsigned int &nTypeValue);
	int				ParameterLengthResolve(char* szReceive, unsigned int &nParameterLength);
	int				ParameterResolve(char* szReceive, unsigned int nParameterLength, char* &szParameter);

private:
	int				CopyTypeCount(char* &szFillData, unsigned short nTypeCount);
	int				CopyProtocolCode(char* &szFillData, unsigned short nProtocolCode);
	int				CopyTypeValue(char* &szFillData, unsigned short nType, unsigned int nTypeValue);
	int				CopyParameter(char* &szFillData, unsigned int nParameterLength, const char* szParameter);
	
private:
	int				AverageDataCalculator(vector<double> &vDataSet, int nTotalSeconds, double &nResult);
	int				AverageDataCalculator(vector<ULLong> &vDataSet, int nTotalSeconds, ULLong &nResult);
	
private:
	static int		DataSetGuarder(vector<double>* vDataSet);
	static int		DataSetGuarder(vector<ULLong>* vDataSet);
	static int		CpuGatheringData();
	static int		MemoryGatheringData();
	static int		DriveLettersGatheringData();

private:	
	int				m_nListenPort;
	bool			m_bStop; 
	 
private:
	static int					m_nRefreshCount;
	static int					m_nRefreshRate;//3s
	static ULLong				m_nMemoryTotal;
	static DataAcquisition		m_DAQ; 
	
private:
	static vector<double>*							m_vpRecord_CPU;
	static vector<ULLong>*							m_vpRecord_Memory_Free;
	static map<const char*, vector<ULLong>*>*		m_vpRecord_DriveLetters_Free;
	static map<const char*, ULLong>*				m_vpDriveLetters_Total;

};
typedef ACE_Singleton<CommunicateServiceTask, ACE_Null_Mutex> CommunicateServiceTaskSingleton;
#define COMMUNICATESERVICE_TASK CommunicateServiceTaskSingleton::instance()
 
#endif //_COMMUNICATE_SERVICE_TASK_H_
