/**************************************************************
 *  Filename:    cvdefine.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: iCV���õĺ궨��.����������������ϵͳ�ĺ궨��ᱻ��������
 * !!!ע�⣺�����Ҫ����������Щ�궨�����󳤶ȵ����飬�磺 char szIP[ICV_IPSTRING_MAXLEN];
 *
 *  @version     07/16/2008  shijunpu	  ������ģ��Ĺ��ò��ֺ궨����ȡ����,����ò���.
**************************************************************/

#ifndef _PK_DEFINE_H_
#define _PK_DEFINE_H_

#include "pkcomm/pkcomm.h"
#include "eviewcomm/eviewcomm.h"

#define PK_LOCALHOST_IP								"127.0.0.1"			// Ĭ�ϱ���IP
#define PKMEMDB_LISTEN_PORT							30001				// Ĭ�ϵ�pdb��˿�
#define PKHISDB_LISTEN_PORT							30002				// Ĭ�ϵ�pdb��˿�
#define PKMEMDB_ACCESS_PASSWORD						"1qazXSW@3edc"		// Ĭ�ϵ�pdb���������	

#define _(STRING) STRING

// ������PBServer֮�䷢����Ϣ�����ݵ�����
#define ACTIONTYPE_DRV2SERVER_CLEAR_DRIVER			1		// �Զ���redisһϵ�в���������������е�����
#define ACTIONTYPE_DRV2SERVER_SET_TAGS_VTQ			2		// ��Ӧredis��set������tag������, ��ʽ��tagname|tagdatalen|tagdata ��Ӧset,����VTQ��tag��
#define ACTIONTYPE_DRV2SERVER_LIST_RPUSH_KV			3		// ��Ӧredis��rpush���б���rpush tag������
#define ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE			4		// ֱ��tag�㣬��������ʱ����VTQ�䣬ֱ�ӽ��ַ���д�뼴��

#define MODULENAME_EVENTSERVER						"pkeventserver"			// �¼�����
#define MODULENAME_LOCALSERVER						"pknodeserver"			// �ڵ㴦�����
#define MODULENAME_MEMDB							"pkmemdb"				// ʵʱ���ݿ����
#define MODULENAME_HISDB							"pkhisdb"				// ��ʷ���ݿ����
#define MODULENAME_HISDATASERVER					"pkhisdataserver"		// ��ʷ���ݷ���
#define MODULENAME_MODBUSSERVER						"pkmodserver"			// modbusת������

#define CHANNELNAME_CONTROL							"control"				// java web���񷢸�pkdataserver
#define CHANNELNAME_ALARMING						"alarming"				// ��������ͨ��,����״̬�ı�ʱ���͵�java
#define CHANNELNAME_EVENT							"event"					// ������־���͵�redis��ͨ��
#define CHANNELNAME_PM_CONTROL						"pm-control"				// ���ƽ��̹�����

#define EVENT_TYPE_MSGID							"evttype"				// �¼����ͣ�����,����,����
#define EVENT_TYPE_CONTROL							"control"				// �¼����ͣ�����
#define EVENT_TYPE_LINK								"link"					// �¼����ͣ�����
#define EVENT_TYPE_ALARM							"alarm"					// �¼����ͣ�����

// �ڴ�������Ƕ��α���
#define VARIABLE_TYPE_DEVICE						0						// ��ͨ������deviceid >=0������ͨ��
#define VARIABLE_TYPE_MEMVAR						-1						// �ڴ����
#define VARIABLE_TYPE_CALC							-2						// ��������
#define VARIABLE_TYPE_ACCUMULATE_TIME				-3						// �ۼƵ�������ۼ�ʱ�䳤��
#define VARIABLE_TYPE_ACCUMULATE_COUNT				-4						// �ۼƵ�������ۼƴ���
#define VARIABLE_TYPE_SIMULATE						-5						// ģ�������deviceid >=0������ͨ��
#define VARIABLE_TYPE_PREDEFINE						-6						// Ԥ�������, ��cpu���ڴ��

//������־json�ֶ�ֵ
#define TAG_FIELD_VALUE								"v"						// tag�����ֵ��ֵ	 value
#define TAG_FIELD_TIME								"t"						// tag�����ֵ��ʱ�� time
#define TAG_FIELD_QUALITY							"q"						// tag�����ֵ������ quality

//��ʼ���ֶ�ֵ
#define INITJSONFIELD_MSGTYPE						"msgtype"
#define INITJSONFIELD_MSGTYPE_INITTAG				"inittag"
#define INITJSONFIELD_MSGTYPE_INITTAGNUM			"inittagnum"
#define INITJSONFIELD_MSGTYPE_INITOBJECT			"initobject"

//link
#define CHANNELNAME_LINK_BROADCAST					"broadcast"			// ����ǰ����Դ��Ϣ���͵��ڴ����ݿ�
#define CHANNELNAME_LINK_ACTION2CLIENT				"action-client"		// ��Ҫִ�еĶ�����Ϣ���͵��ڴ����ݿ�
#define CHANNELNAME_LINK_TRIGGERTRIGGERED			"triggertriggered"  // ����һ��������ʱ����Ϣ���͹���

// pbserver���͵�pkdataserver�ı�������
#define ALARM_ACTION_UPDATE_OBJECTALARM				"alarm-updateobject"
#define ALARM_ACTION_UPDATE_TAGALARM				"alarm-updatetag"

#define ACTIONNAME_CLI2SERVER_ALARM_DELETE			"deletealarm"
#define ACTIONNAME_CLI2SERVER_ALARM_CONFIRM			"confirmalarm"
#define ACTIONNAME_CLI2SERVER_ALARM_SET_PARAM		"setalarmparam"	// �޸ı�����������ֵ
#define ACTIONNAME_CLI2SERVER_ALARM_SUPPRESS		"suppressalarm"	// ���Ʊ�����Ϣһ��ʱ��
#define ACTIONNAME_CLI2SERVER_CONTROL				"control"
#define ACTIONNAME_ACTION_EXECUTED					"action-exectued"   //����������ִ��

// ����״̬����
#define ALARM_PRODUCE								"produce"
#define ALARM_RECOVER								"recover"
#define ALARM_CONFIRM								"confirm"
#define ALARM_SUPPRESS								"suppress"
#define OBJECT_NEWALARM								"new"
#define OBJECT_OLDALARM								"old"

// �������ͱ����ֶζ���
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
#define ALARM_VALONPRODUCE							"vp"		// ��������ʱ��ֵ
#define ALARM_VALONCONFIRM							"vc"		// ����ȷ��ʱ��ֵ
#define ALARM_VALONRECOVER							"vr"		// �����ָ�ʱ��ֵ
#define ALARM_DESC									"desc"
#define ALARM_OBJECT_NAME							"obj"
#define ALARM_OBJECT_DESCRIPTION					"objdesc"
#define ALARM_PROP_NAME								"prop"
#define ALARM_PARAM1								"p1"		// ����1
#define ALARM_PARAM2								"p2"		// ����2
#define ALARM_PARAM3								"p3"		// ����3
#define ALARM_PARAM4								"p4"		// ����4

typedef struct _HighResTime
{
	unsigned int nSeconds;         /* seconds */
	unsigned int nMilSeconds;        /* and microseconds */
	_HighResTime(){nSeconds = nMilSeconds = 0;}
}HighResTime;

#endif //_PK_DEFINE_H_
