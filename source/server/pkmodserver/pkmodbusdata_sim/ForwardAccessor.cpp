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

// 读取tag点的值，并保存回tag点中
MODBUSDATAAPI_EXPORTS int ReadTags(ModbusTagVec &modbusTagVec)
{
	char szValue[64] = { 0 };
	sprintf(szValue, "%dstring", g_nSimValue++);
	vector<ModbusTag *>::iterator itTag = modbusTagVec.begin(); // 是按照每个tag点对应的起始地址排序的。
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
	vector<ModbusTag *>::iterator itTag = modbusTagVec.begin(); // 是按照每个tag点对应的起始地址排序的。
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
DRIVERMAP	m_mapDrivers;	// 所有要用到的驱动的信息
CVTAGMAP	m_mapCVTags;	// 所有iCV驱动中配置的点的MAP
vector<TAGGROUP *>	m_vecTagGroup;	// TAG点分组存储以便

#define NULLASSTRING(x) x==NULL?"":x
#define DRIVER_DEFAULT_ACEESSDLL_NAME	"DDA"	// 缺省的访问驱动DLL名称
#define	FNAME_PFN_DDA_Init		ACE_TEXT("DDA_Init")
#define	FNAME_PFN_DDA_parse		ACE_TEXT("DDA_Parse")
#define	FNAME_PFN_DDA_Release	ACE_TEXT("DDA_Release")
#define	FNAME_PFN_DDA_read		ACE_TEXT("DDA_Read")
#define	FNAME_PFN_DDA_send		ACE_TEXT("DDA_Send")

#define TAG_MAXLINE_SIZE		2048
#define CV_MAX_NUMBER_DATATYPE_TOTAL 19
#define ICV_BLOBVALUE_MAXLEN 64512 
#define PDB_MAX_BLOB_VALUE_LEN	ICV_BLOBVALUE_MAXLEN						// 最大BLOB变量长度

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
	// 形成配置文件名称路径
	char szConfigFile[PK_SHORTFILENAME_MAXLEN];
	char *szCVProjCfgPath = (char *)PKComm::GetConfigPath(); 
	if(szCVProjCfgPath)
		sprintf(szConfigFile, "%s%c%s",szCVProjCfgPath, ACE_DIRECTORY_SEPARATOR_CHAR, "taginfo.xml");
	else
		sprintf(szConfigFile, "%s", "taginfo.xml");

	// 加载配置文件xml格式
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
		// 查找该类型的驱动. 如果当前按驱动Map中没有, 未找到则新建一个缺省的（iCV5/DDA/相对路径）
		DRIVERINFO *pDrv = NULL;
		DRIVERMAP::iterator itDrv = m_mapDrivers.find(strDrvName);
		if(itDrv == m_mapDrivers.end())
		{
			pDrv = new DRIVERINFO();
			pDrv->strAccessDLL = DRIVER_DEFAULT_ACEESSDLL_NAME;
			pDrv->strDrvName = strDrvName;
			pDrv->strDrvPath = ""; // 缺省为空，取相对路径
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

			// 生成变量重复变量
			if(m_mapCVTags.find(strTagName) != m_mapCVTags.end())
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "read from config %s，tag %s duplicated", szConfigFile, strTagName.c_str());
				continue;
			}

			CVTAG *pCVTag = new CVTAG();
			pCVTag->strTagName = strTagName;		// TAG名称
			pCVTag->strTagAddr = strTagAddr;
			pCVTag->nDataType = GetDataType(strDataType.c_str());	// 配置的请求的数据类型，包括：模拟量/数字量/BLOB/文本量。需要吗？

			// 下面几个是保存在DIT中的TAG点信息
			pCVTag->pDrvIOAddr = new DRV_ADDR();		// 驱动DIT中的地址
			memset(pCVTag->pDrvIOAddr, 0, sizeof(DRV_ADDR)); 

			// 下面是当前读取到的值的信息
			memset(&pCVTag->rawData, 0, sizeof(pCVTag->rawData));
			memset(&pCVTag->tmStamp, 0, sizeof(pCVTag->tmStamp));		// 时间戳
			pCVTag->uQuality = -1;	// 数据质量

			pCVTag->pDrvInfo = pDrv; // 驱动所属的驱动信息
			m_mapCVTags.insert(CVTAGMAP::value_type(strTagName, pCVTag));
		}

		pNodeDriver = pNodeDriver->NextSiblingElement();
	}
	
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "drivernum:%d, tagnum:%d", m_mapDrivers.size(), m_mapCVTags.size());

	// 把所有tag点转存到TagGroup中。每个TAGGroup可以存放的最大长度为1000个字节
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


// 读取tag点的值，并保存回tag点中
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
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Tag(%s), 驱动不存在, 或DDARead Func is null, RetCode: %d", pTag->strTagName.c_str(), lRet);
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


// 解析所有变量的地址
long ParseAllTagsAddr()
{
	CVTAGMAP::iterator itTag = m_mapCVTags.begin();
	for(; itTag != m_mapCVTags.end(); itTag ++)
	{
		CVTAG *pTag = itTag->second;
		if(!pTag->pDrvInfo || !pTag->pDrvInfo->pfnDDAParse)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "变量 %s 没有获取到驱动信息或驱动的Parse函数", pTag->strTagName.c_str());
			continue;
		}

		// 进行变量地址解析
		long lRet = pTag->pDrvInfo->pfnDDAParse(pTag->pDrvIOAddr, pTag->strTagAddr.c_str(), pTag->nDataType);

		// 根据变量类型分配数据长度
		pTag->rawData.nDataSize = GetMaxDataLen(pTag->nDataType);
		pTag->rawData.nDataType = pTag->nDataType;
		pTag->rawData.pszData = new char[pTag->rawData.nDataSize];

	} // for(; itTag != g_pGlobalObj->m_mapCVTags.end(); itTag ++)
	return PK_SUCCESS;
}

// 初始化各个驱动, 加载驱动, 调用驱动的初始化函数，获取各个变量的DIT地址
long Initialize(vector<TAGGROUP *> *vecTagGroup)
{
	long lRet = ReadAllTagsInfo(vecTagGroup);
	
	// 下面加载驱动DDA，并解析每个TAG点的地址
	string strExePath = PKComm::GetBinPath();
	
	// 为了防止DDA依赖于STLPort和ACE等DLL，这些DLL在DDA目录下没有，我们将当前路径射到DrvData.dll路径下
	//SetCurrentDirectory(strExePath.c_str());
	ACE_OS::chdir(strExePath.c_str());
	//PKFileHelper::SetWorkingDirectory(strExePath.c_str());

	// 加载每个驱动库
	DRIVERMAP::iterator itDrv = m_mapDrivers.begin();
	for(; itDrv != m_mapDrivers.end(); itDrv ++)
	{
		DRIVERINFO *pDrv = itDrv->second;
		// 驱动DDA.dll路径名
		string strDrvFullPathName = strExePath;
		strDrvFullPathName += ACE_DIRECTORY_SEPARATOR_STR;
		strDrvFullPathName +=  "Drivers";
		strDrvFullPathName += ACE_DIRECTORY_SEPARATOR_STR;
		// 缺省为iCV5的驱动相对路径, Drivers/ModbusTCP/DDA
		strDrvFullPathName += pDrv->strDrvName;
		strDrvFullPathName += ACE_DIRECTORY_SEPARATOR_STR;
		strDrvFullPathName += pDrv->strAccessDLL;
		
		// 加载驱动
		lRet = pDrv->hDrvDLL.open(strDrvFullPathName.c_str());
		if (lRet != PK_SUCCESS)
		{
			g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "加载驱动 %s 失败，返回：%d", strDrvFullPathName.c_str(), lRet);
			continue;
		}

		// 导出函数
		pDrv->pfnDDAInit = (PFN_DDA_Init)pDrv->hDrvDLL.symbol(FNAME_PFN_DDA_Init);
		pDrv->pfnDDAParse = (PFN_DDA_Parse)pDrv->hDrvDLL.symbol(FNAME_PFN_DDA_parse);
		pDrv->pfnDDARead = (PFN_DDA_Read)pDrv->hDrvDLL.symbol(FNAME_PFN_DDA_read);
		pDrv->pfnDDASend = (PFN_DDA_Send)pDrv->hDrvDLL.symbol(FNAME_PFN_DDA_send);
		pDrv->pfnDDARelease = (PFN_DDA_Release)pDrv->hDrvDLL.symbol(FNAME_PFN_DDA_Release);
		if(!pDrv->pfnDDAParse|| !pDrv->pfnDDARead || !pDrv->pfnDDASend || !pDrv->pfnDDARelease)
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "驱动 %s 中必需的导出函数没有获取到", strDrvFullPathName.c_str());

		if(!pDrv->pfnDDAInit)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "驱动 %s 中必需的导出函数 %s 没有获取到", strDrvFullPathName.c_str(), FNAME_PFN_DDA_Init);
			continue;
		}

		lRet = pDrv->pfnDDAInit(pDrv->strDrvName.c_str());
		if(lRet != PK_SUCCESS)
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "驱动 %s 中必需的执行函数 %s 返回 %d", strDrvFullPathName.c_str(), FNAME_PFN_DDA_Init, lRet);

	} //for(; itDrv != g_pGlobalObj->m_mapDrivers.end(); itDrv ++)
	
	lRet = ParseAllTagsAddr();

	return lRet;
}


// 释放各个驱动, 调用驱动的初始化函数, UnLoad驱动, 获取各个变量的DIT地址
long UnInitialize()
{
	// 需要中释放驱动前，先释放所有TAG点,因为TAG点有驱动指针
	CVTAGMAP::iterator itCVTag = m_mapCVTags.begin();
	for (; itCVTag != m_mapCVTags.end(); itCVTag++)
	{
		CVTAG *pTag = (*itCVTag).second;
		delete pTag;
		pTag = NULL;
	}
	m_mapCVTags.clear();

	// 释放驱动信息
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