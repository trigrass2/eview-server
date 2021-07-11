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

using namespace std;
#include <string>
#include <vector>
#include <map>
#include "include/modbusdataapi.h"

#define	MAX_IP_LENGTH				32
#define MODBUS_REGISTERTYPE_DO				0
#define MODBUS_REGISTERTYPE_DI				1
#define MODBUS_REGISTERTYPE_AO				3
#define MODBUS_REGISTERTYPE_AI				4
#define MODBUS_REGISTERTYPE_UNKOWN			0xFF

#define MODBUS_REGISTERTYPE_NAME_DO			"DO"
#define MODBUS_REGISTERTYPE_NAME_DI			"DI"
#define MODBUS_REGISTERTYPE_NAME_AO			"AO"
#define MODBUS_REGISTERTYPE_NAME_AI			"AI"


// ÿ��tag�����Ϣ��ֵ�����ڴӹ��̿��ȡtag��ֵ�Ľӿڡ�
typedef struct tagModbusServiceTag
{
	ModbusTag *pModbusTag;
	// ������Щ����ModbusData.dll�õ�, �Ǳ�����(modbusservice)�������ʹ�õ�
	int	nDataType;		// ��������,int/short...
	int nDataTypeLen;	// �����ַ�����Ҫ���ȣ��������ͱ���Ϊ0���ɡ����������ļ���ȡ�����������ͣ��������Ҫ�ļĴ�������

	int nRegisterNum;		// ��TAG��ռ�õļĴ�����Ŀ.// �����ַ�����Ҫ���ȣ��������ͱ���Ϊ0���ɡ�ÿ����������ռ�ù̶����ȵļĴ�����BIT������DI��DO������������AI��AO���ַ�����BLOBΪ(1+����)/2���������Ͷ��ǹ̶����ȣ�1...4����AI/AO�Ĵ���
	int nEndRegisterNo;		// �����Ĵ���.������һ���Ĵ���
	int nStartRegisterNo;	// ��ʼ�Ĵ�����ַ��AI��AO 2���ֽڣ�DI��DO 1��λ������1��ʼ
	int nRegisterType;		// AI/AO/DI/DO
	char szBinData[MAX_TAGDATA_LENGTH];

	tagModbusServiceTag()
	{
		pModbusTag = new ModbusTag();
		pModbusTag->pInternalRef = this;
		nDataType = 0;
		nDataTypeLen = 0;
		memset(szBinData, 0, sizeof(szBinData));
		nEndRegisterNo = -1;
		nRegisterNum = 0;
		nStartRegisterNo = -1;
		nRegisterType = 0xFF;
	}
}ModbusServiceTag;
typedef vector<ModbusServiceTag *> ModbusServiceTagVec;

typedef struct tagMODBUSFORWARDINFO
{
	string strConnType;		// �������͡��ձ�ʾtcpserver��serial��ʾ����
	string strConnParam;	// �������͡�modbustcpΪ�˿ںţ�modbus rtuΪ���Ӳ���
	string strModServerName;
	vector<ModbusServiceTag *>	m_vecAIAddrToDit; // ���밴��modbus��ʼ��ַ����
	vector<ModbusServiceTag *>	m_vecAOAddrToDit; // ���밴��modbus��ʼ��ַ����
	vector<ModbusServiceTag *>	m_vecDIAddrToDit; // ���밴��modbus��ʼ��ַ����
	vector<ModbusServiceTag *>	m_vecDOAddrToDit; // ���밴��modbus��ʼ��ַ����
	vector<ModbusServiceTag *>	m_vecUnkwnAddrToDit; // ���밴��modbus��ʼ��ַ����
}ModbusServerInfo;

class CSystemConfig  
{
public:
	//��ȡ����
	int ReadConfig();
	static int GetRegisterType(const char *szRegisterType);
public:
	// map<string, ModbusForwardInfo *>	m_mapModbusPortToForwardInfo;
	ModbusServerInfo					*m_pForwardInfo;
	ModbusTagVec						m_vecAllTags; // ���ڳ�ʼ��ʱ��Ϊ��������
public:
	CSystemConfig();
	virtual ~CSystemConfig();
	int GetRegisterNum(int nDataType, int nRegisterType, int nLenBytes);
};

#define SYSTEM_CONFIG ACE_Singleton<CSystemConfig, ACE_Thread_Mutex>::instance()
#endif


