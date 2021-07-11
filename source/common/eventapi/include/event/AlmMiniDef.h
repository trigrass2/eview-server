/*!***********************************************************
 *  @file        AlmMiniDef.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  @brief       (插件所需报警相关最小结构体定义).
 *
 *  @author:     yangbo
 *  @version     11/15/2012  yangbo  Initial Version
**************************************************************/
#ifndef _ALM_MINI_DEF_HEADER_
#define _ALM_MINI_DEF_HEADER_

//////////////////// 报警类型宏定义 //////////////////////////
#define ALM_TYPE_LOLO       1	// 低低报警
#define ALM_TYPE_LO         2	// 低报警
#define ALM_TYPE_HI         3	// 高报警
#define ALM_TYPE_HIHI       4 	// 高高报警
#define ALM_TYPE_ROC        5	// 变化率报警
#define ALM_TYPE_COS		6	// 变位报警
#define ALM_TYPE_OPEN       26	// 0->1报警
#define ALM_TYPE_CLOSE      27	// 1->0报警
// 自定义报警，LA变量使用0-15，MDI变量使用0-7，报警名称自定义
#define ALM_TYPE_USER_DEF0  28	// user defined alarm type			
#define ALM_TYPE_USER_DEF1  29	// user defined alarm type			
#define ALM_TYPE_USER_DEF2  30	// user defined alarm type			
#define ALM_TYPE_USER_DEF3  31	// user defined alarm type			
#define ALM_TYPE_USER_DEF4  32	// user defined alarm type			
#define ALM_TYPE_USER_DEF5  33	// user defined alarm type			
#define ALM_TYPE_USER_DEF6  34	// user defined alarm type			
#define ALM_TYPE_USER_DEF7  35	// user defined alarm type			
#define ALM_TYPE_USER_DEF8  36	// user defined alarm type			
#define ALM_TYPE_USER_DEF9  37	// user defined alarm type			
#define ALM_TYPE_USER_DEF10 38	// user defined alarm type			
#define ALM_TYPE_USER_DEF11 39	// user defined alarm type			
#define ALM_TYPE_USER_DEF12 40	// user defined alarm type			
#define ALM_TYPE_USER_DEF13 41	// user defined alarm type			
#define ALM_TYPE_USER_DEF14 42	// user defined alarm type			
#define ALM_TYPE_USER_DEF15 43	// user defined alarm type	
#define ALM_TYPE_KEEPON     44	// =1状态报警			
#define ALM_TYPE_KEEPOFF    45	// =0状态报警			
#define ALM_TYPE_WILDCARD   255	// 报警通配符

//////////////////// 报警状态宏定义 //////////////////////////
#define ALM_STATUS_OK	0	// no alarm(acked & recovered)			0000 0000
#define ALM_STATUS_ACK	1	// acked alarm(确认)					0000 0001
#define ALM_STATUS_RTN	2	// return to normal(恢复)				0000 0010
#define ALM_STATUS_ALM	3	// in alarm（报警）						0000 0011

#define ALM_STATUS_ALM_MASK		1	// 报警未恢复的MASK				0000 0001
#define ALM_STATUS_UNACK_MASK	2	// 报警未确认的MASK				0000 0010

//////////////////// 报警插件返回值定义 //////////////////////////
#define ALM_RET_SUCCESS				0	// 操作成功
#define ALM_RET_INVALID_PARAMETER	1	// 非法参数
#define ALM_RET_NEED_NOT_SEND		2	// 不需要发送给客户端

//////////////////// 报警插件版本号定义 //////////////////////////
#define ICV_DLL_ALM_INVALID_VERSION 0	// 无效版本号
#define ICV_DLL_ALM_VERSION_ONE		1	// 初始版本号

//////// 报警结构体，只包含插件需要的最常用几种属性 //////////
struct CV_ALARM_MINI
{
	unsigned int	ulAlmSeq;		// alarm sequence
	const char*		pszNodeName;	// node name string ptr
	const char*		pszTagName;		// tag name string ptr
	unsigned char	ucType;			// alarm type
	unsigned char	ucStatus;		// alarm status

	CV_ALARM_MINI()
	{
		ulAlmSeq = 0;
		pszNodeName = NULL;
		pszTagName = NULL;
		ucType = 0;
		ucStatus = 0;
	}
};

#endif //_ALM_MINI_DEF_HEADER_
