/**************************************************************
 *  Filename:    cvdefine.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: iCV公用的宏定义.超过两个及以上子系统的宏定义会被放入这里
 * !!!注意：如果需要声明下面这些宏定义的最大长度的数组，如： char szIP[ICV_IPSTRING_MAXLEN];
 *
 *  @version     07/16/2008  shijunpu	  将各个模块的共用部分宏定义提取出来,放入该部分.
**************************************************************/

#ifndef _PK_DEFINE_H_
#define _PK_DEFINE_H_

#include "pkcomm/pkcomm.h"
#include "eviewcomm/eviewcomm.h"

#define PK_LOCALHOST_IP								"127.0.0.1"			// 默认本机IP
#define PKMEMDB_LISTEN_PORT							30001				// 默认的pdb库端口
#define PKHISDB_LISTEN_PORT							30002				// 默认的pdb库端口
#define PKMEMDB_ACCESS_PASSWORD						"1qazXSW@3edc"		// 默认的pdb库访问密码	

#define _(STRING) STRING

// 驱动和PBServer之间发送消息和数据的类型
#define ACTIONTYPE_DRV2SERVER_CLEAR_DRIVER			1		// 自定义redis一系列操作，清除驱动所有的数据
#define ACTIONTYPE_DRV2SERVER_SET_TAGS_VTQ			2		// 对应redis的set，发送tag点数据, 格式：tagname|tagdatalen|tagdata 对应set,带有VTQ的tag点
#define ACTIONTYPE_DRV2SERVER_LIST_RPUSH_KV			3		// 对应redis的rpush（列表），rpush tag点数据
#define ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE			4		// 直接tag点，，如连接时不带VTQ间，直接将字符串写入即可

#define MODULENAME_EVENTSERVER						"pkeventserver"			// 事件服务
#define MODULENAME_LOCALSERVER						"pknodeserver"			// 节点处理服务
#define MODULENAME_MEMDB							"pkmemdb"				// 实时数据库服务
#define MODULENAME_HISDB							"pkhisdb"				// 历史数据库服务
#define MODULENAME_HISDATASERVER					"pkhisdataserver"		// 历史数据服务
#define MODULENAME_MODBUSSERVER						"pkmodserver"			// modbus转发服务

#define CHANNELNAME_CONTROL							"control"				// java web服务发给pkdataserver
#define CHANNELNAME_ALARMING						"alarming"				// 报警推送通道,报警状态改变时推送到java
#define CHANNELNAME_EVENT							"event"					// 操作日志推送到redis的通道
#define CHANNELNAME_PM_CONTROL						"pm-control"				// 控制进程管理器

#define EVENT_TYPE_MSGID							"evttype"				// 事件类型：控制,联动,报警
#define EVENT_TYPE_CONTROL							"control"				// 事件类型：控制
#define EVENT_TYPE_LINK								"link"					// 事件类型：联动
#define EVENT_TYPE_ALARM							"alarm"					// 事件类型：报警

// 内存变量还是二次变量
#define VARIABLE_TYPE_DEVICE						0						// 普通变量。deviceid >=0就是普通点
#define VARIABLE_TYPE_MEMVAR						-1						// 内存变量
#define VARIABLE_TYPE_CALC							-2						// 计算点变量
#define VARIABLE_TYPE_ACCUMULATE_TIME				-3						// 累计点变量，累计时间长度
#define VARIABLE_TYPE_ACCUMULATE_COUNT				-4						// 累计点变量，累计次数
#define VARIABLE_TYPE_SIMULATE						-5						// 模拟变量。deviceid >=0就是普通点
#define VARIABLE_TYPE_PREDEFINE						-6						// 预定义变量, 如cpu、内存等

//操作日志json字段值
#define TAG_FIELD_VALUE								"v"						// tag点的域值，值	 value
#define TAG_FIELD_TIME								"t"						// tag点的域值，时间 time
#define TAG_FIELD_QUALITY							"q"						// tag点的域值，质量 quality

//初始化字段值
#define INITJSONFIELD_MSGTYPE						"msgtype"
#define INITJSONFIELD_MSGTYPE_INITTAG				"inittag"
#define INITJSONFIELD_MSGTYPE_INITTAGNUM			"inittagnum"
#define INITJSONFIELD_MSGTYPE_INITOBJECT			"initobject"

//link
#define CHANNELNAME_LINK_BROADCAST					"broadcast"			// 将当前触发源信息推送到内存数据库
#define CHANNELNAME_LINK_ACTION2CLIENT				"action-client"		// 将要执行的动作信息推送到内存数据库
#define CHANNELNAME_LINK_TRIGGERTRIGGERED			"triggertriggered"  // 产生一个报警的时候将信息推送过来

// pbserver推送到pkdataserver的报警类型
#define ALARM_ACTION_UPDATE_OBJECTALARM				"alarm-updateobject"
#define ALARM_ACTION_UPDATE_TAGALARM				"alarm-updatetag"

#define ACTIONNAME_CLI2SERVER_ALARM_DELETE			"deletealarm"
#define ACTIONNAME_CLI2SERVER_ALARM_CONFIRM			"confirmalarm"
#define ACTIONNAME_CLI2SERVER_ALARM_SET_PARAM		"setalarmparam"	// 修改报警参数、阈值
#define ACTIONNAME_CLI2SERVER_ALARM_SUPPRESS		"suppressalarm"	// 抑制报警信息一段时间
#define ACTIONNAME_CLI2SERVER_CONTROL				"control"
#define ACTIONNAME_ACTION_EXECUTED					"action-exectued"   //报警动作已执行

// 报警状态定义
#define ALARM_PRODUCE								"produce"
#define ALARM_RECOVER								"recover"
#define ALARM_CONFIRM								"confirm"
#define ALARM_SUPPRESS								"suppress"
#define OBJECT_NEWALARM								"new"
#define OBJECT_OLDALARM								"old"

// 各个类型报警字段定义
#define ALARM_ACTION_TYPE							"actype"
#define ALARM_DEVICE_NAME							"dev"
#define ALARM_TAG_NAME								"tagn"
#define ALARM_ENABLE								"e"
#define ALARM_THRESH								"t"
#define ALARM_LEVEL									"l"
#define ALARM_TYPE									"type"
#define ALARM_JUDGEMUTHOD							"jm"
#define ALARM_SYSTEMNAME							"sys"
#define ALARM_ALARMING								"isa"
#define ALARM_PRODUCETIME							"pt"
#define ALARM_CONFIRMED								"isc"
#define ALARM_CONFIRMER								"cer"
#define ALARM_CONFIRMTIME							"ct"
#define ALARM_RECOVERTIME							"rt"
#define ALARM_REPETITIONS							"re"
#define ALARM_VALONPRODUCE							"vp"		// 报警产生时的值
#define ALARM_VALONCONFIRM							"vc"		// 报警确认时的值
#define ALARM_VALONRECOVER							"vr"		// 报警恢复时的值
#define ALARM_DESC									"desc"
#define ALARM_OBJECT_NAME							"obj"
#define ALARM_OBJECT_DESCRIPTION					"objdesc"
#define ALARM_PROP_NAME								"prop"
#define ALARM_PARAM1								"p1"		// 参数1
#define ALARM_PARAM2								"p2"		// 参数2
#define ALARM_PARAM3								"p3"		// 参数3
#define ALARM_PARAM4								"p4"		// 参数4

typedef struct _HighResTime
{
	unsigned int nSeconds;         /* seconds */
	unsigned int nMilSeconds;        /* and microseconds */
	_HighResTime(){nSeconds = nMilSeconds = 0;}
}HighResTime;

#endif //_PK_DEFINE_H_
