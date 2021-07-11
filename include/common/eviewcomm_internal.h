#pragma once
#include "pkdbapi/pkdbapi.h"
#include "eviewcomm/eviewcomm.h"


#define PROCESSQUEUE_DATA_TO_NODESERVER_AREASIZE_MIN	10*1024*1024		// 驱动发送数据给的共享内存的大小, 10M
#define PROCESSQUEUE_DATA_TO_NODESERVER_AREASIZE_MAX	200*1024*1024		// 驱动发送数据给共享内存的大小, 200M

#define PROCESSQUEUE_CONTROL_TO_NODESERVER_AREASIZE		4*1024*1024			// 共享内存的大小, 4M

//#define PROCESSQUEUE_CONTROL_TO_DRIVER_AREASIZE_MIN		2*1024*1024			// 发送数据给驱动的共享内存的最小值, 1M
//#define PROCESSQUEUE_CONTROL_TO_DRIVER_AREASIZE_MAX		100*1024*1024		// 发送数据给驱动的共享内存的最大值, 40M

//#define	PROCESSQUEUE_PROCESSMGR_CONTROL_AREASIZE		40 * 1024			// 进程管理器的控制命令共享内存大小
//#define	PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGHT	4 * 1024				// 每条进程管理器的控制命令大大小

#define	PROCESSQUEUE_TAG_DATASIZE_LENGHT				1*1024*1024			// 每条TAG点的最大值
#define	PROCESSQUEUE_TAG_VTQ_LENGHT						(1*1024*1024+40)	// 每条TAG点的VTQ的最大值
#define	PROCESSQUEUE_QUEUE_ONRECORD_LENGHT				PROCESSQUEUE_TAG_VTQ_LENGHT	// 每条TAG点的VTQ的最大值

#define PK_TIMESTRING_MAXLEN							24	// YYYY-MM-DD HH:SS:MM.xxx

class PKEviewComm
{
public:
	static int GetVersion(CPKDbApi &dbApi, const char *szModuleName); // 获取某个模块的版本号
	static int LoadSysParam(CPKDbApi &dbApi, char *szParamName, char *szDefaultValue, string &strParamValue);  // 获取缺省系统参数
};

