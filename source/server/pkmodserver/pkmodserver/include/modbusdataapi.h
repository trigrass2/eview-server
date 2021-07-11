#ifndef _MODBUS_SERVICE_DATAAPI_H_
#define _MODBUS_SERVICE_DATAAPI_H_
// ��ʵ�ֽӿ�:Init/UnInit/ReadTags/WriteTags 4����������

#include <string>
#include <vector>
#include <cstring>
using namespace std;

#define MAX_TAGNAME_LENGTH			64	// ������.���������������ڵ������������32+32��
#define MAX_TAGDATA_LENGTH			256	// ÿ��tag�������ֵ

// ÿ��tag�����Ϣ��ֵ�����ڴӹ��̿��ȡtag��ֵ�Ľӿڡ�
typedef struct tagModbusTag
{
	char szName[MAX_TAGNAME_LENGTH];
	// ������Щ������, ������ModbusData.dll д���ȡֵ��
	char szValue[MAX_TAGDATA_LENGTH];	// ����modbusdata.dll���ж�ȡ��д��ʱ����������ת��Ϊ�ַ������д��ݡ�blob��ת��Ϊbase64�����Բ���Ҫ����
	int nStatus; // ��ȡ����д��ķ���ֵ
	void *pInternalRef; // �ڲ����ã��벻Ҫʹ�ã�
	tagModbusTag()
	{
		memset(szName, 0, sizeof(szName));
		memset(szValue, 0, sizeof(szValue));
		nStatus = 0;
		pInternalRef = NULL;
	}
}ModbusTag;

typedef vector<ModbusTag *> ModbusTagVec;


#endif //_MODBUS_SERVICE_DATAAPI_H_
