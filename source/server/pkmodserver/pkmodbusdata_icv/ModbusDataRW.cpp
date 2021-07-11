/**************************************************************
 *  Filename:    CC_GEN_DRVCFG.CPP
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: $().
 *               
**************************************************************/ 
#include "../pkmodserver/include/modbusdataapi.h"
#include "cvlda.h"

#ifdef _WIN32
#define  MODBUSDATAAPI_EXPORTS extern "C" __declspec(dllexport)
#else //_WIN32
#define  MODBUSDATAAPI_EXPORTS extern "C"
#endif //_WIN32

long g_lHandle = 0;

MODBUSDATAAPI_EXPORTS int Init(ModbusTagVec &modbusTagVec)
{
	int nRetInit = LDA_Init(&g_lHandle);
	if(nRetInit != 0)
	{
		printf("LDA_Init failed:%d, modbustag count:%d\n", nRetInit, modbusTagVec.size());
		//PKLog.LogMessage(PK_LOGLEVEL_ERROR, "LDA_Init failed:%d, modbustag count:%d", nRetInit, modbusTagVec.size());
	}
	else
		printf("LDA_Init success:%d, modbustag count:%d\n", nRetInit, modbusTagVec.size());
		//PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "LDA_Init success:%d, modbustag count:%d", nRetInit, modbusTagVec.size());
	return 0;
}
// 读取tag点的值，并保存回tag点中。读取到的值都必须转换为字符串类型（不能是原始类型）
MODBUSDATAAPI_EXPORTS int ReadTags(ModbusTagVec &modbusTagVec)
{
	vector<ModbusTag *>::iterator itTag = modbusTagVec.begin(); // 是按照每个tag点对应的起始地址排序的。
	for(; itTag !=  modbusTagVec.end(); itTag ++)
	{
		ModbusTag *pTag = *itTag;
		memset(pTag->szValue, 0, sizeof(pTag->szValue));
		int nValueLen = strlen(pTag->szValue);
		pTag->nStatus= LDA_ReadValue((char *)pTag->szName, "", pTag->szValue, &nValueLen); // tag名称不能带有scada节点的名字，缺省为F_CV，且读出的值全部转换为字符串类型
		if(pTag->nStatus != 0)
		{
			printf("LDA_ReadValue(%s) failed:%d\n", pTag->szName, pTag->nStatus);
			//PKLog.LogMessage(PK_LOGLEVEL_ERROR, "LDA_ReadValue(%s) failed:%d", pTag->szTagName, pTag->nRetVal);
			continue;
		}
	}
	return 0;
}

// 写入的值都必须转换为字符串类型（不能是原始类型）
MODBUSDATAAPI_EXPORTS int WriteTags(ModbusTagVec &modbusTagVec)
{
	vector<ModbusTag *>::iterator itTag = modbusTagVec.begin(); // 是按照每个tag点对应的起始地址排序的。
	for(; itTag !=  modbusTagVec.end(); itTag ++)
	{
		ModbusTag *pTag = *itTag;
		pTag->nStatus = LDA_WriteValue((char *)pTag->szName, "", pTag->szValue, 0); // 写入的值全部转换为字符串作为值的类型
		printf("LDA_WriteValue(%s,%s) ret:%d\n", pTag->szName, pTag->szValue, pTag->nStatus);
	}
	return 0;
}

MODBUSDATAAPI_EXPORTS int UnInit(ModbusTagVec &modbusTagVec)
{
	LDA_Release(g_lHandle);
	return 0;
}