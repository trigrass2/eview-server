#ifndef _CRS_SERVICE_HXX_DEF_
#define	_CRS_SERVICE_HXX_DEF_

#include <ace/ACE.h>
#include <ace/SOCK_Stream.h>
#include <ace/Event_Handler.h>
#include <ace/Null_Mutex.h>
#include <ace/Synch.h>
#include <ace/Message_Queue.h>
#include <ace/Singleton.h>
#include <ace/Task.h>
#include "common/OS.h"
#include <ace/Get_Opt.h>
#include <ace/Containers.h>

#include "link/linkdefine.h"
#include "link/CrsApiServerDefines.h"
//#include "response/CRSAPI.h"
#include "link/CRSAPIDef.h"
#include "common/pkGlobalHelper.h"
#include "common/pkcomm.h"
#include "common/RMAPI.h"
#include "common/CommHelper.h"
#include "errcode/ErrCode_iCV_Common.hxx"
#include "errcode/ErrCode_iCV_CRS.h"
#include "common/CVNDK.h"
#include "pklog/pklog.h"
#include "event/eventapi.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include "tinyxml/tinyxml.h"
#include "tinyxml/tinystr.h"

// get LogLevel according to return Value
#define CRS_LOGLEVEL(x)	(x==ICV_SUCCESS? LOG_LEVEL_INFO : LOG_LEVEL_ERROR)

// assure that NULL string converted to "", to avoid TiXMLElement::SetAttribute(x, NULL) exception sometimes
#define	CRS_STRING(x)	(x == NULL ? "" : x)

#define CRS_SIZEOF_1K								    1024
#define CV_CRSAUTHORITY_MAXLEN							256
#define CV_CRSPERKILO_NUMBER							1000
//#define SCHEMEITEM_TIMEOUT_DEFAULT_VALUE				3000		// 预案项缺省的等待超时时间
#define CLIENT_CONNECT_TIMEOUT_VALUE					(CLIENT_CONNECT_HANDTIMEINTERVAL * 3)		// 客户端连接超时时限

#define ICV_CRS_EVENTBUFF_MAXLEN						1024
// 主要用于联动系统的定义
#define ICV_RESPONSEVALUE_MAXLEN						256	// ICV联动系统动作响应值的最大长度

#define EVENTNODENAME	"CRSNODE"
#define APPLICATIONNAME "CRService"

#define THREAD_COUNT_IN_CLIENTMANAGE_TASK 1  //only one thread currently

#define CRS_INVALIDE_ID								0
#define SCHEMEITEM_INIT_INDEX						0
#define SCHEMEITEM_OVER_INDEX						-1
#define SCHEMEITEM_STOP_INDEX						0x7FFFFFFF
#define CRS_CATEGOERY_DEPTH							2

#define SCHEMEITEM_CONFIRMSTATE_NONE				0		// 预案项未确认
#define SCHEMEITEM_CONFIRMSTATE_CONFIRMED			1		// 预案项已确认
#define SCHEMEITEM_CONFIRMSTATE_CANCELED			2		// 预案项已取消

#define CLIENTACTIONRESULT_INIT						0
#define CLIENTACTIONRESULT_FINISH_SUCCESS			1
#define CLIENTACTIONRESULT_FINISH_FAIL				2
#define CV_CRSPARAVALUE_MAXLEN						128
#define CV_CRSPARADESC_MAXLEN						256
#define CV_CRSCOMMNAME_MAXLEN						128
#define CV_CRSDESC_MAXLEN							128
#define CV_CRSCOMMNAME_MAXLEN						128
#define CV_DESTHMIIPS_MAXLEN						256
#define CV_CRS_DESC_MAXLEN							128
#define CV_CRS_TRIGGERCFG_MAXLEN					128
extern void CRSEvent(const char *szOperator, const char *szSubType, const char *szComment, const char *szFormat, ...);
/*
typedef struct tagPRIORITY
{
	int		nID;             //ID for Priority
	char	szPriorityName[CV_CRSCOMMNAME_MAXLEN];  //Name
	char	szPriorityDesc[CV_CRSDESC_MAXLEN];	 //Description
	tagPRIORITY(){memset(this, 0x00, sizeof(struct tagPRIORITY));}
}CRS_PRIORITY;
*/
typedef struct tagCATEGORY
{
	int		nID;							    //ID for Priority
	char	szCatName[CV_CRSCOMMNAME_MAXLEN];   //Name
	int		nParentID;							//Parent ID
	tagCATEGORY(){memset(this, 0x00, sizeof(struct tagCATEGORY));}
}CRS_CATEGORY;

typedef struct tagACTIONTYPE
{
	//CREATE TABLE t_crs_schemeitemtypes(
	int		nActionTypeID;								  // fd_itemtype_id 		smallint 	PRIMARY KEY, 
	char	szActionItemTypeName[CV_CRSCOMMNAME_MAXLEN];	  //fd_itemtype_name 	varchar(32) 	NOT NULL,
	char	szActionDllName[ICV_LONGFILENAME_MAXLEN];   //fd_itemtype_handledll 	varchar(260)	NOT NULL,
	unsigned short	bRunOnServer;					  //fd_is_runatscada	smallint 	NOT NULL,
	char	szActionItemTypeDesc[CV_CRSDESC_MAXLEN];		  //fd_itemtype_desc 	varchar(128)
	tagACTIONTYPE(){memset(this, 0x00, sizeof(struct tagACTIONTYPE));}
}LINK_ACTIONTYPE;

typedef struct tagINFO
{
	int nTagID;
	char szName[CV_CRSCOMMNAME_MAXLEN];
	char szAddress[PK_DESC_MAXLEN];
	char szCName[PK_DESC_MAXLEN];
	tagINFO(){memset(this, 0x00, sizeof(struct tagINFO));}
}TAGINFO;

typedef struct tagUsenableTagCurInfo
{
	char szTagName[CV_CRSCOMMNAME_MAXLEN];    //tag点名
	int nCurPriority;      // 当前占用的优先级
	bool bLastUsed;        // 是否是最新一次被使用的
	char szLastValue[CV_CRSCOMMNAME_MAXLEN];    // 上一次占用该资源的值，
	int nLastAcionID;     // 就写actionid吧
	tagUsenableTagCurInfo()
	{
		nLastAcionID = -1;
		nCurPriority = -1;
		bLastUsed = 0;
		memset(szTagName, 0x00, sizeof(szTagName));
		memset(szLastValue, 0x00, sizeof(szLastValue));
	}
}USABLETAGCURINFO;

typedef struct tagUsableTags
{
	int nID;
	char szName[CV_CRSCOMMNAME_MAXLEN];
	vector<USABLETAGCURINFO> vecUsenableTagCurInfos;	

	tagUsableTags(){memset(this, 0x00, sizeof(struct tagUsableTags));}
}USABLETAG;

typedef struct tagACTION
{
	//CREATE TABLE t_crs_schemeitems(
	int		nSchemeID;									//fd_scheme_id 		smallint 	NOT NULL,
	int		nActionOrder;									//fd_item_index 		smallint 	NOT NULL,
	int		nActionID;									//fd_item_id 		smallint 	PRIMARY KEY , 
	unsigned short  bSkipped;							//fd_skipped 		smallint 	NOT NULL,
	unsigned short  bNeedManualConfirm;						//fd_manuconfirmed 	smallint 	NOT NULL,
	int		nWaitConfirmTime;							//fd_waitconfirm_time 	smallint,
	int		nIsRunOnServer;
	unsigned short  bIsExecuteOnConfirmTimeOut;					//fd_timeout_execed 	smallint 	NOT NULL,
	char	szActionName[CV_CRSCOMMNAME_MAXLEN];			//fd_item_name 		varchar(32) 	NOT NULL,
	char	szActionParam[2048];	//fd_response_value 	varchar(128) 	NOT NULL,
	char	szExecteUserList[CV_DESTHMIIPS_MAXLEN];			//fd_desthmi_iplist	varchar(256),
	char	szActionDll[ICV_RESPONSEVALUE_MAXLEN];    
	char	szActionDesc[CV_CRSDESC_MAXLEN];				//fd_item_desc		varchar(128),
	int		nActionCatID;									//fd_itemset_id		smallint	NOT NULL,
	unsigned short		bActionValid;						//fd_item_valid		smallint	NOT NULL
    tagACTION(){memset(this, 0x00, sizeof(struct tagACTION));}
}LINK_ACTION;

typedef struct tagPARAMETER_VALUE
{
	int nTriggerID;										//fd_trigger_id	smallint NOT NULL,
	char szParaName[CV_CRSCOMMNAME_MAXLEN];				//fd_para_name	varchar(32)	NOT NULL,
	char szParaValue[CV_CRSPARAVALUE_MAXLEN];			//fd_para_value	varchar(64),
	char szParaDesc[CV_CRSPARADESC_MAXLEN];				//fd_para_desc	varchar(128),
//	int nSchemeID;										//fd_scheme_id smallint not null,
//	int nItemID;										//fd_item_id smallint not null,
	tagPARAMETER_VALUE(){memset(this, 0x00, sizeof(struct tagPARAMETER_VALUE));}
}CRS_PARAMETER_VALUE;

typedef struct tagSCHEME
{
	int		nID;										//fd_scheme_id 		smallint 	PRIMARY KEY , 
	char    szSchemeName[CV_CRSCOMMNAME_MAXLEN];		//fd_scheme_name 		varchar(32) 	NOT NULL,
	char    szSchemeDesc[CV_DESTHMIIPS_MAXLEN];			//fd_scheme_desc 		varchar(128),
	int		nCatID;										//fd_schemeset_id		smallint 	NOT NULL,
	//unsigned short bManuTrigger;						//fd_scheme_manutrigger	smallint	NOT NULL,
	//char	szAuthorGroup[CV_CRSAUTHORITY_MAXLEN];		//fd_scheme_groups	varchar(256),
	unsigned short uPriority;							//fd_priority_id		smallint	NOT NULL,
	unsigned short bDisplayOnClient;								//fd_process_cansee	smallint	NOT NULL,
	unsigned short bCanStop;							//fd_process_canstop	smallint	NOT NULL,
	unsigned short bSingleConfirm;						//fd_scheme_singlecfm	smallint	NOT NULL,
	unsigned short bValid;								//fd_scheme_valid		smallint	NOT NULL
	char	szConfirmUserName[CV_CRSCOMMNAME_MAXLEN];	//确认的用户名称
	tagSCHEME(){memset(this, 0x00, sizeof(struct tagSCHEME));}
}CRS_SCHEME;
/*
typedef struct tagSCHEME_ITEM_RELATION
{
	//CREATE TABLE t_crs_schemeanditemrelations(
	int 	nSchemeID;				  //smallint 	NOT NULL, 
	int		nItemIndex;				  //fd_item_index 		smallint 	NOT NULL, 
	int		nItemID;				  //fd_item_id 		smallint 	NOT NULL,
	unsigned short  bSkipped;		  // fd_skipped 		smallint 	NOT NULL,
	unsigned short  bManuConfired;    //fd_manuconfirmed 	smallint 	NOT NULL,
	int		nWaitConfirmTime;		  //fd_waitconfirm_time 	smallint,
	unsigned short  bTimeOutExecuted; //fd_timeout_execed 	smallint 	NOT NULL,
	tagSCHEME_ITEM_RELATION()
	{memset(this, 0x00, sizeof(struct tagSCHEME_ITEM_RELATION));}  //primary key(fd_scheme_id, fd_item_id, fd_item_index)
}CRS_SCHEME_ITEM_RELATION;
*/
typedef struct tagTRIGGER_ACTION_RELATION
{
	int nTriggerID;
	vector<string> vecActions;
	tagTRIGGER_ACTION_RELATION()
	{
		memset(this, 0x00, sizeof(struct tagTRIGGER_ACTION_RELATION));
	}
}CRS_TRIGGER_ACTION_RELATION;

typedef struct tagTRIGGERTYPE
{
	int nID; //Type ID
	char szTriggerTypeName[CV_CRSCOMMNAME_MAXLEN]; //Name
	char szTriggerDll[ICV_LONGFILENAME_MAXLEN]; //work dll
	char szTriggerTypeDesc[PK_DESC_MAXLEN]; //description
	tagTRIGGERTYPE()
	{
		memset(this, 0x00, sizeof(struct tagTRIGGERTYPE));
	}
}LINK_TRIGGERTYPE;

typedef struct tagTRIGGER
{
	int				nID;		// 触发源ID
	unsigned short  nType;	    // 触发源类型，如报警触发源、事件触发源、等值触发源等
	int				nSrcID;     // 触发源ID
	char			szSrcState[CV_CRSCOMMNAME_MAXLEN];   // 触发条件，如产生报警、确认报警等
	char			szTriTypeName[CV_CRSCOMMNAME_MAXLEN];    // 触发源类型名
	char szTriggerName[CV_CRSCOMMNAME_MAXLEN];		// 名称
	char szTriggerDesc[CV_CRS_DESC_MAXLEN];			// 描述
	char szTriggerParam[CV_CRS_TRIGGERCFG_MAXLEN];	// 配置参数
	int nSetID;	// 所属集合
	unsigned long ulDuration;
	int nIsSingleuserConfrim;
	int nIsdisplayOnclient;
	int nIsvalid;
	int nPriority;
	vector<int> vecEvent;
	tagTRIGGER()
	{
		memset(this, 0x00, sizeof(struct tagTRIGGER));
	}
}CRS_TRIGGER;


//typedef std::list<CRS_PRIORITY>				   CRSPriorityList;
typedef std::list<CRS_CATEGORY>				   CRSCategoryList;
typedef std::list<LINK_ACTIONTYPE>				CRSActionTypeList;
typedef std::list<LINK_ACTION>					CRSActionList;
typedef std::list<CRS_SCHEME>				   CRSSchemeList;
typedef std::list<TAGINFO>				       CRSTagInfoList;
typedef std::list<USABLETAG>					CRSUsableTagsList;
//typedef std::list<CRS_SCHEME_ITEM_RELATION>    CRSSIRelationList;
typedef std::list<CRS_TRIGGER>				   CRSTriggerList;
typedef std::list<LINK_TRIGGERTYPE>			   CRSTriggerTypeList;
typedef std::list<CRS_TRIGGER_ACTION_RELATION> CRSTARelationList;
typedef std::list<CRS_PARAMETER_VALUE>    CRSParameterList;

typedef std::map<int, int>					CRSIDMap;
typedef std::map<int, LINK_ACTIONTYPE>			CRSItemTypeMap;

/*SCHEME Already Defined in CRSAPI.h*/

/*SCHEMEIFO Already Defined in CRSAPI.h*/
// by ycd @ 11/18/2016
//typedef std::list<CRS_CATEGORY_INFO>    CRS_CATEGORY_INFOList;
//typedef std::list<CRS_TRIGGER_INFO>		CRSTriggerReturnInfoList;
//typedef std::list<CRS_SCHEME_INFO>      CRSSchemeReturnInfoList;
//typedef std::list<CRS_SCHEME_ITEM_INFO> CRSItemReturnInfoList;

/************************************************************************/
/*                           DOUBLEKEY Definition                       */
/************************************************************************/
#ifndef _CLASS_DOUBLEKEY_
#define _CLASS_DOUBLEKEY_

class DOUBLEKEY
{
public:
	DOUBLEKEY():_first(0),_second(0){}
	DOUBLEKEY(int x, int y):_first(x),_second(y){}
	DOUBLEKEY(const DOUBLEKEY& aKey):_first(aKey._first),_second(aKey._second){}
	~DOUBLEKEY(){}
private:
	int _first;
	int _second;
public:
	DOUBLEKEY& operator=(const DOUBLEKEY& aKey)
	{
		if (this == &aKey)
			return *this;

		_first = aKey._first;
		_second = aKey._second;
		return *this;
	}

//	friend bool operator>(const DOUBLEKEY& aKey, const DOUBLEKEY& anotherKey);
//	friend bool operator==(const DOUBLEKEY& aKey, const DOUBLEKEY& anotherKey);
//	friend bool operator<(const DOUBLEKEY& aKey, const DOUBLEKEY& anotherKey);
//	friend bool operator!=(const DOUBLEKEY& aKey, const DOUBLEKEY& anotherKey);

	friend	bool operator>(const DOUBLEKEY& aKey, const DOUBLEKEY& anotherKey)
	{
		return (aKey._first>anotherKey._first || (aKey._first==anotherKey._first&&aKey._second>anotherKey._second));
	}
	
	friend	bool operator==(const DOUBLEKEY& aKey, const DOUBLEKEY& anotherKey)
	{
		return (aKey._first == anotherKey._first && aKey._second==anotherKey._second);
	}
	
	friend	bool operator<(const DOUBLEKEY& aKey, const DOUBLEKEY& anotherKey)
	{
		return !(aKey > anotherKey || aKey == anotherKey);
	}
	
	friend	bool operator!=(const DOUBLEKEY& aKey, const DOUBLEKEY& anotherKey)
	{
		return !(aKey == anotherKey);
	}
};



#endif// _CLASS_DOUBLEKEY_
/************************************************************************/
/*                   End of DOUBLEKEY Definition                        */
/************************************************************************/


//typedef std::map<int, CRS_CATEGORY_INFO>		CRS_CATEGORY_INFOMap;
//typedef std::map<int, CRS_SCHEME_INFO>			SchemeReturnInfoMap;
//typedef std::map<DOUBLEKEY, CRS_SCHEME_ITEM_INFO>	CRS_SCHEME_ITEM_INFOMap;

/************************************************************************/
/*                     ClientManage Definition                        */
/************************************************************************/
typedef struct tagIPADRRESS
{
	char szIP[ICV_HOSTNAMESTRING_MAXLEN];
	int nClientCount;
	std::list<HQUEUE> hQueueList;

	tagIPADRRESS()
	{
		nClientCount = 0;
		memset(szIP, 0x00, sizeof(szIP));
	}
	~tagIPADRRESS()
	{
		hQueueList.clear();
	}
}CRS_IPADDRESS;

// 客户端信息
typedef struct tagCLIENT
{
	char szHMIIP[ICV_HOSTNAMESTRING_MAXLEN];
	HQUEUE hQueue;
	char szOperator[CV_CRSCOMMNAME_MAXLEN];
	ACE_Time_Value tvNewestConnTime;
	tagCLIENT(const tagCLIENT & x)
	{
		PKStringHelper::Safe_StrNCpy(szHMIIP, x.szHMIIP, sizeof(szHMIIP));
		memcpy(szOperator, x.szOperator, sizeof(szOperator));
		hQueue = x.hQueue;
		tvNewestConnTime = x.tvNewestConnTime;
	}
	tagCLIENT()
	{
		memset(szHMIIP, 0, sizeof(szHMIIP));
		memset(szOperator, 0, CV_CRSCOMMNAME_MAXLEN);
		hQueue = NULL;
		tvNewestConnTime = ACE_OS::gettimeofday();
	}

}CRS_CLIENT;

typedef std::list<CRS_IPADDRESS> CRS_IPLIST;
typedef std::list<CRS_CLIENT>    CRS_CLIENTLIST;

typedef struct tagHMIQUEUE
{
	HQUEUE hClientQueue;
	//int  nStatus;
	
	tagHMIQUEUE()
	{
		hClientQueue = NULL;
	//	nStatus = SchemeItemResult_Success;
	}
	
	tagHMIQUEUE(const tagHMIQUEUE& src)
	{
		hClientQueue = src.hClientQueue;
		//nStatus = src.nStatus;
	}
}CRS_HMIQUEUE;

typedef struct tagHMICLIENT
{
	char szHMINAME[CV_CRSCOMMNAME_MAXLEN];
	char szOperator[CV_CRSCOMMNAME_MAXLEN];
	ACE_Time_Value tvNewestConnTime;
	//std::list<CRS_HMIQUEUE> listHmiQueue;
	HQUEUE		hQueue;

	tagHMICLIENT()
	{
		memset(szHMINAME, 0x00, sizeof(szHMINAME));
		memset(szOperator, 0x00, sizeof(szOperator));
		hQueue = NULL;
	}
	~tagHMICLIENT()
	{
		//listHmiQueue.clear();
	}
}CRS_HMICLIENT;

typedef struct tagHMISTATUS
{
	std::string m_strHmiName;
	int nStatus;
}CRS_HMISTATUS;

typedef std::map<std::string, CRS_HMICLIENT*> CRS_HMIMAP;
typedef std::list<CRS_HMISTATUS>	CRS_HMISTATUSLIST;

//Action(activated SchemeItem)
typedef struct tagActivatedACTION
{
	//char szSchemeItem[CV_CRSCOMMNAME_MAXLEN];  //  +CRS_VALUE_DELIMITER
	int		nTriggerID;
	long	lTriggerTime;
	int		nSchemeID;          //三项可以唯一确定当前激活的动作项
	int		nSchemeItemID;
	int		nSchemeItemIndex;
	
	int		nSchemeItemTypeID;
	char	szResponseValue[ICV_RESPONSEVALUE_MAXLEN];
	char	szWorkDllName[ICV_LONGFILENAME_MAXLEN];

	long	lExecTimeOutMS;		//timeout
	ACE_Time_Value tvActivatedTime;	

	int		nActionResult;		//action result
	//目标ip列表，以；分隔，如果为"ALL"（不区分大小写），则目标ip为所有已经注册客户端
	//char	szDestIpList[CV_DESTHMIIPS_MAXLEN/ICV_HOSTNAMESTRING_MAXLEN][ICV_HOSTNAMESTRING_MAXLEN];
	CRS_HMISTATUSLIST  _destHmis;

	tagActivatedACTION()
	{
		nTriggerID = 0;
		lTriggerTime = 0;
		nSchemeID = 0;          //三项可以唯一确定当前激活的动作项
		nSchemeItemID = 0;
		nSchemeItemIndex = 0;
		nSchemeItemTypeID = 0;
		lExecTimeOutMS = 0;
		nActionResult = 0;		//action result

		memset(szResponseValue, 0x00, sizeof(szResponseValue));
		memset(szWorkDllName, 0x00, sizeof(szWorkDllName));
	}

	~tagActivatedACTION()
	{
		_destHmis.clear();
	}
}ACTIVATED_ACTION;
typedef std::list<ACTIVATED_ACTION>    ACTIVATED_ACTION_List;  //SchemeItemCmdInfo *

/************************************************************************/
/*                     Execution Type Definition                        */
/************************************************************************/

typedef struct tagCRS_InternalMsg
{
	long	lMsgType;	//消息类型
	long	lParam;
	void*	pParam;
	int		nTriggerID;    // 报警点ID（或者以后会增加的事件ID）
	char	szActType[CV_CRSCOMMNAME_MAXLEN];     // 产生条件
	tagCRS_InternalMsg()
	{
		memset(this, 0x00, sizeof(tagCRS_InternalMsg));
	}
}CRS_InternalMsg;

/************************************************************************/
/*                End of Execution Type Definition                      */
/************************************************************************/

#endif //_CRS_SERVICE_HXX_DEF_
