#ifndef	_DATA_ACQUISITION_H_
#define	_DATA_ACQUISITION_H_

#include "Peer2PeerData.h"
#include <vector> 
#include <string>

using namespace std;

/*
	Data Acquisition (DAQ)
*/
class DataAcquisition
{
public:
	DataAcquisition(void);
	~DataAcquisition(void);

	
/*------CPU Start------*/
public:	
	/**
	 *  CPU����.
	 *
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version 
	 **/
	int ProcessorCores(); 
	/**
	 *  CPU���˵�ʹ����(%).
	 *
	 *  @param		-[in,out]  vector<double> &vUtilization: [ʹ����]
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version 
	 *	@example	cpu : 2.47525(%)
	 *				cpu0: 1.96078(%)
	 *				cpu1: 2.2703(%)
	 *				...
	 **/
	int CpuUsageOfCores(vector<double> &vUtilization);
	/**
	 *  ���̵�CPUʹ����(%).
	 *
	 *  @param		-[in,out]  double &nUtilize: [ʹ����]
	 *  @param		-[in]  int nPid: [PID] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version 
	 *	@remark		-[Pid = -1] Mean Current Process
	 *	@example	2.47525(%)
	 **/
	int CpuUsageByProcess(double &nUtilize, int nPid = -1);

private:
	/* CPU Cores */
	int m_nProcessorCores;
/*------ CPU End ------*/

	
/*------Memory Start------*/
public:
	/**
	 *  �����ڴ�(kB).
	 *
	 *  @param		-[in,out]  ULLong &nTotal: [��ֵ] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@example	1024(kB)
	 **/
	int	PhysicalMemory(ULLong &nTotal);	
	/**
	 *  ���������ڴ�(kB).
	 *
	 *  @param		-[in,out]  ULLong &nFree: [����ֵ] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@example	512(kB)
	 **/
	int PhysicalMemoryFree(ULLong &nFree);	
	/**
	 *  ���̵������ڴ�ʹ��ֵ(kB).
	 *
	 *  @param		-[in,out]  ULLong &nUsage: [ʹ��ֵ] 
	 *  @param		-[in]  int nPid: [PID] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[Pid = -1] Mean Current Process
	 *	@example	64(kB)
	 **/
	int PhysicalMemroyUsageByProcess(ULLong &nUsage, int nPid = -1);
	/**
	 *  �����ڴ�(kB).
	 *
	 *  @param		-[in,out]  ULLong &nTotal: [��ֵ] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@example	1024(kB)
	 **/
	int	VirtualMemory(ULLong &nTotal);	
	/**
	 *  ���������ڴ�(kB).
	 *
	 *  @param		-[in,out]  ULLong &nFree: [����ֵ] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@example	512(kB)
	 **/
	int VirtualMemoryFree(ULLong &nFree);	
	/**
	 *  ���̵������ڴ�ʹ��ֵ(kB).
	 *
	 *  @param		-[in,out]  ULLong &nUsage: [ʹ��ֵ] 
	 *  @param		-[in]  int nPid: [PID] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[Pid = -1] Mean Current Process
	 *	@example	64(kB)
	 **/
	int VirtualMemroyUsageByProcess(ULLong &nUsage, int nPid = -1);

private:
	/* �����ڴ�(kB) */
	ULLong	m_nPhysicalMemory;
	/* �����ڴ�(kB) */
	ULLong	m_nVirtualMemory;
	
/*------ Memory End ------*/

	
/*------Disk Start------*/
public:
	/**
	 *  ������.
	 *
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 **/
	int DiskCount();
	/**
	 *  ����/�̷���.
	 *
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 **/
	int	DriveLetterCount();
	/**
	 *  ����������Ϣ.
	 *
	 *  @param		-[in,out]  vector<CapacityInfo> &vCapacityInfo: [������Ϣ]  
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version    
	 **/
	int TheDiskCapacityInfo(vector<CapacityInfo*> &vCapacityInfo);
	/**
	 *  ����/�̷�������Ϣ.
	 *
	 *  @param		-[in,out]  vector<CapacityInfo> &vCapacityInfo: [������Ϣ] 
	 *  @param		-[in]  const char* szDriveLetter: [����/�̷�] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[szDriveLetter = NULL] Mean All Drive-Letter 
	 *	@example	Windows(szDriveLetter = 'C:\'), Linux(szDriveLetter = '/root')
	 **/
	int TheDriveLetterCapacityInfo(vector<CapacityInfo*> &vCapacityInfo, const char* szDriveLetter = NULL);
/*------ Disk End ------*/
	

/*------Process Start------*/
public:
	/**
	 *  ��������.
	 *
	 *  @param		-[in]  int nPid: [PID] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[nPid = -1] Mean Self-Process 
	 **/
	int Kill(int nPid = -1);	
	/**
	 *  ���ļ�.
	 *
	 *  @param		-[in]  const char* szFile: [�ļ�] 
	 *  @param		-[in]  const char* szParameters: [����] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[szParameters = NULL] Mean Without Parameter 
	 **/
	int OpenTheFile(const char* szFile, const char* szParameters = NULL);	
	/**
	 *  ������Ϣ.
	 *
	 *  @param		-[in,out]  vector<ProcessInfo*> &vProcessInfo: [������Ϣ] 
	 *  @param		-[in]  int nPid: [PID] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[nPid = -1] Mean All Process-Info 
	 **/
	int Processes(vector<ProcessInfo*> &vProcessInfo, int nPid = -1);	
	/**
	 *  ������Ϣ.
	 *
	 *  @param		-[in]  const char* szProcessName: [��������] 
	 *  @param		-[in,out]  vector<int> &vPids: [PIDs] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[nPid = -1] Mean Error Occurred 
	 **/
	int TheProcessIDsByName(const char* szProcessName, vector<int> &vPids);
	/**
	 *  ������Ϣ.
	 *
	 *  @param		-[in]  const char* szProcessNameAndCmdLine: [��������|��������(����������·����·��)] 
	 *  @param		-[in,out]  int &nPid: [PID] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[nPid = -1] Mean Error Occurred 
	 *	@example	Windows(szProcessNameAndCmdLine = 'example.exe|cmdline'), Linux(szProcessNameAndCmdLine = 'bash|cmdline')
	 **/
	int TheProcessIDByNameAndCmdLine(const char* szProcessNameAndCmdLine, int &nPid);
/*------ Process End ------*/
	

/*------PC Start------*/
public:		
	/**
	 *  Ping.
	 *
	 *  @param		-[in]  char* szHostName: [������ַ]  
	 *  @return		bool.  
	 *  @version	10/28/2016	W   Initial Version   
	 **/
	bool Ping(const char *szIPAddress);
	/**
	 *  ��������.
	 *
	 *  @param		-[in]  char* szHostName: [��������] 
	 *  @param		-[in,out]  unsigned long &nLength: [����] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[nPid = -1] Mean Error Occurred 
	 **/
	int	TheHostName(char* szHostName, unsigned long &nLength);		
	/**
	 *  ��ǰ�û�����.
	 *
	 *  @param		-[in]  char* szUserName: [�û�����] 
	 *  @param		-[in,out]  unsigned long &nLength: [����] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version   
	 **/
	int TheUserName(char* szUserName, unsigned long &nLength);		
	/**
	 *  ����IP.
	 *
	 *  @param		-[in,out]  vector<string> &vIPAddress: [IP] 
	 *  @param		-[in]  unsigned short nFamily: [Э��] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version   
	 *	@remark			 OS				Family		Value
	 *				Windows			AF_INET				2
	 *				Windows/Linux	AF_INET6		23/10
	 *				Windows			AF_UNSPEC			0
	 **/
	int TheHostIPAddress(vector<string> &vIPAddress, unsigned short nFamily /* = AF_INET or AF_INET6 or AF_UNSPEC*/);
/*------ PC End ------*/


/*------Net Start------*/
/*------ Net End ------*/
};

#endif //_DATA_ACQUISITION_H_