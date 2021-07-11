/*!***********************************************************
 *  @file        AlmDef.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  @brief       (事件系统相关定义).
 *
 *  @author:     shijunpu
 *  @version     08/18/2006  shijunpu  Initial Version
**************************************************************/
#ifndef _ALM_DEF_HEADER_
#define _ALM_DEF_HEADER_

#include <string.h>
#include "AlmMiniDef.h"
using namespace std;

#ifdef _WIN32
#define EVENT_API extern "C" __declspec(dllexport)
#else
#define EVENT_API extern "C"
#endif

// 报警描述长度
#define PK_ALMDESC_MAXLEN				32  // 报警描述长度限制
#define PK_EVTTYPE_MAXLEN				32	// 事件类型长度限制
#define PK_EVTSUBTYPE_MAXLEN			64	// 事件子类型长度限制
#define PK_EVTVALUE_MAXLEN				512 // 事件保存数据的长度限制
// 扩展参数定义
#define	ICV_EXTEND_PARAM_LEN			64 //扩展参数的最大长度
#define PK_ALARM_CATEGORY_MAXLEN		32	//报警分类的最大长度

/************************************************************************/
/*                       事件服务模块公共定义部分                       */
/************************************************************************/
//事件类型定义
#define EA_EVTTYPE_OPERATE		"操作事件"
#define EA_EVTTYPE_SYSTEM		"系统事件"
#define EA_EVTTYPE_CONFIG		"配置事件"
#define EA_EVTTYPE_RESPONSE		"联动事件"

//事件子类型
#define EA_SUBTYPE_CTRL			"控制"
#define EA_SUBTYPE_SWITCH		"开关"
#define EA_SUBTYPE_START		"启动"
#define EA_SUBTYPE_STOP			"停止"
#define EA_SUBTYPE_LICENSE		"许可证"
#define EA_SUBTYPE_TRIGGER		"触发"

#define EA_SUBTYPE_LOGINOUT		"登录登出" //权限服务

#define EA_SUBTYPE_CRS_TRIGGER  EA_SUBTYPE_TRIGGER  //联动:触发事件
#define EA_SUBTYPE_CRS_EXECUTE  "执行"				//联动:执行事件
#define EA_SUBTYPE_CRS_START    EA_SUBTYPE_START	//联动:启动预案
#define EA_SUBTYPE_CRS_MODIFY   "修改"				//联动:修改预案




/**
 *  @brief    (事件分类常量定义). 
 */
enum EVENT_CLASS
{
	AECONST_EVENT_CLASS_ALARM,		// 报警：过程变量
	AECONST_EVENT_CLASS_EVENT,		// 事件：复杂逻辑事件和定时事件
	AECONST_EVENT_CLASS_CLEAR,      // 清除所有客户端报警
	AECONST_EVENT_CLASS_MESSAGE,	// 消息：系统消息，应用消息，操作员消息
	AECONST_EVENT_CLASS_ALARMLIST,	// 报警列表：批量处理报警
	AECONST_EVENT_CLASS_ALARMLIST_WITH_SECAREA,	// 带安全区的报警列表
	AECONST_EVENT_CLASS_COUNT,
};

/**
 *  @brief    (事件类型常量定义). 
 */
enum EVENT_TYPE
{
	AECONST_ET_OK,				// no alarm
	AECONST_ET_LOLO = ALM_TYPE_LOLO,			// low low
	AECONST_ET_LO = ALM_TYPE_LO,				// low
	AECONST_ET_HI = ALM_TYPE_HI,				// high
	AECONST_ET_HIHI = ALM_TYPE_HIHI,			// high high
	AECONST_ET_ROC = ALM_TYPE_ROC,				// rate of change
	AECONST_ET_COS = ALM_TYPE_COS,				// change of state 0 ->1 1->0
	AECONST_ET_DEV,				// deviation
	AECONST_ET_FLOAT,			// Floating point error
	AECONST_ET_IO,				// general I/O failure
	AECONST_ET_COMM,			// Comm link failure
	AECONST_ET_URNG,			// under range (clamped at 0)
	AECONST_ET_ORNG,			// over range (clamped at MAX)
	AECONST_ET_RANG,			// out of range (value unknown)
	AECONST_ET_DEVICE,			// Device failure.
	AECONST_ET_STATION,			// Station Failure
	AECONST_ET_ACCESS,			// Access denied (privledge).
	AECONST_ET_NODATA,			// On poll but no data yet
	AECONST_ET_NOXDATA,			// Exception item but no data yet
	// 
	AECONST_ET_LOGIC,			// logic expression hit
	AECONST_ET_TIMER,			// time hit
	AECONST_ET_LOGIC_TIMER,		// logic and timer event
	//
	AECONST_ET_SYSMSG,			// system alert notification
	AECONST_ET_NETMSG,			// netowrk notification
	AECONST_ET_OPRMSG,			// operation action
	AECONST_ET_LINK_MSG,		// response notification

	AECONST_ET_OPEN = ALM_TYPE_OPEN,			// 0->1
	AECONST_ET_CLOSE = ALM_TYPE_CLOSE,			// 1->0
	AECONST_ET_USER_DEF0 = ALM_TYPE_USER_DEF0,		// user defined alarm type			
	AECONST_ET_USER_DEF1 = ALM_TYPE_USER_DEF1,		// user defined alarm type			
	AECONST_ET_USER_DEF2 = ALM_TYPE_USER_DEF2,		// user defined alarm type			
	AECONST_ET_USER_DEF3 = ALM_TYPE_USER_DEF3,		// user defined alarm type			
	AECONST_ET_USER_DEF4 = ALM_TYPE_USER_DEF4,		// user defined alarm type			
	AECONST_ET_USER_DEF5 = ALM_TYPE_USER_DEF5,		// user defined alarm type			
	AECONST_ET_USER_DEF6 = ALM_TYPE_USER_DEF6,		// user defined alarm type			
	AECONST_ET_USER_DEF7 = ALM_TYPE_USER_DEF7,		// user defined alarm type			
	AECONST_ET_USER_DEF8 = ALM_TYPE_USER_DEF8,		// user defined alarm type			
	AECONST_ET_USER_DEF9 = ALM_TYPE_USER_DEF9,		// user defined alarm type			
	AECONST_ET_USER_DEF10 = ALM_TYPE_USER_DEF10,		// user defined alarm type			
	AECONST_ET_USER_DEF11 = ALM_TYPE_USER_DEF11,		// user defined alarm type			
	AECONST_ET_USER_DEF12 = ALM_TYPE_USER_DEF12,		// user defined alarm type			
	AECONST_ET_USER_DEF13 = ALM_TYPE_USER_DEF13,		// user defined alarm type			
	AECONST_ET_USER_DEF14 = ALM_TYPE_USER_DEF14,		// user defined alarm type			
	AECONST_ET_USER_DEF15 = ALM_TYPE_USER_DEF15,		// user defined alarm type			
	AECONST_ET_KEEPON = ALM_TYPE_KEEPON,		// =1			
	AECONST_ET_KEEPOFF = ALM_TYPE_KEEPOFF,		// =0			

	AECONST_ET_COUNT,			// type count
	AECONST_ET_ALL = ALM_TYPE_WILDCARD,		// all alram type if exist
};

/**
 *  @brief    (报警状态常量定义). 
 */
enum ALARM_STATUS
{
	AECONST_ES_OK  = ALM_STATUS_OK,			// no alarm											0000 0000
	AECONST_ES_ALM = ALM_STATUS_ALM,			// in alarm（报警）									0000 0011
	AECONST_ES_RTN = ALM_STATUS_RTN,			// return to normal(恢复)							0000 0010
	AECONST_ES_ACK = ALM_STATUS_ACK,			// acked alarm(确认)								0000 0001
	AECONST_ES_ALM_MASK = ALM_STATUS_ALM_MASK,	// 报警未恢复的MASK									0000 0001
	AECONST_ES_UNACK_MASK = ALM_STATUS_UNACK_MASK,	// 报警未确认的MASK									0000 0010
	AECONST_ES_ACK_RTN = ALM_STATUS_OK,		// acked & recovered  								0000 0000
	AECONST_ES_DEL = 8,			// manual delete data(手动删除)						0000 1000
	AECONST_ES_COUNT,			// count of status
};

// Defines the Timer Type
//enum TIMER_TYPE
//{
//	AECONST_TIMER_ONEOFF	= 0,		// !!! defined in LinkDefines.hxx, should copied to here use version_macros
//	AECONST_TIMER_PERIODIC  = 1,
//	AECONST_TIMER_WEEK		= 2,
//	AECONST_TIMER_MONTH		= 3,
//	AECONST_TIMER_TYPE_COUNT,
//};

/**
 *  @brief    (priority definition for events/alarms). 
 *  @version  03/18/2008  chenshengyu  $(报警级别扩展到0~255，原有的宏定义仍然沿用，以保持兼容性).
 */
enum EVENT_PRIORITY
{
	AECONST_EP_CRITICAL,
	AECONST_EP_HIHI,
	AECONST_EP_HI,
	AECONST_EP_NORMAL,
	AECONST_EP_LOW,
	AECONST_EP_LOLO,
	AECONST_EP_MINOR,
	AECONST_EP_COUNT,
	AECONST_EP_DEFAULT = AECONST_EP_LOW,
};

#include "common/eviewdefine.h"

#define ICV_EA_TEXTVALUE_MAXLEN 8

// 报警/事件ID
// 所有属性均为内部使用，不可修改
// 用于定位一条报警/事件
struct TAlm_ID
{	
	int	ulAlmSeq;				// alarm sequence
};

// 当前报警值
struct TAlm_Value
{
	unsigned char ucValueType;
	unsigned char ucValueSize;
	char 		   szRestoreValue[ICV_EA_TEXTVALUE_MAXLEN];
	char 		   szValue[ICV_EA_TEXTVALUE_MAXLEN];
	TAlm_Value()
	{
		memset(szRestoreValue, 0x00,ICV_EA_TEXTVALUE_MAXLEN);
		memset(szValue, 0x00,ICV_EA_TEXTVALUE_MAXLEN);
	}
} ;


// 变量报警信息
struct TAlm_BlockAlm       // details of a block alarm
{
	TAlm_ID				almID;					// ID of alarm
	TAlm_Value			aCurrentValue;			// current alarm value
	char				szPreValue[ICV_EA_TEXTVALUE_MAXLEN]; //当前报警/确认之前的值 

	char				szNode[PK_NAME_MAXLEN];				// node name
	char				szTag[PK_NAME_MAXLEN];			// tag name
	char                szTagDescription[PK_DESC_MAXLEN];  //tag description
	char				szEgu[PK_ALMDESC_MAXLEN];	// unit name (EGU)
	char                szClassName[PK_NAME_MAXLEN];  // class name
	char                szSubSystemName[PK_NAME_MAXLEN];  //subsystem name
	char				szObjName[PK_NAME_MAXLEN];
	char				szObjDesc[PK_DESC_MAXLEN];
	char				szCategory[PK_ALARM_CATEGORY_MAXLEN];	// alarm category
	char				szExtParam1[ICV_EXTEND_PARAM_LEN];	//extend param
	char				szExtParam2[ICV_EXTEND_PARAM_LEN];	//extend param
	char				szExtParam3[ICV_EXTEND_PARAM_LEN];	//extend param
	char				szExtParam4[ICV_EXTEND_PARAM_LEN];	//extend param

	unsigned char		ucType;			// IA_HIHI, IA_LOLO, etc. 
	// Note: ulState may also contain process priority, see ALMVAL macro in dmacsdba.h.
	unsigned char		ucPriority;		// priority of alarm

	unsigned char		ucStatus;		// alm status

	HighResTime		tmFire;			// fire time

	TAlm_BlockAlm()
	{
		almID.ulAlmSeq = 0;
		memset(szPreValue, 0x00, sizeof(szPreValue));
		memset(szNode,0x00,PK_NAME_MAXLEN);
		memset(szTag, 0x00, PK_NAME_MAXLEN);
		memset(szEgu, 0x00, PK_ALMDESC_MAXLEN);
		memset(szClassName, 0x00, PK_NAME_MAXLEN);
		memset(szSubSystemName, 0x00, PK_NAME_MAXLEN);
		memset(szObjName, 0x00, PK_NAME_MAXLEN);
		memset(szObjDesc, 0x00, PK_DESC_MAXLEN);
		memset(szTagDescription,0x00,PK_DESC_MAXLEN);
		memset(szExtParam1, 0x00, ICV_EXTEND_PARAM_LEN);
		memset(szExtParam2, 0x00, ICV_EXTEND_PARAM_LEN);
		memset(szExtParam3, 0x00, ICV_EXTEND_PARAM_LEN);
		memset(szExtParam4, 0x00, ICV_EXTEND_PARAM_LEN);
	}
} ;

struct CV_ALARM
{
	TAlm_ID				almID;					// ID of alarm
	TAlm_Value			aCurrentValue;			// current alarm value
	char				szPreValue[ICV_EA_TEXTVALUE_MAXLEN]; //当前报警/确认之前的值 

	char				szNodeName[PK_NAME_MAXLEN];				// node name
	char				szTagName[PK_NAME_MAXLEN];			// tag name
	char                szTagDesc[PK_DESC_MAXLEN];  //tag description
	char				szConfirmPerson[ICV_OPERATORNAME_MAXLEN];		// field name
	char				szEgu[PK_ALMDESC_MAXLEN];	// unit name (EGU)
	char                szClassName[PK_NAME_MAXLEN];  // class name
	char                szSubSystemName[PK_NAME_MAXLEN];  //subsystem name
	char				szObjName[PK_NAME_MAXLEN];
	char				szObjDesc[PK_DESC_MAXLEN];
	char				szCategory[PK_ALARM_CATEGORY_MAXLEN];	// alarm category
	char				szExtParam1[ICV_EXTEND_PARAM_LEN];	//extend param
	char				szExtParam2[ICV_EXTEND_PARAM_LEN];	//extend param
	char				szExtParam3[ICV_EXTEND_PARAM_LEN];	//extend param
	char				szExtParam4[ICV_EXTEND_PARAM_LEN];	//extend param

	unsigned char		ucType;			// IA_HIHI, IA_LOLO, etc. 
	unsigned char		ucPriority;		// priority of alarm

	unsigned char		ucStatus;		// alm status

	HighResTime			tsOccur;			// fire time
	HighResTime			tsConfirm;			// confirm time
	HighResTime			tsRecover;			// recover time

	unsigned int		unRepetition;		// repetition time of alarm

	CV_ALARM()
	{
		almID.ulAlmSeq = 0;
		memset(szPreValue, 0x00, sizeof(szPreValue));
		memset(szNodeName,0x00,PK_NAME_MAXLEN);
		memset(szTagName, 0x00, PK_NAME_MAXLEN);
		memset(szEgu, 0x00, PK_ALMDESC_MAXLEN);
		memset(szClassName, 0x00, PK_NAME_MAXLEN);
		memset(szSubSystemName, 0x00, PK_NAME_MAXLEN);
		memset(szObjName, 0x00, PK_NAME_MAXLEN);
		memset(szObjDesc, 0x00, PK_DESC_MAXLEN);
		memset(szTagDesc,0x00,PK_DESC_MAXLEN);
		memset(szConfirmPerson, 0x00, ICV_OPERATORNAME_MAXLEN);
		memset(szCategory, 0x00, sizeof(szCategory));
		memset(szExtParam1, 0x00, ICV_EXTEND_PARAM_LEN);
		memset(szExtParam2, 0x00, ICV_EXTEND_PARAM_LEN);
		memset(szExtParam3, 0x00, ICV_EXTEND_PARAM_LEN);
		memset(szExtParam4, 0x00, ICV_EXTEND_PARAM_LEN);
		unRepetition = 0;
	}
};

// 验证变量报警消息
struct TAlm_VerifiedBlockAlm
{
	TAlm_BlockAlm	blk_alm;
	char			szVerifiedBy[ICV_OPERATORNAME_MAXLEN];	
	TAlm_VerifiedBlockAlm()
	{
		memset(szVerifiedBy, 0x00, ICV_OPERATORNAME_MAXLEN);
	}
};

// Event Message
struct TAlm_EventMsg
{
	TAlm_ID			almID;										 // 由事件报警服务生成的ID
	char			szEventNodeName[PK_NAME_MAXLEN];		 // name of node 
	char			szEventLoginName[PK_NAME_MAXLEN];		 // login name of the operator (if applic)
	char			szAppName[PK_SHORTFILENAME_MAXLEN];	     // name of application generating operator message
	char			szTag[PK_NAME_MAXLEN];				     // tag name related to operator message (only for operator)
	char			szTagDescription[PK_DESC_MAXLEN];		//tag description
	char			szMsg[PK_DESC_MAXLEN];			     // msg content
	char            szClassName[PK_NAME_MAXLEN];		     // class name
	char            szSubSystemName[PK_NAME_MAXLEN];     //subsystem name

	HighResTime	tmFire;								// fire time
	char szEventType[PK_EVTTYPE_MAXLEN];		// 事件类型 
	char szSubType[PK_EVTSUBTYPE_MAXLEN];		// 子类型
	char szCurrentValue[PK_EVTVALUE_MAXLEN];	// 控制前的当前值
	char szCtrlValue[PK_EVTVALUE_MAXLEN];		// 控制后的期望值

	TAlm_EventMsg()
	{
		almID.ulAlmSeq = 0;
		memset(szEventNodeName, 0x00, PK_NAME_MAXLEN);
		memset(szEventLoginName, 0x00, PK_NAME_MAXLEN);
		memset(szAppName, 0x00, PK_SHORTFILENAME_MAXLEN);
		memset(szTag, 0x00, PK_NAME_MAXLEN);
		memset(szTagDescription, 0x00, PK_DESC_MAXLEN);
		memset(szMsg, 0x00, PK_DESC_MAXLEN);
		memset(szClassName, 0x00, sizeof(szClassName));
		memset(szSubSystemName, 0x00, sizeof(szSubSystemName));
		memset(szEventType, 0x00, sizeof(szEventType));
		memset(szSubType, 0x00, sizeof(szSubType));
		memset(szCurrentValue, 0x00, sizeof(szCurrentValue));
		memset(szCtrlValue, 0x00, sizeof(szCtrlValue));
	}
};

typedef struct  
{
	unsigned short uType;
	void*  pParam;
	void* hSrcQueue;
}EA_Message;


#define APP_NAME_MAX_LENGTH 64
typedef struct tagAppNameInfo
{ 
	char szAppName[APP_NAME_MAX_LENGTH];
}AppNameInfo;


#define MAX_SIZE(x,y) (x>=y?x:y)
#define ICV_EA_FILTERCONTENT_MAXLEN 96
#define ICV_EA_ALLALMAREAS_MAXLEN 192 // the max alarm areas' length which an alarm belong to:(six alarm areas' length)
#define ICV_EA_ALLSECAREAS_MAXLEN 192  //the max alarm security's length which an alarm belong to:(six security alarms' length)
#define ICV_EA_ALMSTRUCT_MAXLEN sizeof(TAlm_BlockAlm)  //the length of the structure of an alarm
#define ICV_EA_EVENTSTRUCT_MAXLEN sizeof(TAlm_EventMsg)  //the length of the structure of an event
#define ICV_EA_VERIFIEDALM_MAXLEN sizeof(TAlm_VerifiedBlockAlm) // the length of the structure of an verified alarm
#define ICV_EA_CVALARM_MAXLEN  sizeof(CV_ALARM) 
#define ICV_EA_MSG_MAXLEN sizeof(long)+sizeof(long)\
	+MAX_SIZE(ICV_EA_ALMSTRUCT_MAXLEN,ICV_EA_EVENTSTRUCT_MAXLEN)\
	+ICV_EA_ALLALMAREAS_MAXLEN+ICV_EA_ALLSECAREAS_MAXLEN // max data length:
#define ICV_EA_MSG_TOTALLEN sizeof(long)+sizeof(long)+sizeof(long)+sizeof(CV_ALARM)+ICV_EA_ALLALMAREAS_MAXLEN+ICV_EA_ALLSECAREAS_MAXLEN

#define ICV_EA_FILTERBYPRI 0  // filter by priority
#define ICV_EA_FILTERBYAREAS 1 // filter by alarm areas
#define ICV_EA_STOPFILTER 2   // stop filtering
#define ICV_EA_ALARMTYPE 3  // indicate this msg is an alarm type
#define ICV_EA_EVENTTYPE 4 // indicate this msg is an event type
#define ICV_EA_VERIFIEDALARMTYPE 5 // indicate this msg is a verified alarm type
#define ICV_EA_FILTERTYPE 6  //indicate this msg is a filtering type
#define ICV_EA_BATCHSTART	7	// 开启报警产生\恢复\确认\删除的批量处理
#define ICV_EA_BATCHSTOP	8	// 停止报警产生\恢复\确认\删除的批量处理

#define INITIALVALUE 0 // initialize the value
#define ICV_EA_FILTERAREA_MAXNUM 3 
#define AM_ResourceAuth_LEN 36 //the length of the structure of AM_ResourceAuth 
#define DEFAULT_SERVICE_QUEUEID 1    // default service queueid
#define ICV_EA_TIMEOUT 2000        // time out
#define BDBQUEUE_MAX_RECORD_NUM 50000000  //BDB Queue Record Number
#define EA_TRANSACTION_RECORD_MAXNUM 200
#define REMOVERECORDNUM 200 //Remove Record number from Queue Head
#define EA_REGISTER_ALARM 1
#define EA_REGISTER_EVNET 2
#define EA_REGISTER_ALARMANDEVENT 3

#define EA_REGISTER_WITHUSERNAME 1
#define EA_REGISTER_WITHNODENAME 2
#define EA_UNREGISTER__MSGTYPE	3   // indicate this msg is for unregistering
#define EA_ACKALM_MSGTYPE		4   // indicate this msg is an acknowledge alarm's msg
#define EA_DELETEALM_MSGTYPE    5   // indicate this msg is an delete alarm's msg
#define EA_MSG_ID_HEARTBEAT		6   // indicate this msg is for heart beat
#define EA_MSG_ID_HEARTBEAT_EX		14   // indicate this msg is for heart beat and reregister

#define EA_MSG_QUERY_ALARM				7   // query alarms
#define EA_MSG_QUERY_EVENT				8   // query events
#define EA_MSG_INSERT_ALARM				9   // insert alarms
#define EA_MSG_INSERT_EVENT				10   // insert events
#define EA_MSG_LIST_APPNAME				11	 // list app names	
#define EA_MSG_QUERY_ALARM_COUNT		12   // query alarm count
#define EA_MSG_QUERY_EVENT_COUNT		13   // query event count

#define EA_MSG_ADD_EVENT			15       // add an event
#define EA_MSG_NEW_ALARM			20		// new alarm generated
#define EA_ACKALMLIST_MSGTYPE		31   // indicate this msg is an acknowledge alarm list's msg
#define EA_SYNCALMLIST_MSGTYPE		33   // indicate this msg is to sync alarm list from active scada
#define	 MSGTYPE_SHOULD_END			0xFFFF // EA 线程退出

#define AREA_FILTER_MAXLEN	(ICV_ALARMAREA_MAXNUM * ICV_ALARMAREA_MAXLEN  + 1)

#define  GETQUEUE_TIMEOUT					3  //query timeout

#define EA_REGISTER_ALARMAREAS_MAXNUM 16
#define EA_REGISTER_ALLAREAS EA_REGISTER_ALARMAREAS_MAXNUM+1

#define PDB_ALARM_SVR		"PDB_Alarm"
#define ALARMQUEUE  ACE_TEXT("ALARMQUEUE") 
#define EASERVICE_RUNTIME_DIR_NAME   ACE_TEXT("EASERVICE")
#define EASERVICE_RUNTIME_DIR_MUTEX   ACE_TEXT("EASERVICE_DIR_MUTEX")

// EAServiceCfg 相关定义
#define EAServiceCfgFile		"EAServiceCfg.xml"
#define EACNFG_ROOT_NAME		"EAService"
#define EACNFG_ARCHIVE_NAME		"Archive"
#define EACNFG_EXPIRE_NAME		"Expire"
#define EACNFG_DEST_NAME		"Destination"
#define EACNFG_LIMIT_NAME		"Limit"
#define EACNFG_CHECK_NAME		"Check"
#define EACNFG_EXPIRE_ATTR		"KeepDays"
#define EACNFG_DEST_ATTR		"Path"
#define EACNFG_FILECOUNT_ATTR	"MaxFileNumber"
#define EACNFG_EALIMIT_ATTR		"EALimit"
#define EACNFG_ARCHLIMIT_ATTR	"ArchLimit"
#define EACNFG_LMTOFFSET_ATTR	"EAOffset"
#define EACNFG_CHECKINTV_ATTR	"Interval"

#define EVENT_ARCH_PREFIX	"event_archive_"
#define ALARM_ARCH_PREFIX	"alarm_archive_"
#define EA_ARCH_POSTFIX		".db"

#define DEFAULT_DEST_PATH			"/rtd/eaarchieve"
#define DEFAULT_EXPIRE_SEC			(30*86400)
#define DEFAULT_EXPIRE_DAY			30
#define DEFAULT_EA_RECORD_LIMIT		100000
#define DEFAULT_EA_LIMIT_OFFSET		1000
#define DEFAULT_ARCH_RECORD_LIMIT	100000
#define ICV_EA_DAYTOSEC				(24*60*60)

#define SECOND_PER_DAY	86400

//////////////////////////////////////////////////////////////////////////
//EAQuerying Condition Names
#define EA_QUERY_CONTENT				"content"			// 内容
#define EA_QUERY_TIME_BEGIN				"time_begin"		// 起始时间
#define EA_QUERY_TIME_END				"time_end"			// 截止时间
#define EA_QUERY_ALARM_TYPE				"alarm_type"		// 报警类型
#define EA_QUERY_ALARM_AREA				"alarm_area"		// 报警区域
#define EA_QUERY_NODE_NAME				"node_name"			// 节点名称
#define EA_QUERY_TAG_NAME				"tag_name"			// 变量名称
#define EA_QUERY_PRIORITY				"priority"			// 优先级
#define EA_QUERY_SUBSYS_NAME			"subsys_name"		// 子系统名
#define EA_QUERY_CLASS_NAME				"class_name"		// 类名
#define EA_QUERY_LOGIN_NAME				"login_name"		// 用户
#define EA_QUERY_APP_NAME				"app_name"				// 程序模块名称
#define EA_QUERY_TAG_DESCRIPTION		"tag_description"		// 变量描述
#define EA_QUERY_CONFIRMTIME_BEGIN		"confirm_time_begin"	// 确认起始时间
#define EA_QUERY_CONFIRMTIME_END		"confirm_time_end"		// 截止时间
#define EA_QUERY_RECOVERTIME_BEGIN		"recover_time_begin"	// 恢复起始时间
#define EA_QUERY_RECOVERTIME_END		"recover_time_end"		// 截止时间
#define EA_QUERY_EVENT_TYPE				"event_type"			// 事件类型
#define EA_QUERY_EVENT_SUBTYPE			"event_subtype"			// 事件子类型

#define EA_QUERY_ORDERING_RULES			"ordering_rules"		// 排序规则
#define EA_QUERY_ORDER_ASC				"ASC"				    // 升序排列
#define EA_QUERY_ORDER_DESC				"DESC"					// 降序排列

//以下宏定义了数据库表t_cc_alarm及表t_cc_event中各字段
#define EA_QUERY_ORDER_EVENT_ID			"fd_event_id"				// 字段：事件ID
#define EA_QUERY_ORDER_EVENT_NODE_NAME	"fd_eventnodename"			// 字段：节点名
#define EA_QUERY_ORDER_EVENT_LOGIN_NAME	"fd_eventloginname"			// 字段：登录名
#define EA_QUERY_ORDER_APPNAME			"fd_appname"				// 字段：应用名
#define EA_QUERY_ORDER_TAG				"fd_tag"					// 字段：点名
#define EA_QUERY_ORDER_TAG_DESCRIPTION	"fd_tagdescription"			// 字段：点的描述
#define EA_QUERY_ORDER_EVENTMESSAGE		"fd_eventmessage"			// 字段：消息
#define EA_QUERY_ORDER_CLASS_NAME		"fd_classname"				// 字段：类名
#define EA_QUERY_ORDER_SUBSYSTEM_NAME	"fd_subsystemname"			// 字段：子系统名
#define EA_QUERY_ORDER_TIME				"fd_time"					// 字段：时间
#define EA_QUERY_ORDER_FIRSTALARMAREA	"fd_firstalarmarea"			// 字段：报警区域1
#define EA_QUERY_ORDER_SECONDALARMAREA	"fd_secondalarmarea"		// 字段：报警区域2
#define EA_QUERY_ORDER_THIRDALARMAREA	"fd_thirdalarmarea"			// 字段：报警区域3
#define EA_QUERY_ORDER_FOURTHALARMAREA	"fd_fourthalarmarea"		// 字段：报警区域4
#define EA_QUERY_ORDER_FIFTHALARMAREA	"fd_fifthalarmarea"			// 字段：报警区域5
#define EA_QUERY_ORDER_SIXTHALARMAREA	"fd_sixthalarmarea"			// 字段：报警区域6
#define EA_QUERY_ORDER_EVENT_TYPE		"fd_event_type"				// 字段：事件类型
#define EA_QUERY_ORDER_EVENT_SUBTYPE	"fd_event_subtype"			// 字段：事件子类型
#define EA_QUERY_ORDER_CURRENTVALUE		"fd_current_value"			// 字段：当前值
#define EA_QUERY_ORDER_CTRLVALUE		"fd_ctrl_value"				// 字段：期望值

#define EA_QUERY_ORDER_OBJ				"fd_obj"
#define EA_QUERY_ORDER_OBJ_DESCRIPTION	"fd_objdescription"
#define EA_QUERY_ORDER_ALARM_ID			"fd_alarm_id"				// 字段：报警ID		
#define EA_QUERY_ORDER_VALUE_TYPE		"fd_value_type"				// 字段：值类型
#define EA_QUERY_ORDER_VALUE_SIZE		"fd_value_size"				// 字段：值所占大小
#define EA_QUERY_ORDER_VALUE_CONTENT	"fd_value_content"			// 字段：值内容
#define EA_QUERY_ORDER_PREVALUE_CONTENT "fd_prevalue_content"		// 字段：报警之前的值内容
#define EA_QUERY_ORDER_NODE				"fd_node"					// 字段：节点名		
#define EA_QUERY_ORDER_CONFIRM_PERSON	"fd_confirmperson"			// 字段：确认者
#define EA_QUERY_ORDER_EGU				"fd_egu"					// 字段：工程单位
#define EA_QUERY_ORDER_CATEGORY			"fd_category"				// 字段：类目
#define EA_QUERY_ORDER_TYPE				"fd_type"					// 字段：类型
#define EA_QUERY_ORDER_PRIORITY			"fd_priority"				// 字段：优先级
#define EA_QUERY_ORDER_STATUS			"fd_status"					// 字段：状态
#define EA_QUERY_ORDER_OCCUR_TIME		"fd_ocurrtime"				// 字段：发生时间
#define EA_QUERY_ORDER_CONFIRM_TIME		"fd_confirmtime"			// 字段：确认时间
#define EA_QUERY_ORDER_RECOVER_TIME		"fd_recovertime"			// 字段：恢复时间
#define EA_QUERY_ORDER_FIRSTALARMSECAREA	"fd_firstalarmsecarea"	// 字段：报警安全区域1
#define EA_QUERY_ORDER_SECONDALARMSECAREA	"fd_secondalarmsecarea"	// 字段：报警安全区域2
#define EA_QUERY_ORDER_THIRDALARMSECAREA	"fd_thirdalarmsecarea"	// 字段：报警安全区域3
#define EA_QUERY_ORDER_FOURTHALARMSECAREA	"fd_fourthalarmsecarea"	// 字段：报警安全区域4
#define EA_QUERY_ORDER_FIFTHALARMSECAREA	"fd_fifthalarmsecarea"	// 字段：报警安全区域5
#define EA_QUERY_ORDER_SIXTHALARMSECAREA	"fd_sixthalarmsecareas"	// 字段：报警安全区域6
#define EA_QUERY_ORDER_EXTPARAM1			"fd_ext_param1"			// 字段：扩展参数1	
#define EA_QUERY_ORDER_EXTPARAM2			"fd_ext_param2"			// 字段：扩展参数2
#define EA_QUERY_ORDER_EXTPARAM3			"fd_ext_param3"			// 字段：扩展参数3	
#define EA_QUERY_ORDER_EXTPARAM4			"fd_ext_param4"			// 字段：扩展参数4	

#define EA_CONDITION_SEPERATOR		'|'
#define EA_CONDITION_EQUAL			'='
#define EA_STRING_DELIMITER			';'

static const char* g_szEACondition[] = {
	EA_QUERY_CONTENT,
	EA_QUERY_TIME_BEGIN,
	EA_QUERY_TIME_END,
	EA_QUERY_ALARM_TYPE,
	EA_QUERY_ALARM_AREA,
	EA_QUERY_TAG_NAME,
	EA_QUERY_PRIORITY,
	EA_QUERY_SUBSYS_NAME,
	EA_QUERY_CLASS_NAME
};

enum EA_Query_Index{
	Index_Content,
	Index_BeginTime,
	Index_EndTime,
	Index_AlarmType,
	Index_AlarmArea,
	Index_TagName,
	Index_Priority,
	Index_SubsysName,
	Index_ClassName,
	Total_Condition_Number
};

struct TAlm_WrappedAlarm
{
	CV_ALARM alarm;
	char alarmArea[ICV_EA_ALLALMAREAS_MAXLEN];
	long alarmAreaNum;

	TAlm_WrappedAlarm()
	{
		memset(alarmArea, 0x00, sizeof(alarmArea));
		alarmAreaNum = 0;
	}
};

struct TAlm_ControlMsg
{
	unsigned char ucCtrlCmd;
	unsigned long ulTimeoutInMS;
	TAlm_ControlMsg()
	{
		ucCtrlCmd = 0;
		ulTimeoutInMS = 0;
	}
};

EVENT_API int evtLogEvent(TAlm_EventMsg & event_msg);

#endif //_ALM_DEF_HEADER_
