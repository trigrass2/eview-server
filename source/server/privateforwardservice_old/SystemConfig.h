/**************************************************************
 *  Filename:    SystemConfig.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ��ȡ������Ϣͷ�ļ�.
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
	//��ȡ����
	bool LoadConfig(); 
	string m_strServerIp;	// �����IP������
	int m_nServerPort;		// ����������˿�
	int m_nCycleMS;			// ת������
	string m_strGatewayName;	// ������
	string m_strGatewayPath;	// ����·��

	string m_strProjectCode;
	
public: 

public:
	CSystemConfig();
	virtual ~CSystemConfig();

};

#define SYSTEM_CONFIG ACE_Singleton<CSystemConfig, ACE_Thread_Mutex>::instance()
#endif


