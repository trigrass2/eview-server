/**************************************************************
 *  Filename:    SystemConfig.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 读取配置信息头文件.
 *
 *  @author:     liuqifeng
 *  @version     04/13/2009  liuqifeng  Initial Version
**************************************************************/
#ifndef _SYSTEMCONFIG_H_
#define _SYSTEMCONFIG_H_

#include "driversdk/DeviceDataTable.h"
using namespace std;
#include <string>
#include <vector>
#include <map>

#define		MAX_IP_LENGTH		128

class CSystemConfig  
{
public:
	//读取配置
	bool LoadConfig(); 
	string m_strServerIp;	// 服务端IP或域名
	int m_nServerPort;		// 服务端侦听端口
	int m_nCycleMS;			// 转发周期
	string m_strGatewayName;	// 网关名
	string m_strGatewayPath;	// 网关路径

	string m_strProjectCode;
	
public: 

public:
	CSystemConfig();
	virtual ~CSystemConfig();

};

#define SYSTEM_CONFIG ACE_Singleton<CSystemConfig, ACE_Thread_Mutex>::instance()
#endif


