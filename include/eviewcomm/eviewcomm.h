#ifndef _EVIEW_COMMON_H_
#define _EVIEW_COMMON_H_

#define TAG_QUALITY_GOOD						0
#define TAG_QUALITY_COMMUNICATE_FAILURE			-10000		// 通信故障,未连接上设备 *
#define TAG_QUALITY_DRIVER_NOT_ALIVE			-20000		// 驱动未启动，**
#define TAG_QUALITY_UNKNOWN_REASON				-30000		// 刚加载时的初始状态，***, 其他时候包括驱动未启动都不能报这个错误！这个状态下不判断设备断开连接的报警和点质量为BAD的报警
#define TAG_QUALITY_INIT_STATE					-90000		// 刚加载时的初始状态，***, 其他时候包括驱动未启动都不能报这个错误！这个状态下不判断设备断开连接的报警和点质量为BAD的报警

#define TAG_QUALITY_COMMUNICATE_FAILURE_STRING	"*"							// 非上述状态的bad
#define TAG_QUALITY_COMMUNICATE_FAILURE_TIP		"device disconnected"		// 设备未连接，*

#define TAG_QUALITY_DRIVER_NOT_ALIVE_STRING		"**"						// 驱动未启动
#define TAG_QUALITY_DRIVER_NOT_ALIVE_TIP		"driver not alive"			// 其他不正常的状态

#define TAG_QUALITY_UNKNOWN_REASON_STRING		"***"						// 原因未知的状态
#define TAG_QUALITY_UNKNOWN_REASON_TIP			"unknown reason"			// 原因未知的状态

#define TAG_QUALITY_INIT_STATE_STRING			"?"							// 刚加载时的初始状态，***, 此时不进行报警的判断
#define TAG_QUALITY_INIT_STATE_TIP				"system init"				// 驱动异常，***

#define PK_SUCCESS						0
#ifndef PK_LOGLEVEL_DEBUG
#define PK_LOGLEVEL_DEBUG				1
#define PK_LOGLEVEL_INFO				2
#define PK_LOGLEVEL_WARN				4
#define PK_LOGLEVEL_ERROR				8
#define PK_LOGLEVEL_CRITICAL			16
#define PK_LOGLEVEL_NOTICE				32
#endif

// TAG点的数据类型.在控制命令时会作为参数传入
#define TAG_DT_UNKNOWN					0		// 未知类型
#define TAG_DT_BOOL						1		// 1 bit value
#define TAG_DT_CHAR						2		// 8 bit signed integer value
#define TAG_DT_UCHAR					3		// 8 bit signed integer value
#define TAG_DT_INT16					4		// 16 bit Signed Integer value
#define TAG_DT_UINT16					5		// 16 bit Unsigned Integer value
#define TAG_DT_INT32					6		// 16 bit Signed Integer value
#define TAG_DT_UINT32					7		// 16 bit Unsigned Integer value
#define TAG_DT_INT64					8		// 16 bit Signed Integer value
#define TAG_DT_UINT64					9		// 16 bit Unsigned Integer value
#define TAG_DT_FLOAT					10		// 32 bit IEEE float
#define TAG_DT_DOUBLE					11		// 64 bit double
#define TAG_DT_TEXT						12		// 4 byte TIME (H:M:S:T)
#define TAG_DT_BLOB						13		// blob, maximum 65535
#define TAG_DT_MAX						13		// Total number of Datatypes
#define TAG_DT_FLOAT16					14

#define DT_NAME_BOOL					"bool"		// 1 bit value
#define DT_NAME_CHAR					"int8"		// 8 bit signed integer value
#define DT_NAME_UCHAR					"uint8"		// 8 bit unsigned integer value
#define DT_NAME_INT16					"int16"		// 16 bit Signed Integer value
#define DT_NAME_UINT16					"uint16"	// 16 bit Unsigned Integer value
#define DT_NAME_INT32					"int32"		// 32 bit signed integer value
#define DT_NAME_UINT32					"uint32"	// 32 bit integer value
#define DT_NAME_INT64					"int64"		// 64 bit signed integer value
#define DT_NAME_UINT64					"uint64"	// 64 bit unsigned integer value
#define DT_NAME_FLOAT					"float"		// 32 bit IEEE float
#define DT_NAME_DOUBLE					"double"	// 64 bit double
#define DT_NAME_TEXT					"text"		// ASCII string, maximum: 127
#define DT_NAME_BLOB					"blob"		// blob, maximum 65535
#define DT_NAME_FLOAT16					"float16"	// 16 bit

#define PK_NAME_MAXLEN					128
#define PK_IOADDR_MAXLEN				128
#define PK_DESC_MAXLEN					256
#define PK_PARAM_MAXLEN					4096
#define PK_USERDATA_MAXNUM				10		// 为驱动开发人员保留的数据数目	

#endif //_EVIEW_COMMON_H_
