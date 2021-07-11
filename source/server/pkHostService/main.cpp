/**************************************************************
 *  Filename:    @OutputName.cpp
 *  Copyright:   Shanghai X Software Co., Ltd.
 *
 *  Description: .
 *
 *  @author:     w
 *  @version     09/@dd/2016  @Reviser  Initial Version
 **************************************************************/
#include "DataAcquisition.h"
#include <iostream>
#include <iomanip>

#ifdef _WIN32 
#include "common/RegularDllExceptionAttacher.h"
CRegularDllExceptionAttacher g_ExceptionAttacher;

#pragma comment(lib,"dbghelp.lib")
#pragma comment(lib,"ExceptionReport.lib")
#else

#include <unistd.h>
#include <limits.h>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <sys/prctl.h>

#endif
  
using namespace std;


static void ResultTest(const char *szMethodName, vector<string> &vResult)
{
	cout << endl;
	cout << "---TEST---\"" << szMethodName << "\"---TEST---" << endl;
	for(vector<string>::iterator item = vResult.begin(); item != vResult.end(); item++)
	{
		cout << "\t" << item->c_str() << endl;
	}
	cout << "---TEST---\"" << szMethodName << "\"---TEST---" << endl;
}

static void ResultTest(const char *szMethodName, vector<double> &vResult)
{
	int count = vResult.size();
	cout << "---TEST---\"" << szMethodName << "\"---TEST---" << endl;
	for(int item = 0; item < count; item++)
	{
		cout << vResult[item] << endl;
	}
	cout << "---TEST---\"" << szMethodName << "\"---TEST---" << endl;
} 
   
static void ResultTest(const char *szMethodName, vector<CapacityInfo*> &vResult)
{ 
	cout << "---TEST---\"" << szMethodName << "\"---TEST---" << endl;
	int count = 0;
	for(vector<CapacityInfo*>::iterator item = vResult.begin(); item != vResult.end(); item++)
	{
		cout << "---item:" << count << " Start---" << endl;
		cout << "\t" << "m_lTotal:" << (*item)->m_nTotal << endl; 
		cout << "\t" << "m_nFree:" << (*item)->m_nFree << endl;
		cout << "\t" << "m_szName:" << (*item)->m_szName << endl;
		cout << "---item:" << count++ << " End---" << endl;
	}
	cout << "---TEST---\"" << szMethodName << "\"---TEST---" << endl;
}
   
static void ResultTest(const char *szMethodName, vector<ProcessInfo*> &vResult)
{ 
	cout << "---TEST---\"" << szMethodName << "\"---TEST---" << endl;
	for(vector<ProcessInfo*>::iterator item = vResult.begin(); item != vResult.end(); item++)
	{ 
#ifdef WIN32
		printf("%d/%d/%d %d:%d:%d %d %llu %s %s %s\n", 
			(*item)->m_nStartTime_Year, (*item)->m_nStartTime_Month, (*item)->m_nStartTime_Day, 
			(*item)->m_nStartTime_Hour, (*item)->m_nStartTime_Minute, (*item)->m_nStartTime_Second, 
			(*item)->m_nPid, (*item)->m_nRunTime, (*item)->m_szName, (*item)->m_szFileName, (*item)->m_szDescribe); 
#else
		printf("%d/%d/%d %d:%d:%d %d %llu %s %s\n", 
			(*item)->m_nStartTime_Year, (*item)->m_nStartTime_Month, (*item)->m_nStartTime_Day, 
			(*item)->m_nStartTime_Hour, (*item)->m_nStartTime_Minute, (*item)->m_nStartTime_Second, 
			(*item)->m_nPid, (*item)->m_nRunTime, (*item)->m_szName, (*item)->m_szFileName); 
#endif 
	}
	cout << "---TEST---\"" << szMethodName << "\"---TEST---" << endl;
}
/*
*/
/*
int main(int argc, char* args[])
{       
	 
	DataAcquisition ig;
 
	//--------------------------Windows-------------------------- 
	
	cout << "-----Method: 0------" << endl;
	int nRet = ig.Kill(9999);
	cout << nRet << endl;
	
	//cout << "-----Method: 1------" << endl;
	//double nU = -1;
	//ig.CpuUsageByProcess(nU); 
	//cout << nU << endl;
	//ig.CpuUsageByProcess(nU,17393);
	//cout << nU << endl;

	//vector<double> v;
	//ig.CpuUsageOfCores(v);  
	//ResultTest("", v);

	//cout << ig.ProcessorCores() << endl;


	//cout << "-----Method: 2------" << endl;
	//cout << ig.DiskCount() << endl;
	//cout << ig.DriveLetterCount() << endl;
	 
	
	//cout << "-----Method: 3------" << endl;
	//if(argc > 0)
	//{
	//	for(int i = 0; i < argc; i++)
	//		cout << "Parameters is: " << args[i] << endl;
	//}
	//else
	//	cout << "argc == 0" << endl;
	//ig.OpenTheFile("D:\\Program Files\\npp.6.9.bin\\notepad++.exe");
	//ig.OpenTheFile("D:\\Program Files\\npp.6.9.bin\\notepad++.exe","a");
	//ig.OpenTheFile("/mnt/hgfs/GatheringService/GatheringService/aa");
	//ig.OpenTheFile("/mnt/hgfs/GatheringService/GatheringService/aa", "a");
	
	
	//cout << "-----Method: 4------" << endl;
	//ULLong a;
	//ig.PhysicalMemory(a); 
	//ig.PhysicalMemoryFree(a); 
	//ig.PhysicalMemroyUsageByProcess(a); 
	//ig.PhysicalMemroyUsageByProcess(a,17393); 
	//ig.VirtualMemory(a); 
	//ig.VirtualMemoryFree(a); 
	//ig.VirtualMemroyUsageByProcess(a); 
	//ig.VirtualMemroyUsageByProcess(a,5574);
	//cout << a << endl;

	//cout << "-----Method: 5------" << endl; 
	//cout << ig.Ping("www.baidu.com") << endl;
	//cout << ig.Ping("www.google.com") << endl;
	
	
	//cout << "-----Method: 6------" << endl;  
	//int pid = -1;
	
	//ig.TheProcessIDByName("jbd2/sda2-8", pid);
	//cout << pid << endl;
	//pid = -1;
	//ig.TheProcessIDByNameAndCmdLine("SogouCloud.exe|-daemon", pid);
	//cout << pid << endl;
	//vector<ProcessInfo*> vProcessInfo;	
	//ig.Processes(vProcessInfo, pid);
	//ResultTest("Processes", vProcessInfo); 
	//cout << pid <<endl;

	
	//cout << "-----Method: 7------" << endl;  
	//cout << ig.ProcessorCores() << endl;

	
	//cout << "-----Method: 8------" << endl;  
	//cout << ig.DiskCount() << endl;
	//cout << ig.DriveLetterCount() << endl;

	//vector<CapacityInfo*> vCapacityInfo2;
	//ig.TheDiskCapacityInfo(vCapacityInfo2);
	//ResultTest("TheDiskCapacityInfo", vCapacityInfo2);
	
	
	//cout << "-----Method: 9------" << endl; 
	//ig.TheDriveLetterCapacityInfo(vCapacityInfo2, "L:\\");
	//ResultTest("TheDriveLetterCapacityInfo", vCapacityInfo2);

	//ig.TheDriveLetterCapacityInfo(vCapacityInfo2, "K:\\");
	//ResultTest("TheDriveLetterCapacityInfo", vCapacityInfo2);
	
	//ig.TheDriveLetterCapacityInfo(vCapacityInfo2);
	//ResultTest("TheDriveLetterCapacityInfo", vCapacityInfo2); 
	//ig.TheDriveLetterCapacityInfo(vCapacityInfo2, "/run/media/wooo/Èí¼þ");
	//ig.TheDriveLetterCapacityInfo(vCapacityInfo2, "/run/media/wooo/570de22d-54a4-d201-4001-e22d54a4d201");
	//ResultTest("TheDriveLetterCapacityInfo", vCapacityInfo2); 

	
	//cout << "-----Method: 10------" << endl;
	//AF_INET or AF_INET6 or AF_UNSPEC 
	//vector<string> vIPAddress;
	//ig.TheHostIPAddress(vIPAddress,2); 
	//ResultTest("TheHostIPAddress 4", vIPAddress); 
	//ig.TheHostIPAddress(vIPAddress,10); 
	//ResultTest("TheHostIPAddress 6", vIPAddress); 
	//ig.TheHostIPAddress(vIPAddress,0); 
	//ResultTest("TheHostIPAddress Both", vIPAddress); 
	
	
	
	//cout << "-----Method: 11------" << endl; 
	//char sz[256] = { 0 };
	//unsigned long size = 256; 
	//ig.TheHostName(sz, size);
	//cout << sz << " " << size << endl;	 
	//ig.TheUserName(sz, size);
	//cout << sz << " " << size << endl;
	

	getchar();
  	
	return 0;
}
*/
