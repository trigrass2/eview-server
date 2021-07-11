/**************************************************************
 *  Filename:    CC_GEN_DRVCFG.CPP
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: $().
 *               
**************************************************************/ 
#include "../pkmodserver/include/modbusdataapi.h"
#include "stdio.h"
#include "pkcomm/pkcomm.h"

#ifdef _WIN32
#define  MODBUSDATAAPI_EXPORTS extern "C" __declspec(dllexport)
#else //_WIN32
#define  MODBUSDATAAPI_EXPORTS extern "C"
#endif //_WIN32

int g_nSimValue = 0;
MODBUSDATAAPI_EXPORTS int Init(ModbusTagVec &modbusTagVec)
{
	return 0;
}

// ��ȡtag���ֵ���������tag����
MODBUSDATAAPI_EXPORTS int ReadTags(ModbusTagVec &modbusTagVec)
{
	char szValue[64] = { 0 };
	sprintf(szValue, "%dstring", g_nSimValue++);
	vector<ModbusTag *>::iterator itTag = modbusTagVec.begin(); // �ǰ���ÿ��tag���Ӧ����ʼ��ַ����ġ�
	for (itTag = modbusTagVec.begin(); itTag != modbusTagVec.end(); itTag++)
	{
		ModbusTag *pTag = *itTag;
		PKStringHelper::Safe_StrNCpy(pTag->szValue, szValue, sizeof(pTag->szValue));
		pTag->nStatus = 0;
	}
	return 0;
}

MODBUSDATAAPI_EXPORTS int WriteTags(ModbusTagVec &modbusTagVec)
{
	vector<ModbusTag *>::iterator itTag = modbusTagVec.begin(); // �ǰ���ÿ��tag���Ӧ����ʼ��ַ����ġ�
	for(; itTag !=  modbusTagVec.end(); itTag ++)
	{
		ModbusTag *pTag = *itTag;

	}
	return 0;
}

MODBUSDATAAPI_EXPORTS int UnInit(ModbusTagVec &modbusTagVec)
{
	return 0;
}
/*
DRIVERMAP	m_mapDrivers;	// ����Ҫ�õ�����������Ϣ
CVTAGMAP	m_mapCVTags;	// ����iCV���������õĵ��MAP
vector<TAGGROUP *>	m_vecTagGroup;	// TAG�����洢�Ա�

#define NULLASSTRING(x) x==NULL?"":x
#define DRIVER_DEFAULT_ACEESSDLL_NAME	"DDA"	// ȱʡ�ķ�������DLL����
#define	FNAME_PFN_DDA_Init		ACE_TEXT("DDA_Init")
#define	FNAME_PFN_DDA_parse		ACE_TEXT("DDA_Parse")
#define	FNAME_PFN_DDA_Release	ACE_TEXT("DDA_Release")
#define	FNAME_PFN_DDA_read		ACE_TEXT("DDA_Read")
#define	FNAME_PFN_DDA_send		ACE_TEXT("DDA_Send")

#define TAG_MAXLINE_SIZE		2048
#define CV_MAX_NUMBER_DATATYPE_TOTAL 19
#define ICV_BLOBVALUE_MAXLEN 64512 
#define PDB_MAX_BLOB_VALUE_LEN	ICV_BLOBVALUE_MAXLEN						// ���BLOB��������

// definition of the OPC Quality State
typedef struct
{
	unsigned short nLimit     : 2;		// 0..3   (2 bits)
	unsigned short nSubStatus : 4;		// 0..15  (4 bits)
	unsigned short nQuality   : 2;		// 0..3   (2 bits)
	unsigned short nLeftOver  : 8;
} QUALITY_STATE;

//////////////////////////////////////////////////////////////////////////

char GetDataType(const char *szDataype)
{
	char nDataType = 0;
	if(PKStringHelper::StriCmp(szDataype, DT_NAME_BOOL) == 0 )
		nDataType = TAG_DT_BOOL;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_CHAR) == 0)
		nDataType = TAG_DT_CHAR;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_UCHAR) == 0)
		nDataType = TAG_DT_UCHAR;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_INT16) == 0)
		nDataType = TAG_DT_INT16;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_UINT16) == 0)
		nDataType = TAG_DT_UINT16;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_INT32) == 0)
		nDataType = TAG_DT_INT32;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_UINT32) == 0)
		nDataType = TAG_DT_UINT32;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_INT64) == 0)
		nDataType = TAG_DT_INT64;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_UINT64) == 0)
		nDataType = TAG_DT_UINT64;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_FLOAT) == 0)
		nDataType = TAG_DT_FLOAT;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_DOUBLE) == 0)
		nDataType = TAG_DT_DOUBLE;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_TEXT) == 0)
		nDataType = TAG_DT_TEXT;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_TIME) == 0)
		nDataType = TAG_DT_TIME;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_TIME_HMST) == 0)
		nDataType = TAG_DT_TIMEHMST;
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_BLOB) == 0)
		nDataType = TAG_DT_BLOB;

	return nDataType;
}

char GetMaxDataLen(char nDataType)
{
	int nDataLen = 0;
	if(nDataType == TAG_DT_BOOL || nDataType == TAG_DT_CHAR || nDataType == TAG_DT_UCHAR)
		nDataLen = 1;
	else if(nDataType == TAG_DT_INT16 || nDataType == TAG_DT_UINT16)
		nDataLen = 2;
	else if(nDataType == TAG_DT_INT32 || nDataType == TAG_DT_UINT32 || nDataType == TAG_DT_FLOAT || nDataType == TAG_DT_TIMEHMST)
		nDataLen = 4;
	else if(nDataType == TAG_DT_INT64 || nDataType == TAG_DT_UINT64 || nDataType == TAG_DT_DOUBLE || nDataType == TAG_DT_TIME)
		nDataLen = 8;
	else if(nDataType == TAG_DT_TEXT)
		nDataLen = 127;
	else if(nDataType == TAG_DT_BLOB)
		nDataLen = 65535;

	return nDataLen;
}

long ReadAllTagsInfo(vector<TAGGROUP *> *vecTagGroup)
{
	// �γ������ļ�����·��
	char szConfigFile[PK_SHORTFILENAME_MAXLEN];
	char *szCVProjCfgPath = (char *)PKComm::GetConfigPath(); 
	if(szCVProjCfgPath)
		sprintf(szConfigFile, "%s%c%s",szCVProjCfgPath, ACE_DIRECTORY_SEPARATOR_CHAR, "taginfo.xml");
	else
		sprintf(szConfigFile, "%s", "taginfo.xml");

	// ���������ļ�xml��ʽ
	TiXmlElement* pElmChild = NULL;
	TiXmlDocument doc(szConfigFile);
	if( !doc.LoadFile() ) 
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Inner forward Driver Config File %s failed, errCode=%d!", szConfigFile, EC_ICV_DA_CONFIG_FILE_INVALID);
		return false;
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Load Config File %s Successful!", szConfigFile);

	// 2.Get Root Element.
	TiXmlElement* pElmRoot = doc.RootElement(); // "innerforward" node
	if(!pElmRoot)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Inner forward Driver Config File %s has no root Elem, errCode=%d!", szConfigFile, EC_ICV_DA_CONFIG_FILE_INVALID);
		return false;
	}

	TiXmlElement* pNodeDriver = pElmRoot->FirstChildElement("driver");
	while(pNodeDriver)
	{
		string strDrvName = NULLASSTRING(pNodeDriver->Attribute("name")); // ip or domain
		// ���Ҹ����͵�����. �����ǰ������Map��û��, δ�ҵ����½�һ��ȱʡ�ģ�iCV5/DDA/���·����
		DRIVERINFO *pDrv = NULL;
		DRIVERMAP::iterator itDrv = m_mapDrivers.find(strDrvName);
		if(itDrv == m_mapDrivers.end())
		{
			pDrv = new DRIVERINFO();
			pDrv->strAccessDLL = DRIVER_DEFAULT_ACEESSDLL_NAME;
			pDrv->strDrvName = strDrvName;
			pDrv->strDrvPath = ""; // ȱʡΪ�գ�ȡ���·��
			m_mapDrivers.insert(DRIVERMAP::value_type(strDrvName, pDrv));
		}
		else
			pDrv = itDrv->second;

		TiXmlElement* pNodeTag = pNodeDriver->FirstChildElement();
		while(pNodeTag)
		{
			string strTagName = NULLASSTRING(pNodeTag->Attribute("name")); // ip or domain
			string strDataType = NULLASSTRING(pNodeTag->Attribute("type")); // ip or domain
			string strTagAddr = NULLASSTRING(pNodeTag->Attribute("addr")); // devicename:blockname:addrinblock
			pNodeTag = pNodeTag->NextSiblingElement();

			// ���ɱ����ظ�����
			if(m_mapCVTags.find(strTagName) != m_mapCVTags.end())
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "read from config %s��tag %s duplicated", szConfigFile, strTagName.c_str());
				continue;
			}

			CVTAG *pCVTag = new CVTAG();
			pCVTag->strTagName = strTagName;		// TAG����
			pCVTag->strTagAddr = strTagAddr;
			pCVTag->nDataType = GetDataType(strDataType.c_str());	// ���õ�������������ͣ�������ģ����/������/BLOB/�ı�������Ҫ��

			// ���漸���Ǳ�����DIT�е�TAG����Ϣ
			pCVTag->pDrvIOAddr = new DRV_ADDR();		// ����DIT�еĵ�ַ
			memset(pCVTag->pDrvIOAddr, 0, sizeof(DRV_ADDR)); 

			// �����ǵ�ǰ��ȡ����ֵ����Ϣ
			memset(&pCVTag->rawData, 0, sizeof(pCVTag->rawData));
			memset(&pCVTag->tmStamp, 0, sizeof(pCVTag->tmStamp));		// ʱ���
			pCVTag->uQuality = -1;	// ��������

			pCVTag->pDrvInfo = pDrv; // ����������������Ϣ
			m_mapCVTags.insert(CVTAGMAP::value_type(strTagName, pCVTag));
		}

		pNodeDriver = pNodeDriver->NextSiblingElement();
	}
	
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "drivernum:%d, tagnum:%d", m_mapDrivers.size(), m_mapCVTags.size());

	// ������tag��ת�浽TagGroup�С�ÿ��TAGGroup���Դ�ŵ���󳤶�Ϊ1000���ֽ�
	int nGrpDataLen = 0;
	unsigned short uLocalGrpId = 0;
	if(m_mapCVTags.size() > 0)
	{
		TAGGROUP * pGroup = new TAGGROUP();
		pGroup->uLocalGrpId = ++ uLocalGrpId;
		vecTagGroup->push_back(pGroup);

		CVTAGMAP::iterator itTag = m_mapCVTags.begin();
		for(; itTag != m_mapCVTags.end(); itTag ++)
		{
			CVTAG *pTag = itTag->second;
			pGroup->vecTag.push_back(pTag);
			nGrpDataLen += GetMaxDataLen(pTag->nDataType);
			if(nGrpDataLen > 1000 && itTag != m_mapCVTags.end())
			{
				pGroup = new TAGGROUP();
				pGroup->uLocalGrpId = ++ uLocalGrpId;
				vecTagGroup->push_back(pGroup);
				nGrpDataLen = 0;
			} // if(nGrpDataLen > 1000)
		} // for(; itTag != m_mapCVTags.end(); itTag ++)
	} // if(m_mapCVTags.size() > 0)
	return 0;
}


// ��ȡtag���ֵ���������tag����
long ReadAllTagsData(vector<TAGGROUP *> *vecTagGroup)
{
	long lRet = 0;
	vector<TAGGROUP *>::iterator itGrp = vecTagGroup->begin();
	for(; itGrp != vecTagGroup->end(); itGrp ++)
	{
		TAGGROUP *pGrp = (TAGGROUP *)*itGrp;
		vector<CVTAG *>::iterator itTag = pGrp->vecTag.begin();
		for(; itTag != pGrp->vecTag.end(); itTag ++)
		{
			CVTAG *pTag = (CVTAG *) *itTag;
			if(!pTag->pDrvInfo || !pTag->pDrvInfo->pfnDDARead)
			{
				lRet = -3;
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Tag(%s), ����������, ��DDARead Func is null, RetCode: %d", pTag->strTagName.c_str(), lRet);
				continue;
			}

			memset(pTag->rawData.pszData, 0, pTag->rawData.nDataSize);
			lRet = pTag->pDrvInfo->pfnDDARead(pTag->pDrvIOAddr, &pTag->uQuality, &pTag->rawData, &pTag->tmStamp);
			if(lRet != PK_SUCCESS)
			{
				if (lRet == EC_ICV_DA_INVALID_IO_ADDR)
				{
					g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "TagName %s, Address[%s] Parse Failed, RetCode:%d!", 
						pTag->strTagName.c_str(), pTag->strTagAddr.c_str(), lRet);	

				}
				else // if(lRet != PK_SUCCESS)
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "TagName %s, Address[%s] Read Failed, RetCode:%d!", 
						pTag->strTagName.c_str(), pTag->strTagAddr.c_str(), lRet);		
				}

				continue;
			} // if(lRet != PK_SUCCESS)
		} // for(; itTag != pGrp->vecTag.end(); itTag ++)
	} // for(; itGrp != vecTagGroup->end(); itGrp ++)
	return 0;
}


// �������б����ĵ�ַ
long ParseAllTagsAddr()
{
	CVTAGMAP::iterator itTag = m_mapCVTags.begin();
	for(; itTag != m_mapCVTags.end(); itTag ++)
	{
		CVTAG *pTag = itTag->second;
		if(!pTag->pDrvInfo || !pTag->pDrvInfo->pfnDDAParse)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���� %s û�л�ȡ��������Ϣ��������Parse����", pTag->strTagName.c_str());
			continue;
		}

		// ���б�����ַ����
		long lRet = pTag->pDrvInfo->pfnDDAParse(pTag->pDrvIOAddr, pTag->strTagAddr.c_str(), pTag->nDataType);

		// ���ݱ������ͷ������ݳ���
		pTag->rawData.nDataSize = GetMaxDataLen(pTag->nDataType);
		pTag->rawData.nDataType = pTag->nDataType;
		pTag->rawData.pszData = new char[pTag->rawData.nDataSize];

	} // for(; itTag != g_pGlobalObj->m_mapCVTags.end(); itTag ++)
	return PK_SUCCESS;
}

// ��ʼ����������, ��������, ���������ĳ�ʼ����������ȡ����������DIT��ַ
long Initialize(vector<TAGGROUP *> *vecTagGroup)
{
	long lRet = ReadAllTagsInfo(vecTagGroup);
	
	// �����������DDA��������ÿ��TAG��ĵ�ַ
	string strExePath = PKComm::GetBinPath();
	
	// Ϊ�˷�ֹDDA������STLPort��ACE��DLL����ЩDLL��DDAĿ¼��û�У����ǽ���ǰ·���䵽DrvData.dll·����
	//SetCurrentDirectory(strExePath.c_str());
	ACE_OS::chdir(strExePath.c_str());
	//PKFileHelper::SetWorkingDirectory(strExePath.c_str());

	// ����ÿ��������
	DRIVERMAP::iterator itDrv = m_mapDrivers.begin();
	for(; itDrv != m_mapDrivers.end(); itDrv ++)
	{
		DRIVERINFO *pDrv = itDrv->second;
		// ����DDA.dll·����
		string strDrvFullPathName = strExePath;
		strDrvFullPathName += ACE_DIRECTORY_SEPARATOR_STR;
		strDrvFullPathName +=  "Drivers";
		strDrvFullPathName += ACE_DIRECTORY_SEPARATOR_STR;
		// ȱʡΪiCV5���������·��, Drivers/ModbusTCP/DDA
		strDrvFullPathName += pDrv->strDrvName;
		strDrvFullPathName += ACE_DIRECTORY_SEPARATOR_STR;
		strDrvFullPathName += pDrv->strAccessDLL;
		
		// ��������
		lRet = pDrv->hDrvDLL.open(strDrvFullPathName.c_str());
		if (lRet != PK_SUCCESS)
		{
			g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "�������� %s ʧ�ܣ����أ�%d", strDrvFullPathName.c_str(), lRet);
			continue;
		}

		// ��������
		pDrv->pfnDDAInit = (PFN_DDA_Init)pDrv->hDrvDLL.symbol(FNAME_PFN_DDA_Init);
		pDrv->pfnDDAParse = (PFN_DDA_Parse)pDrv->hDrvDLL.symbol(FNAME_PFN_DDA_parse);
		pDrv->pfnDDARead = (PFN_DDA_Read)pDrv->hDrvDLL.symbol(FNAME_PFN_DDA_read);
		pDrv->pfnDDASend = (PFN_DDA_Send)pDrv->hDrvDLL.symbol(FNAME_PFN_DDA_send);
		pDrv->pfnDDARelease = (PFN_DDA_Release)pDrv->hDrvDLL.symbol(FNAME_PFN_DDA_Release);
		if(!pDrv->pfnDDAParse|| !pDrv->pfnDDARead || !pDrv->pfnDDASend || !pDrv->pfnDDARelease)
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���� %s �б���ĵ�������û�л�ȡ��", strDrvFullPathName.c_str());

		if(!pDrv->pfnDDAInit)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���� %s �б���ĵ������� %s û�л�ȡ��", strDrvFullPathName.c_str(), FNAME_PFN_DDA_Init);
			continue;
		}

		lRet = pDrv->pfnDDAInit(pDrv->strDrvName.c_str());
		if(lRet != PK_SUCCESS)
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���� %s �б����ִ�к��� %s ���� %d", strDrvFullPathName.c_str(), FNAME_PFN_DDA_Init, lRet);

	} //for(; itDrv != g_pGlobalObj->m_mapDrivers.end(); itDrv ++)
	
	lRet = ParseAllTagsAddr();

	return lRet;
}


// �ͷŸ�������, ���������ĳ�ʼ������, UnLoad����, ��ȡ����������DIT��ַ
long UnInitialize()
{
	// ��Ҫ���ͷ�����ǰ�����ͷ�����TAG��,��ΪTAG��������ָ��
	CVTAGMAP::iterator itCVTag = m_mapCVTags.begin();
	for (; itCVTag != m_mapCVTags.end(); itCVTag++)
	{
		CVTAG *pTag = (*itCVTag).second;
		delete pTag;
		pTag = NULL;
	}
	m_mapCVTags.clear();

	// �ͷ�������Ϣ
	DRIVERMAP::iterator itDrv = m_mapDrivers.begin();
	for (; itDrv != m_mapDrivers.end(); itDrv++)
	{
		DRIVERINFO *pDrv = (*itDrv).second;
		if(pDrv->pfnDDARelease)
			try
		{
			pDrv->pfnDDARelease();
		}
		catch (...)
		{
		}

		delete pDrv;
		pDrv = NULL;
	}
	m_mapDrivers.clear();

	return PK_SUCCESS;
}
*/