/**************************************************************
 *  Filename:    CC_GEN_DRVCFG.CPP
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: $().
 *               
**************************************************************/ 
#include "../pkmodserver/include/modbusdataapi.h"
#include "pkdata/pkdata.h"
#include "pklog/pklog.h"
#include "json/json.h"
CPKLog g_logger;

#ifdef _WIN32
#define  MODBUSDATAAPI_EXPORTS extern "C" __declspec(dllexport)
#else //_WIN32
#define  MODBUSDATAAPI_EXPORTS extern "C"
#endif //_WIN32

PKHANDLE g_hPKData = NULL;
MODBUSDATAAPI_EXPORTS int Init(ModbusTagVec &modbusTagVec)
{
	string strTagNames;
	vector<ModbusTag *>::iterator itTag = modbusTagVec.begin(); // �ǰ���ÿ��tag���Ӧ����ʼ��ַ����ġ�
	for(itTag = modbusTagVec.begin(); itTag !=  modbusTagVec.end(); itTag ++)
	{
		ModbusTag *pTag = *itTag;
		if(strTagNames.empty())
			strTagNames = pTag->szName;
		else
			strTagNames = strTagNames + ",  " + pTag->szName;
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "modbuspkdata, Init(count=%d), tag:%s",modbusTagVec.size(), strTagNames.c_str());

	g_hPKData = pkInit(NULL,NULL,NULL);
	if(g_hPKData == NULL)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "modbuspkdata, Init(count=%d) failed,g_hPKData == NULL",modbusTagVec.size());
		return -1;
	}
    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "modbuspkdata, Init(count=%d) success,g_hPKData == %ld",modbusTagVec.size(), (long)g_hPKData);
	return 0;
}
// ��ȡtag���ֵ���������tag����
MODBUSDATAAPI_EXPORTS int ReadTags(ModbusTagVec &modbusTagVec)
{
	if(g_hPKData == NULL)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "modbuspkdata, must inited first,g_hPKData == NULL,ReadTags,count=%d",modbusTagVec.size());
		return -2;
	}
	else
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "modbuspkdata, ReadTags,count=%d",modbusTagVec.size());

	if(modbusTagVec.size() <= 0)
		return 0;

	PKDATA *pkDatas = new PKDATA[modbusTagVec.size()]();
	// ��֯������ȡ�ӿ�
	int iTag = 0;
	vector<ModbusTag *>::iterator itTag = modbusTagVec.begin(); // �ǰ���ÿ��tag���Ӧ����ʼ��ַ����ġ�
	for(itTag = modbusTagVec.begin(); itTag !=  modbusTagVec.end(); itTag ++)
	{
		ModbusTag *pTag = *itTag;
		memset(pTag->szValue, 0, sizeof(pTag->szValue));

		vector<string> vecObjPropField = PKStringHelper::StriSplit(pTag->szName, ".");	// object.prop.field(picchose.value)---->picchose  value
		string strTagName = vecObjPropField[0]; // tag����
		string strField = "";
		if (vecObjPropField.size() >= 2)
			strField = vecObjPropField[vecObjPropField.size() - 1]; // ȡ���һ��
		for (int i = 1; i < vecObjPropField.size() - 1; i++)
		{
			strTagName = strTagName + "." + vecObjPropField[i];
		}
		//��ʱ��Ϊһ����value
		if (PKStringHelper::StriCmp(strField.c_str(), "value") != 0 && PKStringHelper::StriCmp(strField.c_str(), "v") != 0) // ������һ�β���value��tag��Ϊpicchose.text
		{
			strField = "v";
			if (strField.length() >= 1)
				strTagName = strTagName + "." + strField;
			strField = "v"; // ȡ���һ��, ʹ��v������value
		}
		else
		{
			strField = "v"; // ȡ���һ��v������value
		}
		PKStringHelper::Safe_StrNCpy(pkDatas[iTag].szObjectPropName, strTagName.c_str(), sizeof(pkDatas[iTag].szObjectPropName)); // field
		PKStringHelper::Safe_StrNCpy(pkDatas[iTag].szFieldName, strField.c_str(), sizeof(pkDatas[iTag].szFieldName)); // field
		iTag ++;
	}

	int nRet = pkMGet(g_hPKData, pkDatas, modbusTagVec.size());
	if(nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "pkMGet failed(count=%d),ret:%d", modbusTagVec.size(), nRet);
	}

	// �����һ��һ����ֵ
	iTag = 0;
	for(itTag = modbusTagVec.begin(); itTag !=  modbusTagVec.end(); itTag ++)
	{
		ModbusTag *pTag = *itTag;
		if(nRet == 0 && pkDatas[iTag].nStatus == 0)
		{
			PKStringHelper::Safe_StrNCpy(pTag->szValue, pkDatas[iTag].szData, sizeof(pTag->szValue));
			pTag->nStatus = 0;
			/*
			Json::Reader jsonReader;
			Json::Value root;
			if(jsonReader.parse(pkDatas[iTag].szData, root, false) == false)
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ReadTags,tag=%s,value=%s,not json format!", pkDatas[iTag].szName, pkDatas[iTag].szData);
				pTag->nStatus = -100;
			}
			else
			{
				if(root["v"].isNull())
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ReadTags,tag=%s,value=%s, not found v or v invalid", pkDatas[iTag].szName, pkDatas[iTag].szData);
					pTag->nStatus = -200;
				}
				else
				{
					strncpy(pTag->szValue, root["v"].asString().c_str(), sizeof(pTag->szValue) - 1);
					pTag->nStatus = 0;
				}
			}*/
		}
		else
		{
			// ����ʱ��ֵ��Ϊ���ַ���
			pTag->szValue[0] = '\0';
			if(pkDatas[iTag].nStatus != 0)
				pTag->nStatus = pkDatas[iTag].nStatus;
			else
				pTag->nStatus = 0;
		}
		iTag ++;
	}

	delete [] pkDatas;
	pkDatas = NULL;

	return nRet;
}

MODBUSDATAAPI_EXPORTS int WriteTags(ModbusTagVec &modbusTagVec)
{
	if(g_hPKData == NULL)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "modbuspkdata, must inited first,g_hPKData == NULL,on WriteTags count=%d=",modbusTagVec.size());
		return -2;
	}
	else
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "modbuspkdata, WriteTags(from pkdata),count=%d",modbusTagVec.size());

	if(modbusTagVec.size() <= 0)
		return 0;

	PKDATA *pkDatas = new PKDATA[modbusTagVec.size()]();
	// ��֯�����ӿ�
	string strTagFields = "";
	int iTag = 0;
	vector<ModbusTag *>::iterator itTag = modbusTagVec.begin(); // �ǰ���ÿ��tag���Ӧ����ʼ��ַ����ġ�
	for(itTag = modbusTagVec.begin(); itTag !=  modbusTagVec.end(); itTag ++)
	{
		ModbusTag *pTag = *itTag;
		vector<string> vecObjPropField = PKStringHelper::StriSplit(pTag->szName,".");	// object.prop.field(picchose.value)---->picchose  value
		string strField = vecObjPropField[vecObjPropField.size() - 1]; // ȡ���һ��
		string strTagName = vecObjPropField[0];
		for (int i = 1; i < vecObjPropField.size() - 1; i++)
		{
			strTagName = strTagName + "." + vecObjPropField[i];
		}
		//��ʱ��Ϊһ����value
		if (PKStringHelper::StriCmp(strField.c_str(), "value") != 0) // ������һ�β���value��tag��Ϊpicchose.text
		{
			strTagName = strTagName + "." + vecObjPropField[vecObjPropField.size() - 1];
			strField = "value"; // ȡ���һ��
		}
		else
		{
			strField = "value"; // ȡ���һ��
		}
		memset(pkDatas[iTag].szObjectPropName, 0, sizeof(pkDatas[iTag].szObjectPropName)); // tagname
		PKStringHelper::Safe_StrNCpy(pkDatas[iTag].szObjectPropName, strTagName.c_str(), sizeof(pkDatas[iTag].szObjectPropName)); // field
		PKStringHelper::Safe_StrNCpy(pkDatas[iTag].szFieldName, strField.c_str(), sizeof(pkDatas[iTag].szFieldName)); // field
		memset(pkDatas[iTag].szData, 0, pkDatas[iTag].nDataBufLen);
		PKStringHelper::Safe_StrNCpy(pkDatas[iTag].szData, pTag->szValue, pkDatas[iTag].nDataBufLen);
		pkDatas[iTag].nStatus = 0;// д��ʱ����ΪGOOD

		if (itTag == modbusTagVec.begin()) // ��һ��
			strTagFields = pkDatas[iTag].szFieldName;
		else
			strTagFields = strTagFields + ";" + pkDatas[iTag].szFieldName;

		iTag ++;

	}
	int nRet = pkMControl(g_hPKData, pkDatas, modbusTagVec.size());
	if(nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "pkMControl(tagcount=%d,tagfield:%s) failed,ret:%d", modbusTagVec.size(), strTagFields.c_str(), nRet);
	}
	else
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "pkMControl(tagcount=%d,tagfield:%s) success",modbusTagVec.size(), strTagFields.c_str());

	// �����һ��һ����ֵ
	iTag = 0;
	for(itTag = modbusTagVec.begin(); itTag !=  modbusTagVec.end(); itTag ++)
	{
		ModbusTag *pTag = *itTag;
		pkDatas[iTag].nStatus = nRet;
		iTag ++;
	}

	delete [] pkDatas;
	pkDatas = NULL;
	return nRet;
}

MODBUSDATAAPI_EXPORTS int UnInit(ModbusTagVec &modbusTagVec)
{
	if(g_hPKData != NULL)
		pkExit(g_hPKData);
	g_hPKData = NULL;
	return 0;
}
