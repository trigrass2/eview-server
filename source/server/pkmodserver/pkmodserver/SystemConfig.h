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


// 每个tag点的信息和值。用于从过程库读取tag点值的接口。
typedef struct tagModbusServiceTag
{
	ModbusTag *pModbusTag;
	// 下面这些不是ModbusData.dll用的, 是本程序(modbusservice)方便起见使用的
	int	nDataType;		// 数据类型,int/short...
	int nDataTypeLen;	// 对于字符串需要长度，其他类型保留为0即可。根据配置文件读取到的数据类型，计算出需要的寄存器数量

	int nRegisterNum;		// 该TAG点占用的寄存器数目.// 对于字符串需要长度，其他类型保留为0即可。每个数据类型占用固定长度的寄存器，BIT必须用DI或DO，其他必须是AI或AO，字符串和BLOB为(1+长度)/2，其他类型都是固定长度（1...4）个AI/AO寄存器
	int nEndRegisterNo;		// 结束寄存器.不是下一个寄存器
	int nStartRegisterNo;	// 起始寄存器地址（AI和AO 2个字节，DI和DO 1个位），从1开始
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
	string strConnType;		// 连接类型。空表示tcpserver，serial表示串口
	string strConnParam;	// 连接类型。modbustcp为端口号，modbus rtu为连接参数
	string strModServerName;
	vector<ModbusServiceTag *>	m_vecAIAddrToDit; // 必须按照modbus起始地址排序
	vector<ModbusServiceTag *>	m_vecAOAddrToDit; // 必须按照modbus起始地址排序
	vector<ModbusServiceTag *>	m_vecDIAddrToDit; // 必须按照modbus起始地址排序
	vector<ModbusServiceTag *>	m_vecDOAddrToDit; // 必须按照modbus起始地址排序
	vector<ModbusServiceTag *>	m_vecUnkwnAddrToDit; // 必须按照modbus起始地址排序
}ModbusServerInfo;

class CSystemConfig  
{
public:
	//读取配置
	int ReadConfig();
	static int GetRegisterType(const char *szRegisterType);
public:
	// map<string, ModbusForwardInfo *>	m_mapModbusPortToForwardInfo;
	ModbusServerInfo					*m_pForwardInfo;
	ModbusTagVec						m_vecAllTags; // 用于初始化时作为参数传入
public:
	CSystemConfig();
	virtual ~CSystemConfig();
	int GetRegisterNum(int nDataType, int nRegisterType, int nLenBytes);
};

#define SYSTEM_CONFIG ACE_Singleton<CSystemConfig, ACE_Thread_Mutex>::instance()
#endif


