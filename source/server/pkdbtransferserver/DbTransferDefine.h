/**************************************************************
*  Filename:    MainTask.h
*  Copyright:   Shanghai peakinfo Software Co., Ltd.
*
*  Description: mqtt read redis and publish to topic.
*
*  @author:     shijunpu
*  @version     04/16/2018  shijunpu  Initial Version
**************************************************************/

#ifndef _DBTRANSFER_DEFINE_H_
#define _DBTRANSFER_DEFINE_H_

#include "pkdbapi/pkdbapi.h"
#include "pklog/pklog.h"
#include <vector>
using namespace std;
#define COL_DATATYPE_ID_STRING					1			// �ַ�������, ����int;
#define COL_DATATYPE_ID_INT						2			// �������ͣ�����string��text��char;
#define COL_DATATYPE_ID_REAL					3			// ����ʱ�����ͣ�����date��time;
#define COL_DATATYPE_ID_DATETIME				4			// ����ʱ�����ͣ�����date��time;
#define COL_DATATYPE_ID_UNKNOWN					0			//δ֪;

#define COL_DATATYPE_NAME_STRING				"string"	// �ַ�������;
#define COL_DATATYPE_NAME_INT					"int"		// ��������;
#define COL_DATATYPE_NAME_REAL					"real"		// С������;
#define COL_DATATYPE_NAME_DATETIME				"datetime"	// ����ʱ������;

// �����㡢hour_diff��day_diff��month_diff��quarter_diff��year_diff���ַ���
// �����ֵ��ʱ���м��㣬�����µ����ݿ⡣�����Ҫȡ��ʷ����ӹ̶��ı�t_diff_hour���и���ʱ���ȡ�ϸ�ʱ��ε�ֵ��ֻ����Щ��Ҫ�����ۼ�ͳ�Ƶ�tag����Ҫ���浽�ñ��С��ñ��б����������tag���ƻ����.��������
#define COL_CALCMETHOD_RAW_VALUE				0			// ������;
#define COL_CALCMETHOD_HOUR_DIFF				1			// ȡСʱ��ֵ������ֵ�����������ܺ�ͳ�ƣ�;
#define COL_CALCMETHOD_DAY_DIFF					2			// ȡ�ղ�ֵ������ֵ�����������ܺ�ͳ�ƣ�;
#define COL_CALCMETHOD_WEEK_DIFF				3			// ȡ�ܲ�ֵ������ֵ�����������ܺ�ͳ�ƣ�;
#define COL_CALCMETHOD_MONTH_DIFF				4			// ȡ�²�ֵ������ֵ�����������ܺ�ͳ�ƣ�;
#define COL_CALCMETHOD_QUARTER_DIFF				5			// ȡ���Ȳ�ֵ������ֵ�����������ܺ�ͳ�ƣ�;
#define COL_CALCMETHOD_YEAR_DIFF				6			// ȡ���ֵ������ֵ�����������ܺ�ͳ�ƣ�;

#include <map>
#include <string>
using namespace std;

extern CPKLog g_logger;

int DatatypeToId(const char *szDataType);

class CDBTransferRule;
class CDBConnection
{
public:
	string m_strId;
	string m_strName;
	string m_strDbType;
	string m_strConnString;
	string m_strUserName;
	string m_strPassword;
	string m_strDescription;
	string m_strCodeset;
	int		m_nDBType;
	CPKDbApi m_pkdb;
	vector<CDBTransferRule *> m_vecTransferRule;
	map<string, string>		m_mapTableCreated2Time;	// �Ѿ������ı�;

public:
	CDBConnection()
	{
		m_strId = m_strName = m_strDbType = m_strConnString = m_strUserName = m_strPassword = m_strDescription = m_strCodeset = "";
	}
	~CDBConnection()
	{
		m_pkdb.UnInitialize();
	}
};

class CColInfo
{
public:
	string m_strColName;	// ���е�ֵ;
	int m_nDataType;
	string m_strTimeFormt;  // ���������ʱ���ʽ������Ҫ��ʽ����;
	int m_nCalcMethod;		// �������͡������㡢hour_diff��day_diff��month_diff��quarter_diff��year_diff���ַ���
public:
	CColInfo()
	{
		m_nDataType = COL_DATATYPE_ID_STRING;
		m_strColName = "";
		m_strTimeFormt = "yyyy-mm-dd HH:MI:SS.xxx ";
		m_nCalcMethod = COL_CALCMETHOD_RAW_VALUE;
	}
public:
	int ExtractMethod(string &strColNameWithMethod) // lishi, day_diff(lishi)
	{
		int nPos = strColNameWithMethod.find("_diff("); // û���ҵ�
		if (nPos == string::npos)
		{
			m_strColName = strColNameWithMethod;
			return 0;
		}

		string strMethodType = strColNameWithMethod.substr(0, nPos); // day,hour,...
		m_strColName = strColNameWithMethod.substr(nPos + strlen("_diff(")); 
		if (m_strColName.length() >= 1)
			m_strColName = m_strColName.substr(0, m_strColName.length() - 1); //ȥ�����1������

		if (PKStringHelper::StriCmp(strMethodType.c_str(), "hour") == 0)
			m_nCalcMethod = COL_CALCMETHOD_HOUR_DIFF;
		else if (PKStringHelper::StriCmp(strMethodType.c_str(), "day") == 0)
			m_nCalcMethod = COL_CALCMETHOD_DAY_DIFF;
		else if (PKStringHelper::StriCmp(strMethodType.c_str(), "week") == 0)
			m_nCalcMethod = COL_CALCMETHOD_WEEK_DIFF;
		else if (PKStringHelper::StriCmp(strMethodType.c_str(), "month") == 0)
			m_nCalcMethod = COL_CALCMETHOD_MONTH_DIFF;
		else if (PKStringHelper::StriCmp(strMethodType.c_str(), "quarter") == 0)
			m_nCalcMethod = COL_CALCMETHOD_QUARTER_DIFF;
		else if (PKStringHelper::StriCmp(strMethodType.c_str(), "year") == 0)
			m_nCalcMethod = COL_CALCMETHOD_YEAR_DIFF;
		else
		{
			m_nCalcMethod = COL_CALCMETHOD_RAW_VALUE;
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��֧�ֵ��ۻ������㷽��:%s, colinfo:%s, ȱʡ����Ϊ����ԭʼֵ, ����! ֧��:hour_diff(),day_diff(),week_diff(),month_diff(),quarter_diff(),year_diff()",
				strMethodType.c_str(), strColNameWithMethod.c_str());
			return -100;
		}
		return 0;
	}

	int ParseFrom(string strColMeta) // fieldname,datatype,[timeformat]   lishi,string;lishi2,string;
	{
		vector<string> vecColInfo = PKStringHelper::StriSplit(strColMeta.c_str(), ",");
		if (vecColInfo.size() <= 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "coldefine:%s, must be:{colname,datatype,colvalue} || {colname,colvalue}", strColMeta.c_str());
			return -1;
		}

		string strColNameWithMethod = vecColInfo[0];
		ExtractMethod(strColNameWithMethod);

		if (vecColInfo.size() >= 2) //colname, datatype
		{
			string strDataType = vecColInfo[1];
			m_nDataType = DatatypeToId(strDataType.c_str());
		}

		if (vecColInfo.size() >= 3) //timeformat
		{
			m_strTimeFormt = vecColInfo[2];
		}
		return 0;
	}
};

class CColValue
{
public:
	string m_strTagOrPropName;	// �ݴ��е�ֵ��������tag������;
	int m_nQuality;		// �ݴ浱ǰTAG���������0ΪGOOD
	string m_strTagValue;	// �ݴ浱ǰ��TAG���ֵ

public:
	CColValue()
	{
		m_nQuality = 0;
		m_strTagOrPropName =  m_strTagValue = "";
	}
	CColValue & operator=(const CColValue &x)
	{
		m_nQuality = x.m_nQuality;
		m_strTagOrPropName = x.m_strTagOrPropName;
		m_strTagValue = x.m_strTagValue;
		return *this;
	}

	CColValue(const CColValue &x)
	{
		*this = x;
	}
};

// ÿһ��tag��;
class CTagInfo
{
public:
	string m_strTagId;
	string m_strTagName;
public:
	CTagInfo()
	{
		m_strTagId = m_strTagName = "";
	}
};
class CPropInfo
{
public:
	string m_strPropId;
	string m_strPropName;
public:
	CPropInfo()
	{
		m_strPropId = m_strPropName = "";
	}
};

class CDBTransferRuleTags
{
public:
	string m_strId;
	string m_strRuleId;
	vector<CTagInfo *> m_vecTag;

public: // һ����Ҫ��ȡ�Ķ������ԣ���׼���ñ������������Ӱ��Ч��;
	PKDATA *	m_pkTagDatas;
	int			m_nTagDataNum;

public:
	CDBTransferRuleTags()
	{
		m_strId = m_strRuleId = "";
		m_vecTag.clear(); 
		m_pkTagDatas = NULL;
	}
	
	// ��ʼ������PKData�Ĳ���;
	int InitPKData()
	{ 
		m_nTagDataNum = m_vecTag.size();
		if (m_nTagDataNum > 0)
		{
			m_pkTagDatas = new PKDATA[m_nTagDataNum]();
			for (int i = 0; i < m_vecTag.size(); i++)
			{
				PKStringHelper::Safe_StrNCpy(m_pkTagDatas[i].szObjectPropName, m_vecTag[i]->m_strTagName.c_str(), sizeof(m_pkTagDatas[i].szObjectPropName));
				strcpy(m_pkTagDatas[i].szFieldName, "");
			}
		}
		return 0;
	}
};


class CDBTransferRuleObjProps
{
public:
	string m_strId;
	string m_strRuleId;
	string m_strObjId;
	string m_strObjName;
	vector<CPropInfo *> m_vecProp;

public: // һ����Ҫ��ȡ�Ķ������ԣ���׼���ñ������������Ӱ��Ч��;
	PKDATA *	m_pkTagDatas;
	int			m_nTagDataNum;

public:
	CDBTransferRuleObjProps()
	{
		m_strId = m_strRuleId = m_strObjId = "";
		m_strObjName = "";
		m_vecProp.clear();
	}

	// ��ʼ������PKData�Ĳ���;
	int InitPKData()
	{
		m_nTagDataNum = m_vecProp.size();
		if (m_nTagDataNum > 0)
		{
			m_pkTagDatas = new PKDATA[m_nTagDataNum]();
			for (int i = 0; i < m_vecProp.size(); i++)
			{
				string strObjPropName = m_strObjName + "." + m_vecProp[i]->m_strPropName;
				PKStringHelper::Safe_StrNCpy(m_pkTagDatas[i].szObjectPropName, strObjPropName.c_str(), sizeof(m_pkTagDatas[i].szObjectPropName));
				strcpy(m_pkTagDatas[i].szFieldName, "");
			}
		}
		return 0;
	}
};

// �̶��У� ������FieldName_FixCols�ֶεĽ������ֶ�����Ӧ���ǣ�{colname,datatype,colvalue} || {colname,colvalue};
class CFixCol
{
public:
	CFixCol() 
	{
		m_strColName = "";
		m_strColValue = "";
		m_nDataType = COL_DATATYPE_ID_STRING;
	}
public:
	int ParseFrom(string strColMeta) // colname,datatype,colvalue;
	{
		vector<string> vecColInfo = PKStringHelper::StriSplit(strColMeta.c_str(), ",");
		if (vecColInfo.size() <= 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "coldefine:%s, must be:{colname,datatype,colvalue} || {colname,colvalue}", strColMeta.c_str());
			return -1;
		}

		m_strColName = vecColInfo[0];

		if (vecColInfo.size() >= 2) //colname, datatype, colvalue;
		{
			string strDataType = vecColInfo[1];
			m_nDataType = DatatypeToId(strDataType.c_str());
		}
		
		if (vecColInfo.size() >= 3) //colname, datatype, colvalue;
		{
			m_strColValue = vecColInfo[2];
		}
		return 0;
	}
public:
	string m_strColName;
	int		m_nDataType;
	string	m_strColValue;
};

class CDBTransferRule
{
public:
	string m_strId;
	string m_strName;
	string m_strDBConnId; // ���ݿ����ӵ�ID
	string m_strTableName;
	string	m_strSavePeriod;
	bool	m_bWriteOnChange;
	string m_strDescription;
	string m_strWriteMethod; //д�뷽����real/history;
	bool   m_bIsRTTable; // �Ƿ��ǰ���ʵʱ���ʽд��,����m_strWriteMethod����;

	CColInfo m_colObjName;				// ����ģʽ�£�����������;
	CColInfo m_colTagName;				// �豸ģʽ�£����������е�����;
	CColInfo m_colQuality;				// ���������ֶ����ƺ���������, ȱʡΪ��data_quality��һ������ֵ��;
	CColInfo m_colDateTime;				// ��������ֵ�ֶε����ƣ�ȱʡΪ��data_time;
	CColInfo m_colUpdateTime;			// ��������ֵ�ֶε����ƣ�ȱʡΪ��update_time;

	CDBConnection *m_pDbConn;				// ÿ�������Ӧ�����ݿ�����;
	vector<CFixCol *> m_vecFixCol;			// ����̶���ͷ����ֵ������;
	vector<CColInfo *> m_vecColTagConfig;	// �豸tagģʽ�£�***������ͷԪ��Ϣ***��Ӧ�ı������ڲ�ͬ�еĶ���еĶ���, ��ͬ�зŲ�ͬ��tag��
	vector<CColInfo *> m_vecColPropConfig;	// ����ģʽ�£�***������ͷԪ��Ϣ***��Ӧ�Ķ���������Ե�����

	vector<CDBTransferRuleObjProps *>	m_vecObjProps;	// ����ģʽʱ��***ÿһ��***��¼�Ķ���������Ϣ ���������vector��
	vector<CDBTransferRuleTags *>		m_vecTags;	// �豸ģʽʱ��***ÿһ��***tag�����Ա��������vector��

	vector<string>	m_vecStatTagName;		// ����ֵ�����ͳ��ֵ���õ�tag������.����

public:
	CDBTransferRule()
	{
		m_strId = m_strName = m_strSavePeriod = m_strDescription = m_strWriteMethod = m_strDBConnId = "";
		//m_strFieldName_ObjName = m_strFieldName_PropName = m_strFieldName_TagName = m_strFieldName_Value = "";
		m_bWriteOnChange = false;
		m_bIsRTTable = true;
		m_pDbConn = NULL;
		//m_nDataType_DataTime = COL_DATATYPE_ID_STRING;
		//m_nDataType_UpdateTime = COL_DATATYPE_ID_STRING;
		m_colQuality.m_nDataType = COL_DATATYPE_ID_INT;
		m_colDateTime.m_nDataType = m_colUpdateTime.m_nDataType = COL_DATATYPE_ID_STRING;
		m_colDateTime.m_strTimeFormt = m_colUpdateTime.m_strTimeFormt = "yyyy-mm-dd HH:MI:SS.xxx";
		m_colObjName.m_nDataType = COL_DATATYPE_ID_STRING;
		m_strTableName = "t_db_transfer_data";
	}

	int BuildStatTags()
	{
		for (int iCol = 0; iCol < m_vecColPropConfig.size()-1; iCol++)
		{
			CColInfo *pColInfo = m_vecColPropConfig[iCol];
			if (pColInfo->m_nCalcMethod == COL_CALCMETHOD_RAW_VALUE)
				continue;

			for (int i = 0; i < m_vecObjProps.size(); i++)
			{
				CDBTransferRuleObjProps *pObjProps = m_vecObjProps[i];
				string strObjName = pObjProps->m_strObjName;
				string strPropName = pObjProps->m_vecProp[iCol]->m_strPropName;
				string strTagName = strObjName + "." + strPropName;
				m_vecStatTagName.push_back(strTagName);
			}
		}

		for (int iCol = 0; iCol < m_vecColTagConfig.size(); iCol++)
		{
			CColInfo *pColInfo = m_vecColTagConfig[iCol];
			//if (pColInfo->m_nCalcMethod == COL_CALCMETHOD_RAW_VALUE)
			//	continue;

			for (int i = 0; i < m_vecTags.size(); i++)
			{
				CDBTransferRuleTags *pTags = m_vecTags[i];
				if (pTags->m_vecTag.size() <= iCol)
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag config invalid, rule: %s, LineNo:%d, tag count:%d < col count:%d", m_strName.c_str(), i, pTags->m_vecTag.size(), iCol);
					continue;
				}

				string strTagName = pTags->m_vecTag[iCol]->m_strTagName;
				m_vecStatTagName.push_back(strTagName);
			}
		}
		return 0;
	}

};

#endif  // _DBTRANSFER_DEFINE_H_
