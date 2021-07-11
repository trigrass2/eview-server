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
	 *  CPU核数.
	 *
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version 
	 **/
	int ProcessorCores(); 
	/**
	 *  CPU各核的使用率(%).
	 *
	 *  @param		-[in,out]  vector<double> &vUtilization: [使用率]
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version 
	 *	@example	cpu : 2.47525(%)
	 *				cpu0: 1.96078(%)
	 *				cpu1: 2.2703(%)
	 *				...
	 **/
	int CpuUsageOfCores(vector<double> &vUtilization);
	/**
	 *  进程的CPU使用率(%).
	 *
	 *  @param		-[in,out]  double &nUtilize: [使用率]
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
	 *  物理内存(kB).
	 *
	 *  @param		-[in,out]  ULLong &nTotal: [总值] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@example	1024(kB)
	 **/
	int	PhysicalMemory(ULLong &nTotal);	
	/**
	 *  空闲物理内存(kB).
	 *
	 *  @param		-[in,out]  ULLong &nFree: [空闲值] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@example	512(kB)
	 **/
	int PhysicalMemoryFree(ULLong &nFree);	
	/**
	 *  进程的物理内存使用值(kB).
	 *
	 *  @param		-[in,out]  ULLong &nUsage: [使用值] 
	 *  @param		-[in]  int nPid: [PID] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[Pid = -1] Mean Current Process
	 *	@example	64(kB)
	 **/
	int PhysicalMemroyUsageByProcess(ULLong &nUsage, int nPid = -1);
	/**
	 *  虚拟内存(kB).
	 *
	 *  @param		-[in,out]  ULLong &nTotal: [总值] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@example	1024(kB)
	 **/
	int	VirtualMemory(ULLong &nTotal);	
	/**
	 *  空闲虚拟内存(kB).
	 *
	 *  @param		-[in,out]  ULLong &nFree: [空闲值] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@example	512(kB)
	 **/
	int VirtualMemoryFree(ULLong &nFree);	
	/**
	 *  进程的虚拟内存使用值(kB).
	 *
	 *  @param		-[in,out]  ULLong &nUsage: [使用值] 
	 *  @param		-[in]  int nPid: [PID] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[Pid = -1] Mean Current Process
	 *	@example	64(kB)
	 **/
	int VirtualMemroyUsageByProcess(ULLong &nUsage, int nPid = -1);

private:
	/* 物理内存(kB) */
	ULLong	m_nPhysicalMemory;
	/* 虚拟内存(kB) */
	ULLong	m_nVirtualMemory;
	
/*------ Memory End ------*/

	
/*------Disk Start------*/
public:
	/**
	 *  磁盘数.
	 *
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 **/
	int DiskCount();
	/**
	 *  挂载/盘符数.
	 *
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 **/
	int	DriveLetterCount();
	/**
	 *  磁盘容量信息.
	 *
	 *  @param		-[in,out]  vector<CapacityInfo> &vCapacityInfo: [容量信息]  
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version    
	 **/
	int TheDiskCapacityInfo(vector<CapacityInfo*> &vCapacityInfo);
	/**
	 *  挂载/盘符容量信息.
	 *
	 *  @param		-[in,out]  vector<CapacityInfo> &vCapacityInfo: [容量信息] 
	 *  @param		-[in]  const char* szDriveLetter: [挂载/盘符] 
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
	 *  结束进程.
	 *
	 *  @param		-[in]  int nPid: [PID] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[nPid = -1] Mean Self-Process 
	 **/
	int Kill(int nPid = -1);	
	/**
	 *  打开文件.
	 *
	 *  @param		-[in]  const char* szFile: [文件] 
	 *  @param		-[in]  const char* szParameters: [参数] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[szParameters = NULL] Mean Without Parameter 
	 **/
	int OpenTheFile(const char* szFile, const char* szParameters = NULL);	
	/**
	 *  进程信息.
	 *
	 *  @param		-[in,out]  vector<ProcessInfo*> &vProcessInfo: [进程信息] 
	 *  @param		-[in]  int nPid: [PID] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[nPid = -1] Mean All Process-Info 
	 **/
	int Processes(vector<ProcessInfo*> &vProcessInfo, int nPid = -1);	
	/**
	 *  进程信息.
	 *
	 *  @param		-[in]  const char* szProcessName: [进程名称] 
	 *  @param		-[in,out]  vector<int> &vPids: [PIDs] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[nPid = -1] Mean Error Occurred 
	 **/
	int TheProcessIDsByName(const char* szProcessName, vector<int> &vPids);
	/**
	 *  进程信息.
	 *
	 *  @param		-[in]  const char* szProcessNameAndCmdLine: [进程名称|启动参数(不包含进程路径或路径)] 
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
	 *  @param		-[in]  char* szHostName: [主机地址]  
	 *  @return		bool.  
	 *  @version	10/28/2016	W   Initial Version   
	 **/
	bool Ping(const char *szIPAddress);
	/**
	 *  主机名称.
	 *
	 *  @param		-[in]  char* szHostName: [主机名称] 
	 *  @param		-[in,out]  unsigned long &nLength: [长度] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version  
	 *	@remark		-[nPid = -1] Mean Error Occurred 
	 **/
	int	TheHostName(char* szHostName, unsigned long &nLength);		
	/**
	 *  当前用户名称.
	 *
	 *  @param		-[in]  char* szUserName: [用户名称] 
	 *  @param		-[in,out]  unsigned long &nLength: [长度] 
	 *  @return		int.  
	 *  @version	10/28/2016	W   Initial Version   
	 **/
	int TheUserName(char* szUserName, unsigned long &nLength);		
	/**
	 *  主机IP.
	 *
	 *  @param		-[in,out]  vector<string> &vIPAddress: [IP] 
	 *  @param		-[in]  unsigned short nFamily: [协议] 
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