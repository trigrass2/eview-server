/**************************************************************
 *  Filename:    GWProtoDef.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 描述网关和采集服务之间的通讯协议。采用MQTT协议封装
 *
 *  @author:     shijunpu
 *  @version     07/10/2014  shijunpu  Initial Version
**************************************************************/

#ifndef _GATEWAY_COLLECTSERVICE_PROTOCOL_DEF_H_
#define _GATEWAY_COLLECTSERVICE_PROTOCOL_DEF_H_

/************************************************************************/
/* 
描述网关和采集服务之间的通讯协议。采用MQTT协议封装，每个请求均有应答，应答返回码0表示成功，其他表示各类异常
协议内容：
网关-->服务器
RegisterGW，		响应：RegisterGW_Response		返回值：0, AlreadyRegistered,NotExist
RegisterGroup，		响应：RegisterGroup_Response	返回值：0，AlreadyRegistered
WriteGroup			响应：WriteGroup_Response		返回值：0，NotRegistered, NotMatched（组中对象长度和传输的不匹配）,Error
UnRegisterGroup，	响应：UnRegisterGroup_Response	返回值：0，NotRegistered
UnRegisterGW，		响应：UnRegisterGW_Response		返回值：0，AlreadyRegistered,NotExist

服务器-->网关：
WriteTags			响应：WriteTags_Response		返回值：0，GroupNoExist,TagNotExist,TagCannotWrite（Group,Tag,Value）
*/
/************************************************************************/
// 服务端需要区分来自网关、web服务，因此这里需注意区分
// 注册网关的协议不需要返回服务器代码，因为服务端靠连接号区分了，以后的数据通过这个连接就过去了，不需要通过网关号区分
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
// 下面是服务端返回给网关的错误码 Server_TO_GW
#define		PROTOCOL_RETCODE_GW_ALREADY_REGISTERED				101	// AlreadyRegistered, Server or Group
#define		PROTOCOL_RETCODE_GW_NOT_EXIST						102	// AlreadyRegistered, Server or Group

#define		PROTOCOL_RETCODE_GROUP_ALREADY_REGISTERED			111	// AlreadyRegistered, Server or Group
#define		PROTOCOL_RETCODE_GROUP_NOT_EXIST					112	// AlreadyRegistered, Server or Group
#define		PROTOCOL_RETCODE_GROUP_NOT_MATCHED					113	// AlreadyRegistered,

// 下面是网关返回给服务端的错误码
#define		PROTOCOL_RETCODE_TAG_NOT_EXIST						201	// AlreadyRegistered, Server or Group
#define		PROTOCOL_RETCODE_TAG_CANNOT_WRITE					202	// AlreadyRegistered

#endif  // _GATEWAY_COLLECTSERVICE_PROTOCOL_DEF_H_
