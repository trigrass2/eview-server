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
#define COL_DATATYPE_ID_STRING					1			// 字符串类型, 包含int;
#define COL_DATATYPE_ID_INT						2			// 整数类型，包含string，text或char;
#define COL_DATATYPE_ID_REAL					3			// 日期时间类型，包含date，time;
#define COL_DATATYPE_ID_DATETIME				4			// 日期时间类型，包含date，time;
#define COL_DATATYPE_ID_UNKNOWN					0			//未知;

#define COL_DATATYPE_NAME_STRING				"string"	// 字符串类型;
#define COL_DATATYPE_NAME_INT					"int"		// 整数类型;
#define COL_DATATYPE_NAME_REAL					"real"		// 小数类型;
#define COL_DATATYPE_NAME_DATETIME				"datetime"	// 日期时间类型;

// 不计算、hour_diff、day_diff、month_diff、quarter_diff、year_diff几种方法
// 计算差值定时进行计算，并更新到数据库。如果需要取历史，则从固定的表：t_diff_hour表中根据时间获取上个时间段的值。只有那些需要进行累计统计的tag才需要保存到该表中。该表中保存的主键是tag名称或对象.属性名称
#define COL_CALCMETHOD_RAW_VALUE				0			// 不计算;
#define COL_CALCMETHOD_HOUR_DIFF				1			// 取小时差值（区间值，往往用于能耗统计）;
#define COL_CALCMETHOD_DAY_DIFF					2			// 取日差值（区间值，往往用于能耗统计）;
#define COL_CALCMETHOD_WEEK_DIFF				3			// 取周差值（区间值，往往用于能耗统计）;
#define COL_CALCMETHOD_MONTH_DIFF				4			// 取月差值（区间值，往往用于能耗统计）;
#define COL_CALCMETHOD_QUARTER_DIFF				5			// 取季度差值（区间值，往往用于能耗统计）;
#define COL_CALCMETHOD_YEAR_DIFF				6			// 取年差值（区间值，往往用于能耗统计）;

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
	map<string, string>		m_mapTableCreated2Time;	// 已经创建的表;

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
	string m_strColName;	// 该列的值;
	int m_nDataType;
	string m_strTimeFormt;  // 如果是日期时间格式，则需要格式描述;
	int m_nCalcMethod;		// 计算类型。不计算、hour_diff、day_diff、month_diff、quarter_diff、year_diff几种方法
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
		int nPos = strColNameWithMethod.find("_diff("); // 没有找到
		if (nPos == string::npos)
		{
			m_strColName = strColNameWithMethod;
			return 0;
		}

		string strMethodType = strColNameWithMethod.substr(0, nPos); // day,hour,...
		m_strColName = strColNameWithMethod.substr(nPos + strlen("_diff(")); 
		if (m_strColName.length() >= 1)
			m_strColName = m_strColName.substr(0, m_strColName.length() - 1); //去掉最后1个括号

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
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "不支持的累积量计算方法:%s, colinfo:%s, 缺省设置为变量原始值, 请检查! 支持:hour_diff(),day_diff(),week_diff(),month_diff(),quarter_diff(),year_diff()",
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
	string m_strTagOrPropName;	// 暂存列的值，可能是tag点名称;
	int m_nQuality;		// 暂存当前TAG点的质量，0为GOOD
	string m_strTagValue;	// 暂存当前的TAG点的值

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

// 每一个tag点;
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

public: // 一次性要读取的对象属性，线准备好避免后续再生成影响效率;
	PKDATA *	m_pkTagDatas;
	int			m_nTagDataNum;

public:
	CDBTransferRuleTags()
	{
		m_strId = m_strRuleId = "";
		m_vecTag.clear(); 
		m_pkTagDatas = NULL;
	}
	
	// 初始化调用PKData的参数;
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

public: // 一次性要读取的对象属性，线准备好避免后续再生成影响效率;
	PKDATA *	m_pkTagDatas;
	int			m_nTagDataNum;

public:
	CDBTransferRuleObjProps()
	{
		m_strId = m_strRuleId = m_strObjId = "";
		m_strObjName = "";
		m_vecProp.clear();
	}

	// 初始化调用PKData的参数;
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

// 固定列， 配置在FieldName_FixCols字段的解析，字段内容应该是：{colname,datatype,colvalue} || {colname,colvalue};
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
	string m_strDBConnId; // 数据库连接的ID
	string m_strTableName;
	string	m_strSavePeriod;
	bool	m_bWriteOnChange;
	string m_strDescription;
	string m_strWriteMethod; //写入方法，real/history;
	bool   m_bIsRTTable; // 是否是按照实时表格式写入,根据m_strWriteMethod而来;

	CColInfo m_colObjName;				// 对象模式下，对象名称列;
	CColInfo m_colTagName;				// 设备模式下，变量名称列的列名;
	CColInfo m_colQuality;				// 保存质量字段名称和数据类型, 缺省为：data_quality，一定是数值型;
	CColInfo m_colDateTime;				// 保存数据值字段的名称，缺省为：data_time;
	CColInfo m_colUpdateTime;			// 保存数据值字段的名称，缺省为：update_time;

	CDBConnection *m_pDbConn;				// 每个规则对应的数据库连接;
	vector<CFixCol *> m_vecFixCol;			// 保存固定列头及列值的数组;
	vector<CColInfo *> m_vecColTagConfig;	// 设备tag模式下，***各个列头元信息***对应的变量放在不同列的多个列的定义, 不同列放不同的tag点
	vector<CColInfo *> m_vecColPropConfig;	// 对象模式下，***各个列头元信息***对应的对象各个属性的数组

	vector<CDBTransferRuleObjProps *>	m_vecObjProps;	// 对象模式时，***每一行***记录的对象属性信息 保存在这个vector中
	vector<CDBTransferRuleTags *>		m_vecTags;	// 设备模式时，***每一行***tag的属性保存在这个vector中

	vector<string>	m_vecStatTagName;		// 做差值相减等统计值所用的tag点或对象.属性

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
