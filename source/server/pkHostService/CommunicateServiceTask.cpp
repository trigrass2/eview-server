#include "CommunicateServiceTask.h"

#include "pklog/pklog.h" 
#include "pkcomm/pkcomm.h"
#include "common/eviewdefine.h"
#include "tinyxml2/tinyxml2.h"
#include <cstring>  
#include <ace/Thread.h>
#include <ace/Synch.h>

using namespace std;

CPKLog g_logger;
ACE_Thread_Mutex g_Mutex_CPU;
ACE_Thread_Mutex g_Mutex_Memory;
ACE_Thread_Mutex g_Mutex_DriveLetter;

int CommunicateServiceTask::m_nRefreshRate = 3;
int CommunicateServiceTask::m_nRefreshCount = 0;
ULLong CommunicateServiceTask::m_nMemoryTotal = 0;
DataAcquisition CommunicateServiceTask::m_DAQ = DataAcquisition();

vector<double>*	CommunicateServiceTask::m_vpRecord_CPU = new vector<double>();
vector<ULLong>*	CommunicateServiceTask::m_vpRecord_Memory_Free = new vector<ULLong>();
map<const char*, ULLong>* CommunicateServiceTask::m_vpDriveLetters_Total = new map<const char*, ULLong>();
map<const char*, vector<ULLong>*>* CommunicateServiceTask::m_vpRecord_DriveLetters_Free = new map<const char*, vector<ULLong>*>();


CommunicateServiceTask::CommunicateServiceTask(void)
	:
	m_bStop(false),
    m_nListenPort(30003)
{
}
CommunicateServiceTask::~CommunicateServiceTask(void)
{
	delete m_vpRecord_CPU;
	delete m_vpRecord_Memory_Free;
	//map<const char*, vector<ULLong>*>::iterator
    for (map<const char*, vector<ULLong>*>::iterator item = m_vpRecord_DriveLetters_Free->begin(); item != m_vpRecord_DriveLetters_Free->end(); item++)
	{
		delete[] item->first;
		delete item->second;
	}
	delete m_vpRecord_DriveLetters_Free;
	delete m_vpDriveLetters_Total;
}

int	PackageTotalLength(char* pHeader, int nLen)
{
	if (nLen != PACKHEADERLENGTH)
		return -1;
	if (*pHeader != HEADER)
		return -2;
	pHeader++;
	int nTotalLength = 0;
	memcpy(&nTotalLength, pHeader, 4);

	if (nTotalLength > MESSAGEMAXLENGGTH)
		return -3;
	return nTotalLength + PACKHEADERLENGTH;
}
// is not called
void ConnectCallBack(HQUEUE hDestQue, int nConnectState)
{
	char szRemoteIP[16];
	unsigned short nRemotePort;
	SOCKSVR_GetQueueAddr(hDestQue, szRemoteIP, sizeof(szRemoteIP), &nRemotePort);

	int nLocalQueID = -1, nRemoteQueID = -1;
	switch (nConnectState)
	{
	case 1:
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "SOCKSVR_CONNECTED(RemoteQue: %d, RemoteIP:%s:%d).\n", nRemoteQueID, szRemoteIP, nRemotePort);
		break;
	case 0:
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "SOCKSVR_DISCONNECTED(LocalQue: %d, RemoteIP:%s:%d).\n", nLocalQueID, szRemoteIP, nRemotePort);
		break;
	default:
		break;
	}
}

/*---Tool Start---*/
int CommunicateServiceTask::AverageDataCalculator(vector<double> &vDataSet, int nTotalSeconds, double &nResult)
{
	nResult = 0;
	int nAvgCount = nTotalSeconds / m_nRefreshRate;
	int size = vDataSet.size();
	nAvgCount = nAvgCount > size ? size : nAvgCount;

	if (nAvgCount == 0)
		return -1;

	double sum = 0;
	vector<double>::iterator item = vDataSet.begin() + (size - nAvgCount);
	while (item != vDataSet.end())
		sum += *(item++);

	nResult = sum / (nAvgCount * 1.0);
	return 0;
}

int CommunicateServiceTask::AverageDataCalculator(vector<ULLong> &vDataSet, int nTotalSeconds, ULLong &nResult)
{
	nResult = 0;
	int nAvgCount = nTotalSeconds / m_nRefreshRate;
	int size = vDataSet.size();
	nAvgCount = nAvgCount > size ? size : nAvgCount;

	if (nAvgCount == 0)
		return -1;

	ULLong sum = 0;
	vector<ULLong>::iterator item = vDataSet.begin() + (size - nAvgCount);
	while (item != vDataSet.end())
		sum += *(item++);

	nResult = sum / nAvgCount;
	return 0;
}

int CommunicateServiceTask::DataSetGuarder(vector<double>* vDataSet)
{
	if ((int)vDataSet->size() > (int)(TOTAL_SECONDS_H24 / m_nRefreshRate))
		vDataSet->erase(vDataSet->begin());
	return 0;
}

int CommunicateServiceTask::DataSetGuarder(vector<ULLong>* vDataSet)
{
	if ((int)vDataSet->size() > (int)(TOTAL_SECONDS_H24 / m_nRefreshRate))
		vDataSet->erase(vDataSet->begin());
	return 0;
}
/*--- Tool End ---*/

/*---Gathering Start---*/
int CommunicateServiceTask::CpuGatheringData()
{
	vector<double> vCPU;
	m_DAQ.CpuUsageOfCores(vCPU);

	g_Mutex_CPU.acquire();
	DataSetGuarder(m_vpRecord_CPU);
	m_vpRecord_CPU->push_back(vCPU.front());
	g_Mutex_CPU.release();
	return 0;
}

int CommunicateServiceTask::MemoryGatheringData()
{
	if (m_nMemoryTotal == 0)
	{
		ULLong nTotal = 0;
		m_DAQ.PhysicalMemory(nTotal);
		m_nMemoryTotal = nTotal;
	}
	ULLong nFree = 0;
	m_DAQ.PhysicalMemoryFree(nFree);

	g_Mutex_Memory.acquire();
	DataSetGuarder(m_vpRecord_Memory_Free);
	m_vpRecord_Memory_Free->push_back(nFree);
	g_Mutex_Memory.release();
	return 0;
}

int CommunicateServiceTask::DriveLettersGatheringData()
{
	g_Mutex_DriveLetter.acquire();
	vector<CapacityInfo*> vCapacityInfo;
	//map<const char*, vector<ULLong>*>::iterator
    for (map<const char*, vector<ULLong>*>::iterator item = m_vpRecord_DriveLetters_Free->begin(); item != m_vpRecord_DriveLetters_Free->end(); item++)
	{
		DataSetGuarder(item->second);
		m_DAQ.TheDriveLetterCapacityInfo(vCapacityInfo, item->first);

		if (vCapacityInfo.empty())
			continue;

		item->second->push_back(vCapacityInfo.front()->m_nFree);
		/*
		auto st = m_vpRecord_DriveLetters_Free->find(item->first);
		if (st != m_vpRecord_DriveLetters_Free->end())
			g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "auto m_vpRecord_DriveLetters_Free->find(%s) Error...", item->first);
		else
			g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "auto m_vpRecord_DriveLetters_Free->find(%s) Success.", item->first);
		*/
		bool isExistDriveLetterTotal = false;
		//map<const char*, ULLong>::iterator
        for (map<const char*, ULLong>::iterator search = m_vpDriveLetters_Total->begin(); search != m_vpDriveLetters_Total->end(); search++)
		{
			if (strcmp(item->first, search->first) == 0)
			{
				isExistDriveLetterTotal = true;
				break;
			}
		}
		//插入的是字符串的引用吗？。。。
		if (!isExistDriveLetterTotal)
			m_vpDriveLetters_Total->insert(pair<const char*, ULLong>(item->first, vCapacityInfo.front()->m_nTotal)); 

		//earse 的删除等同于 delete 吗？
        for (vector<CapacityInfo*>::iterator item = vCapacityInfo.begin(); item != vCapacityInfo.end(); item++)
			delete *item; 
	}
	g_Mutex_DriveLetter.release();
	return 0;
}

int CommunicateServiceTask::OnTimeOutDo(void* pContext)
{
	g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "Gathering ...");
	//cpu
	CpuGatheringData();
	//memory
	MemoryGatheringData();
	//drive-letter
	m_nRefreshCount++;
	if ((m_nRefreshCount % 10) == 1)
	{
		DriveLettersGatheringData();
		m_nRefreshCount = 1;
	}
	g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "Gathering Done.");
	return 0;
}
/*---Gathering End---*/

/*---Copy Data Start---*/
int	CommunicateServiceTask::CopyParameter(char* &szFillData, unsigned int nParameterLength, const char* szParameter)
{
	char *szTmp = szFillData;
	//ParameterLength 
	memcpy(szTmp, &nParameterLength, 4);
	szTmp += 4;
	//Parameter
	int nLength = strlen(szParameter);
	strncpy(szTmp, szParameter, nLength);
	szTmp += nLength;

	return szTmp - szFillData;
}

int	CommunicateServiceTask::CopyTypeValue(char* &szFillData, unsigned short nType, unsigned int nTypeValue)
{
	char *szTmp = szFillData;
	//Type 
	memcpy(szTmp, &nType, 2);
	szTmp += 2;
	//TypeValue 
	memcpy(szTmp, &nTypeValue, 4);
	szTmp += 4;

	return szTmp - szFillData;
}

int	CommunicateServiceTask::CopyProtocolCode(char* &szFillData, unsigned short nProtocolCode)
{
	char *szTmp = szFillData;
	//ProtocolCode 
	memcpy(szTmp, &nProtocolCode, 2);
	szTmp += 2;

	return szTmp - szFillData;
}

int	CommunicateServiceTask::CopyTypeCount(char* &szFillData, unsigned short nTypeCount)
{
	char *szTmp = szFillData;
	//TypeCount 
	memcpy(szTmp, &nTypeCount, 2);
	szTmp += 2;

	return szTmp - szFillData;
}
/*---Copy Data End---*/

/*---Response Start---*/
int CommunicateServiceTask::ContorlProcessKill(char* szReceive)
{
	char *szTmp = szReceive;

	unsigned int nParameterLength = 0;
	szTmp += ParameterLengthResolve(szTmp, nParameterLength);

	char *szPid = new char[nParameterLength + 1]();
	PKStringHelper::Safe_StrNCpy(szPid, szTmp, nParameterLength + 1);
	szTmp += nParameterLength;
	
	int nPid = 0;
	sscanf(szPid, "%d", &nPid);
    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "ready to Kill Process,Kill Pid=%d...", nPid);
    int nRet = m_DAQ.Kill(nPid);
 
	delete[] szPid;
    return nRet;
}

int CommunicateServiceTask::ContorlRunTheFile(char* szReceive)
{
	char *szTmp = szReceive;

	unsigned int nParameterLength = 0;
	szTmp += ParameterLengthResolve(szTmp, nParameterLength);

	char *szFile = new char[nParameterLength + 1]();
	PKStringHelper::Safe_StrNCpy(szFile, szTmp, nParameterLength + 1);
	szTmp += nParameterLength;

    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Contorl Run File/Command(%s) ...", szFile);
    int nRet = m_DAQ.OpenTheFile(szFile);
 
	delete[] szFile;
    return nRet;
}

int	CommunicateServiceTask::FillProcessData(char* &szFillData, char *szParameter)
{
	unsigned short nStartTime_Year = 0; 
	unsigned short nStartTime_Month = 0; 	
	unsigned short nStartTime_Day = 0;
	unsigned short nStartTime_Hour = 0; 	
	unsigned short nStartTime_Minute = 0;
	unsigned short nStartTime_Second = 0; 	
	unsigned int nPid = 0;
	unsigned long long nRunTime = 0;

	int npid = -1;
	m_DAQ.TheProcessIDByNameAndCmdLine(szParameter, npid); 
	if (npid != -1)
	{
		vector<ProcessInfo*> pi;
        m_DAQ.Processes(pi, npid);
		if (!pi.empty())
		{
			ProcessInfo* item = pi.front();
			nStartTime_Year = item->m_nStartTime_Year;
			nStartTime_Month = item->m_nStartTime_Month;
			nStartTime_Day = item->m_nStartTime_Day;
			nStartTime_Hour = item->m_nStartTime_Hour;
			nStartTime_Minute = item->m_nStartTime_Minute;
			nStartTime_Second = item->m_nStartTime_Second;
            nPid = item->m_nPid;

			//vector<ProcessInfo*>::iterator
            for (vector<ProcessInfo*>::iterator item = pi.begin(); item != pi.end(); item++)
				delete *item; 
		}
	}

	char *szTmp = szFillData;

	memcpy(szTmp, &nStartTime_Year, 2);
	szTmp += 2;
	memcpy(szTmp, &nStartTime_Month, 2);
	szTmp += 2;
	memcpy(szTmp, &nStartTime_Day, 2);
	szTmp += 2;
	memcpy(szTmp, &nStartTime_Hour, 2);
	szTmp += 2;
	memcpy(szTmp, &nStartTime_Minute, 2);
	szTmp += 2;
	memcpy(szTmp, &nStartTime_Second, 2);
	szTmp += 2;

	memcpy(szTmp, &nPid, 4);
	szTmp += 4;

	memcpy(szTmp, &nRunTime, 8);
	szTmp += 8;

	return szTmp - szFillData;
}
//下次请求有有效数据
int	CommunicateServiceTask::FillDriveData(unsigned int nTypeValue, char* &szFillData, char* szParameter)
{  
	g_Mutex_DriveLetter.acquire();
	bool isExistDriveLetter = false;
	//map<const char*, vector<ULLong>*>::iterator
    for (map<const char*, vector<ULLong>*>::iterator search = m_vpRecord_DriveLetters_Free->begin(); search != m_vpRecord_DriveLetters_Free->end(); search++)
	{
		if (strcmp(szParameter, search->first) == 0)
		{
			isExistDriveLetter = true;
			break;
		}
	}
	if (!isExistDriveLetter)
	{
		int nLength = strlen(szParameter);
		char *szDriveLetter = new char[nLength + 1]();
		PKStringHelper::Safe_StrNCpy(szDriveLetter, szParameter, nLength + 1);
		m_vpRecord_DriveLetters_Free->insert(pair<const char*, vector<ULLong>*>(szDriveLetter, new vector<ULLong>()));
	}

	ULLong nTotal = 0;
	/*
	auto st = m_vpDriveLetters_Total->find(szParameter);
	if (st != m_vpDriveLetters_Total->end())
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "auto m_vpDriveLetters_Total->find(%s) Success.", szParameter);
	else
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "auto m_vpDriveLetters_Total->find(%s) Error...", szParameter);
	*/
	//map<const char*, ULLong>::iterator
    for (map<const char*, ULLong>::iterator search = m_vpDriveLetters_Total->begin(); search != m_vpDriveLetters_Total->end(); search++)
	{
		if (strcmp(szParameter, search->first) == 0)
		{
			nTotal = search->second;
			break;
		}
	}

	ULLong nFree = 0;
	/*
	auto st2 = m_vpRecord_DriveLetters_Free->find(szParameter);
	if (st2 != m_vpRecord_DriveLetters_Free->end())
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "auto m_vpRecord_DriveLetters_Free->find(%s) Success.", szParameter);
	else
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "auto m_vpRecord_DriveLetters_Free->find(%s) Error...", szParameter);
	*/
	//map<const char*, vector<ULLong>*>::iterator
    for (map<const char*, vector<ULLong>*>::iterator search = m_vpRecord_DriveLetters_Free->begin(); search != m_vpRecord_DriveLetters_Free->end(); search++)
	{
		if (strcmp(szParameter, search->first) == 0)
		{
			if (!search->second->empty())
				nFree = search->second->back();
			break;
		}
	}
	g_Mutex_DriveLetter.release();

	char *szTmp = szFillData;
	if ((nTypeValue & CALC_PRT_ALL) == CALC_PRT_ALL)
	{
		memcpy(szTmp, &nTotal, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_PRT_V) == CALC_PRT_V)
	{
		memcpy(szTmp, &nFree, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_PRT_VP) == CALC_PRT_VP)
	{
		double nFreePercent = nTotal == 0 ? 0 : (nFree * 1.0) / (nTotal * 1.0) * 100;
		memcpy(szTmp, &nFreePercent, 8);
		szTmp += 8;
	}
	return szTmp - szFillData;
}

int	CommunicateServiceTask::FillMemoryData(unsigned int nTypeValue, char* &szFillData)
{
	g_Mutex_Memory.acquire();
	ULLong nFree = m_vpRecord_Memory_Free->empty() ? 0 : m_vpRecord_Memory_Free->back();
	vector<ULLong> *vpRecord_Memory_Free_Tmp = new vector<ULLong>(m_vpRecord_Memory_Free->begin(), m_vpRecord_Memory_Free->end());
	g_Mutex_Memory.release();

	char *szTmp = szFillData;
	if ((nTypeValue & CALC_PRT_ALL) == CALC_PRT_ALL)
	{
		memcpy(szTmp, &m_nMemoryTotal, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_PRT_V) == CALC_PRT_V)
	{
		memcpy(szTmp, &nFree, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_PRT_VP) == CALC_PRT_VP)
	{
		double nFreePercent = m_nMemoryTotal == 0 ? 0 : (nFree * 1.0) / (m_nMemoryTotal * 1.0) * 100;
		memcpy(szTmp, &nFreePercent, 8);
		szTmp += 8;
	}
	ULLong nAvgFree = 0;
	double nAvgFreePercent = 0;
	if ((nTypeValue & CALC_AVG_V_M1) == CALC_AVG_V_M1)
	{
		AverageDataCalculator(*vpRecord_Memory_Free_Tmp, TOTAL_SECONDS_M1, nAvgFree);
		memcpy(szTmp, &nAvgFree, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_M1) == CALC_AVG_VP_M1)
	{
		nAvgFreePercent = m_nMemoryTotal == 0 ? 0 : (nAvgFree * 1.0) / (m_nMemoryTotal * 1.0) * 100;
		memcpy(szTmp, &nAvgFreePercent, 8);
		szTmp += 8;
	}

	if ((nTypeValue & CALC_AVG_V_M10) == CALC_AVG_V_M10)
	{
		AverageDataCalculator(*vpRecord_Memory_Free_Tmp, TOTAL_SECONDS_M10, nAvgFree);
		memcpy(szTmp, &nAvgFree, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_M10) == CALC_AVG_VP_M10)
	{
		nAvgFreePercent = m_nMemoryTotal == 0 ? 0 : (nAvgFree * 1.0) / (m_nMemoryTotal * 1.0) * 100;
		memcpy(szTmp, &nAvgFreePercent, 8);
		szTmp += 8;
	}

	if ((nTypeValue & CALC_AVG_V_M30) == CALC_AVG_V_M30)
	{
		AverageDataCalculator(*vpRecord_Memory_Free_Tmp, TOTAL_SECONDS_M30, nAvgFree);
		memcpy(szTmp, &nAvgFree, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_M30) == CALC_AVG_VP_M30)
	{
		nAvgFreePercent = m_nMemoryTotal == 0 ? 0 : (nAvgFree * 1.0) / (m_nMemoryTotal * 1.0) * 100;
		memcpy(szTmp, &nAvgFreePercent, 8);
		szTmp += 8;
	}

	if ((nTypeValue & CALC_AVG_V_M60) == CALC_AVG_V_M60)
	{
		AverageDataCalculator(*vpRecord_Memory_Free_Tmp, TOTAL_SECONDS_M60, nAvgFree);
		memcpy(szTmp, &nAvgFree, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_M60) == CALC_AVG_VP_M60)
	{
		nAvgFreePercent = m_nMemoryTotal == 0 ? 0 : (nAvgFree * 1.0) / (m_nMemoryTotal * 1.0) * 100;
		memcpy(szTmp, &nAvgFreePercent, 8);
		szTmp += 8;
	}

	if ((nTypeValue & CALC_AVG_V_H3) == CALC_AVG_V_H3)
	{
		AverageDataCalculator(*vpRecord_Memory_Free_Tmp, TOTAL_SECONDS_H3, nAvgFree);
		memcpy(szTmp, &nAvgFree, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_H3) == CALC_AVG_VP_H3)
	{
		nAvgFreePercent = m_nMemoryTotal == 0 ? 0 : (nAvgFree * 1.0) / (m_nMemoryTotal * 1.0) * 100;
		memcpy(szTmp, &nAvgFreePercent, 8);
		szTmp += 8;
	}

	if ((nTypeValue & CALC_AVG_V_H6) == CALC_AVG_V_H6)
	{
		AverageDataCalculator(*vpRecord_Memory_Free_Tmp, TOTAL_SECONDS_H6, nAvgFree);
		memcpy(szTmp, &nAvgFree, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_H6) == CALC_AVG_VP_H6)
	{
		nAvgFreePercent = m_nMemoryTotal == 0 ? 0 : (nAvgFree * 1.0) / (m_nMemoryTotal * 1.0) * 100;
		memcpy(szTmp, &nAvgFreePercent, 8);
		szTmp += 8;
	}

	if ((nTypeValue & CALC_AVG_V_H12) == CALC_AVG_V_H12)
	{
		AverageDataCalculator(*vpRecord_Memory_Free_Tmp, TOTAL_SECONDS_H12, nAvgFree);
		memcpy(szTmp, &nAvgFree, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_H12) == CALC_AVG_VP_H12)
	{
		nAvgFreePercent = m_nMemoryTotal == 0 ? 0 : (nAvgFree * 1.0) / (m_nMemoryTotal * 1.0) * 100;
		memcpy(szTmp, &nAvgFreePercent, 8);
		szTmp += 8;
	}

	if ((nTypeValue & CALC_AVG_V_H24) == CALC_AVG_V_H24)
	{
		AverageDataCalculator(*vpRecord_Memory_Free_Tmp, TOTAL_SECONDS_H24, nAvgFree);
		memcpy(szTmp, &nAvgFree, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_H24) == CALC_AVG_VP_H24)
	{
		nAvgFreePercent = m_nMemoryTotal == 0 ? 0 : (nAvgFree * 1.0) / (m_nMemoryTotal * 1.0) * 100;
		memcpy(szTmp, &nAvgFreePercent, 8);
		szTmp += 8;
	}
	delete vpRecord_Memory_Free_Tmp;

	return szTmp - szFillData;
}

int	CommunicateServiceTask::FillCpuData(unsigned int nTypeValue, char* &szFillData)
{
	g_Mutex_CPU.acquire();
	double nUtilization = m_vpRecord_CPU->empty() ? 0 : m_vpRecord_CPU->back();
	vector<double> *vpRecord_CPU_Tmp = new vector<double>(m_vpRecord_CPU->begin(), m_vpRecord_CPU->end());
	g_Mutex_CPU.release();

	char *szTmp = szFillData;
	if ((nTypeValue & CALC_PRT_ALL) == CALC_PRT_ALL)
	{
		memcpy(szTmp, &nUtilization, 8);
		szTmp += 8;
	}

	double nAvgUtilization = 0;
	if ((nTypeValue & CALC_AVG_VP_M1) == CALC_AVG_VP_M1)
	{
		AverageDataCalculator(*vpRecord_CPU_Tmp, TOTAL_SECONDS_M1, nAvgUtilization);
		memcpy(szTmp, &nAvgUtilization, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_M10) == CALC_AVG_VP_M10)
	{
		AverageDataCalculator(*vpRecord_CPU_Tmp, TOTAL_SECONDS_M10, nAvgUtilization);
		memcpy(szTmp, &nAvgUtilization, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_M30) == CALC_AVG_VP_M30)
	{
		AverageDataCalculator(*vpRecord_CPU_Tmp, TOTAL_SECONDS_M30, nAvgUtilization);
		memcpy(szTmp, &nAvgUtilization, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_M60) == CALC_AVG_VP_M60)
	{
		AverageDataCalculator(*vpRecord_CPU_Tmp, TOTAL_SECONDS_M60, nAvgUtilization);
		memcpy(szTmp, &nAvgUtilization, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_H3) == CALC_AVG_VP_H3)
	{
		AverageDataCalculator(*vpRecord_CPU_Tmp, TOTAL_SECONDS_H3, nAvgUtilization);
		memcpy(szTmp, &nAvgUtilization, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_H6) == CALC_AVG_VP_H6)
	{
		AverageDataCalculator(*vpRecord_CPU_Tmp, TOTAL_SECONDS_H6, nAvgUtilization);
		memcpy(szTmp, &nAvgUtilization, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_H12) == CALC_AVG_VP_H12)
	{
		AverageDataCalculator(*vpRecord_CPU_Tmp, TOTAL_SECONDS_H12, nAvgUtilization);
		memcpy(szTmp, &nAvgUtilization, 8);
		szTmp += 8;
	}
	if ((nTypeValue & CALC_AVG_VP_H24) == CALC_AVG_VP_H24)
	{
		AverageDataCalculator(*vpRecord_CPU_Tmp, TOTAL_SECONDS_H24, nAvgUtilization);
		memcpy(szTmp, &nAvgUtilization, 8);
		szTmp += 8;
	}
	delete vpRecord_CPU_Tmp;

	return szTmp - szFillData;
}
/*---Response End---*/

/*---Resolve Start---*/
int CommunicateServiceTask::ParameterResolve(char* szReceive, unsigned int nParameterLength, char* &szParameter)
{
	const char *tmp = szReceive;
	szParameter = new char[nParameterLength + 1]();
	PKStringHelper::Safe_StrNCpy(szParameter, tmp, nParameterLength + 1);
	tmp += nParameterLength;
	return tmp - szReceive;
}

int CommunicateServiceTask::ParameterLengthResolve(char* szReceive, unsigned int &nParameterLength)
{
	nParameterLength = 0;
	char *tmp = szReceive;
	memcpy(static_cast<void*>(&nParameterLength), tmp, 4);
	tmp += 4;
	return tmp - szReceive;
}

int CommunicateServiceTask::TypeValueResolve(char* szReceive, unsigned int &nTypeValue)
{
	nTypeValue = 0;
	char *tmp = szReceive;
	memcpy(static_cast<void*>(&nTypeValue), tmp, 4);
	tmp += 4;
	return tmp - szReceive;
}

int	CommunicateServiceTask::TypeResolve(char* szReceive, unsigned short &nType)
{
	nType = 0;
	char *tmp = szReceive;
	memcpy(static_cast<void*>(&nType), tmp, 2);
	tmp += 2;
	return tmp - szReceive;
}

int	CommunicateServiceTask::TypeCountResolve(char* szReceive, unsigned short &nTypeCount)
{
	nTypeCount = 0;
	char *tmp = szReceive;
	memcpy(static_cast<void*>(&nTypeCount), tmp, 2);
	tmp += 2;
	return tmp - szReceive;
}

int	CommunicateServiceTask::ProtocolCodeResolve(char* szReceive, unsigned short &nProcotolCode)
{
	nProcotolCode = 0;
	char *tmp = szReceive;
	memcpy(static_cast<void*>(&nProcotolCode), tmp, 2);
	tmp += 2;
	return tmp - szReceive;
}

int	CommunicateServiceTask::LengthResolve(char* szReceive, unsigned int &nLength)
{
	nLength = 0;
	char *tmp = szReceive;
	memcpy(static_cast<void*>(&nLength), tmp, 4);
	tmp += 4;
	return tmp - szReceive;
}
/*---Resolve End---*/


/*
	头:1 长度:4 协议码:2 类型个数:2 类型a:2 类型a值:4 类型b:2 类型b值:4 ... 类型x:2 类型x值:4 类型x参数长度:4 类型x参数:? ...
	说明：
	1.根据协议码判断解析.
	2.类型值是"位值"看做Int类型，解析时按位做 与"&"运算，从低到高位依次在类型值之后填充请求值，遇到含参数类型，填充值在类型参数值之后.
	*/
int CommunicateServiceTask::Receive4AndSend2Client()
{
	HQUEUE hRemoteQueue = 0;
	char *szReceiveBuf = 0;
	int nReceiveBytes = 0;
	//receive
	int nRet = SOCKSVR_Recv(&hRemoteQueue, &szReceiveBuf, &nReceiveBytes, 3000);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "Received Timeout...");
		return -1;
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "pkHostServer Received (%d) Bytes", nReceiveBytes);
	char *szCleaner = szReceiveBuf;
	
	//Header
	szReceiveBuf++;

	//Length
	unsigned int nLength;
	szReceiveBuf += LengthResolve(szReceiveBuf, nLength);

	//ProtocolCode
	unsigned short nProtocolCode;
	szReceiveBuf += ProtocolCodeResolve(szReceiveBuf, nProtocolCode);

    char szCmdName[64] = {0};
    if (nProtocolCode == PROTOCOL_CODE_CONTROL)
	{
		unsigned short nType = 0;
		szReceiveBuf += TypeResolve(szReceiveBuf, nType);
		if (nType == TYPE_CONTROL_RUN)
		{
            strcpy(szCmdName, "StartProcess");
			ContorlRunTheFile(szReceiveBuf);
		}
		else if(nType == TYPE_CONTROL_KILL)
		{
            strcpy(szCmdName, "KillProcess");
			ContorlProcessKill(szReceiveBuf);
		}
	}
    else // query information
	{
		char *szTmpSend = new char[MESSAGEMAXLENGGTH / 4]();//Enough Room For Fill Data.
		szTmpSend[0] = HEADER;
		char *szFillData = &szTmpSend[PACKHEADERLENGTH];

		szFillData += CopyProtocolCode(szFillData, nProtocolCode);

		//TypeCount
		unsigned short nTypeCount;
		szReceiveBuf += TypeCountResolve(szReceiveBuf, nTypeCount);

		szFillData += CopyTypeCount(szFillData, nTypeCount);

        if (nProtocolCode == PROTOCOL_CODE_CPU_MEM_DISK_INFO)
		{
            strcpy(szCmdName, "CPU&Memory&Disk");
			while (nTypeCount-- > 0)
			{
				unsigned short nType = 0;
				szReceiveBuf += TypeResolve(szReceiveBuf, nType);
				unsigned int nTypeValue = 0;
				szReceiveBuf += TypeValueResolve(szReceiveBuf, nTypeValue);

				szFillData += CopyTypeValue(szFillData, nType, nTypeValue);

				switch (nType)
				{
				case TYPE_CPU:
					szFillData += FillCpuData(nTypeValue, szFillData);
					break;
				case TYPE_MEMORY:
					szFillData += FillMemoryData(nTypeValue, szFillData);
					break;
				case TYPE_DRIVE:
				{
					unsigned int nParameterLength = 0;
					szReceiveBuf += ParameterLengthResolve(szReceiveBuf, nParameterLength);
					char *szParameter = { 0 };
					szReceiveBuf += ParameterResolve(szReceiveBuf, nParameterLength, szParameter);
					szFillData += CopyParameter(szFillData, nParameterLength, szParameter);
					szFillData += FillDriveData(nTypeValue, szFillData, szParameter);
					delete[] szParameter;
				}
				break;
				}

			}
		}
        else if (nProtocolCode == PROTOCOL_CODE_PROCESSINFO)
		{
            strcpy(szCmdName, "ProcessInfo");
			while (nTypeCount-- > 0)
			{
				unsigned int nParameterLength = 0;
				szReceiveBuf += ParameterLengthResolve(szReceiveBuf, nParameterLength);
				char *szParameter = { 0 };
				szReceiveBuf += ParameterResolve(szReceiveBuf, nParameterLength, szParameter);
				szFillData += CopyParameter(szFillData, nParameterLength, szParameter);
				szFillData += FillProcessData(szFillData, szParameter);
				delete[] szParameter;
			}
		}
        else
        {
            sprintf(szCmdName, "UnknownCmd:%d",nProtocolCode);
            g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Recv Unknown Cmd:%d",nProtocolCode);
        }

		//计算长度
		int nTotalLength = (szFillData - szTmpSend);
		int nDataLength = nTotalLength - PACKHEADERLENGTH;
		memcpy(&szTmpSend[1], &nDataLength, 4);

		int nRet = SOCKSVR_Send(hRemoteQueue, szTmpSend, nTotalLength);
		if (nRet != 0)
            g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Recv %s command, Response Length(%d) Failed", szCmdName, nTotalLength);
		else
            g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Recv %s command, Response Length(%d) Success",szCmdName, nTotalLength);

		delete[] szTmpSend;
	}

	//release
	SOCKSVR_FreeMem(szCleaner);
	//lost connection ？
	//SOCKSVR_FreeQueue(hRemoteQueue);
	return nRet;
}

int CommunicateServiceTask::OnStop()
{
	SOCKSVR_Finalize();

	return 0;
}

int CommunicateServiceTask::StopTask()
{
	m_bStop = true;
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "The ServiceMainTask is stopping...");

	int nWaitResult = wait();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "The ServiceMainTask has stopped(%d).", nWaitResult);
	return nWaitResult;
}

int CommunicateServiceTask::svc()
{
	int nRet = 0;
	this->OnStart();

	ACE_Time_Value wait(0, 16000);
	while (!m_bStop)
	{
		nRet = Receive4AndSend2Client();
		ACE_OS::sleep(wait);
	}

	this->OnStop();
	return 0;
}

int CommunicateServiceTask::OnStart()
{


	char szLocation[260] = { 0 };
    string strCfgPath = PKComm::GetConfigPath();
    strCfgPath = strCfgPath + PK_OS_DIR_SEPARATOR + "pkhostservice.xml";

	//
    ReadConfig(strCfgPath.c_str());

	long lRet = SOCKSVR_Init(ConnectCallBack, SOCK_SEND_SYNC);
	if (0 != lRet)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "SOCKCLI_Init Failed");
		return lRet;
	}
	SOCKSVR_SetPackHeaderParam(PACKHEADERLENGTH, PackageTotalLength);

	if (SOCKSVR_Listen(m_nListenPort) != 0)
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Listen Port(%d) Failed", m_nListenPort);
	else
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Listen Port(%d) Success", m_nListenPort);
	return 0;
}

int CommunicateServiceTask::StartTask()
{
	m_bStop = false;
	int nRet = 0;
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "The ExecuteTask is starting...");
	//avoid concurrence
	ACE_OS::sleep(1);
	nRet = (long)this->activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Failed to start ServiceMainTask, Errorcode(%d).", nRet);
		return nRet;
	}
	return nRet;
}

int CommunicateServiceTask::ReadConfig(const char* szFilePath)
{
	tinyxml2::XMLDocument doc;
	int nRet = doc.LoadFile(szFilePath);
	if (tinyxml2::XML_SUCCESS != nRet)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "pkHostService Load Config File(%s) Failed, Ret:%s", szFilePath, nRet);
		return nRet;
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "pkHostService Load Config File(%s) Success", szFilePath);

	//EndPoint
	tinyxml2::XMLElement* pRoot = doc.RootElement();
	if (!pRoot)
	{
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "The Config File Has Not A RootNode");
		return -1;
	}

	if (pRoot->NoChildren())
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "There is no Childs in The RootNode");
		return -1;
	}

	tinyxml2::XMLElement *pListenPort = pRoot->FirstChildElement();
	if (!pListenPort)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Query Node(%s) value Failed.", NODE_LISTENPORT);
		return -1;
	}
	sscanf(pListenPort->GetText(), "%i", &m_nListenPort);

	tinyxml2::XMLElement *pRefreshRate = pListenPort->NextSiblingElement(NODE_REFRESHRATE);
	if (!pRefreshRate)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Query Node(%s) value Failed.", NODE_REFRESHRATE);
		return -1;
	}
	sscanf(pRefreshRate->GetText(), "%i", &m_nRefreshRate);

	return 0;
}
