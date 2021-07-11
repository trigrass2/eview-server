#pragma once
#include "pkdbapi/pkdbapi.h"
#include "eviewcomm/eviewcomm.h"


#define PROCESSQUEUE_DATA_TO_NODESERVER_AREASIZE_MIN	10*1024*1024		// �����������ݸ��Ĺ����ڴ�Ĵ�С, 10M
#define PROCESSQUEUE_DATA_TO_NODESERVER_AREASIZE_MAX	200*1024*1024		// �����������ݸ������ڴ�Ĵ�С, 200M

#define PROCESSQUEUE_CONTROL_TO_NODESERVER_AREASIZE		4*1024*1024			// �����ڴ�Ĵ�С, 4M

//#define PROCESSQUEUE_CONTROL_TO_DRIVER_AREASIZE_MIN		2*1024*1024			// �������ݸ������Ĺ����ڴ����Сֵ, 1M
//#define PROCESSQUEUE_CONTROL_TO_DRIVER_AREASIZE_MAX		100*1024*1024		// �������ݸ������Ĺ����ڴ�����ֵ, 40M

//#define	PROCESSQUEUE_PROCESSMGR_CONTROL_AREASIZE		40 * 1024			// ���̹������Ŀ���������ڴ��С
//#define	PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGHT	4 * 1024				// ÿ�����̹������Ŀ���������С

#define	PROCESSQUEUE_TAG_DATASIZE_LENGHT				1*1024*1024			// ÿ��TAG������ֵ
#define	PROCESSQUEUE_TAG_VTQ_LENGHT						(1*1024*1024+40)	// ÿ��TAG���VTQ�����ֵ
#define	PROCESSQUEUE_QUEUE_ONRECORD_LENGHT				PROCESSQUEUE_TAG_VTQ_LENGHT	// ÿ��TAG���VTQ�����ֵ

#define PK_TIMESTRING_MAXLEN							24	// YYYY-MM-DD HH:SS:MM.xxx

class PKEviewComm
{
public:
	static int GetVersion(CPKDbApi &dbApi, const char *szModuleName); // ��ȡĳ��ģ��İ汾��
	static int LoadSysParam(CPKDbApi &dbApi, char *szParamName, char *szDefaultValue, string &strParamValue);  // ��ȡȱʡϵͳ����
};

