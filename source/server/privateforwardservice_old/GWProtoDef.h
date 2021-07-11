/**************************************************************
 *  Filename:    GWProtoDef.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �������غͲɼ�����֮���ͨѶЭ�顣����MQTTЭ���װ
 *
 *  @author:     shijunpu
 *  @version     07/10/2014  shijunpu  Initial Version
**************************************************************/

#ifndef _GATEWAY_COLLECTSERVICE_PROTOCOL_DEF_H_
#define _GATEWAY_COLLECTSERVICE_PROTOCOL_DEF_H_

/************************************************************************/
/* 
�������غͲɼ�����֮���ͨѶЭ�顣����MQTTЭ���װ��ÿ���������Ӧ��Ӧ�𷵻���0��ʾ�ɹ���������ʾ�����쳣
Э�����ݣ�
����-->������
RegisterGW��		��Ӧ��RegisterGW_Response		����ֵ��0, AlreadyRegistered,NotExist
RegisterGroup��		��Ӧ��RegisterGroup_Response	����ֵ��0��AlreadyRegistered
WriteGroup			��Ӧ��WriteGroup_Response		����ֵ��0��NotRegistered, NotMatched�����ж��󳤶Ⱥʹ���Ĳ�ƥ�䣩,Error
UnRegisterGroup��	��Ӧ��UnRegisterGroup_Response	����ֵ��0��NotRegistered
UnRegisterGW��		��Ӧ��UnRegisterGW_Response		����ֵ��0��AlreadyRegistered,NotExist

������-->���أ�
WriteTags			��Ӧ��WriteTags_Response		����ֵ��0��GroupNoExist,TagNotExist,TagCannotWrite��Group,Tag,Value��
*/
/************************************************************************/
// �������Ҫ�����������ء�web�������������ע������
// ע�����ص�Э�鲻��Ҫ���ط��������룬��Ϊ����˿����Ӻ������ˣ��Ժ������ͨ��������Ӿ͹�ȥ�ˣ�����Ҫͨ�����غ�����
#define		PROTOCOL_GW_TO_SERVER_REGISTERGW					0x101
#define		PROTOCOL_SERVER_TO_GW_REGISTERGW_RESPONSE			(PROTOCOL_GW_TO_SERVER_REGISTERGW + 0x80)
#define		PROTOCOL_GW_TO_SERVER_REGISTERGROUP					0x102
#define		PROTOCOL_SERVER_TO_GW_REGISTERGROUP_RESPONSE		(PROTOCOL_GW_TO_SERVER_REGISTERGROUP + 0x80)
#define		PROTOCOL_GW_TO_SERVER_WRITEGROUP					0x103
#define		PROTOCOL_SERVER_TO_GW_WRITEGROUP_RESPONSE			(PROTOCOL_GW_TO_SERVER_WRITEGROUP + 0x80)
#define		PROTOCOL_GW_TO_SERVER_UNREGISTERGROUP				0x201
#define		PROTOCOL_SERVER_TO_GW_UNREGISTERGROUP_RESPONSE		(PROTOCOL_GW_TO_SERVER_UNREGISTERGROUP + 0x80)
#define		PROTOCOL_GW_TO_SERVER_UNREGISTERSERVER				0x202
#define		PROTOCOL_SERVER_TO_GW_UNREGISTERSERVER_RESPONSE		(PROTOCOL_GW_TO_SERVER_UNREGISTERSERVER + 0x80)

#define		PROTOCOL_SERVER_TO_GW_WRITETAGS						0x21
#define		PROTOCOL_GW_TO_SERVER_WRITETAGS_RESPONSE			(PROTOCOL_SERVER_TO_GW_WRITETAGS + 0x80)

#define		PROTOCOL_RETCODE_SUCCESS							0
// �����Ƿ���˷��ظ����صĴ����� Server_TO_GW
#define		PROTOCOL_RETCODE_GW_ALREADY_REGISTERED				101	// AlreadyRegistered, Server or Group
#define		PROTOCOL_RETCODE_GW_NOT_EXIST						102	// AlreadyRegistered, Server or Group

#define		PROTOCOL_RETCODE_GROUP_ALREADY_REGISTERED			111	// AlreadyRegistered, Server or Group
#define		PROTOCOL_RETCODE_GROUP_NOT_EXIST					112	// AlreadyRegistered, Server or Group
#define		PROTOCOL_RETCODE_GROUP_NOT_MATCHED					113	// AlreadyRegistered,

// ���������ط��ظ�����˵Ĵ�����
#define		PROTOCOL_RETCODE_TAG_NOT_EXIST						201	// AlreadyRegistered, Server or Group
#define		PROTOCOL_RETCODE_TAG_CANNOT_WRITE					202	// AlreadyRegistered

#endif  // _GATEWAY_COLLECTSERVICE_PROTOCOL_DEF_H_
