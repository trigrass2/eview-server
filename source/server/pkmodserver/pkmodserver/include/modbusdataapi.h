#ifndef _MODBUS_SERVICE_DATAAPI_H_
#define _MODBUS_SERVICE_DATAAPI_H_
// 需实现接口:Init/UnInit/ReadTags/WriteTags 4个导出函数

#include <string>
#include <vector>
#include <cstring>
using namespace std;

#define MAX_TAGNAME_LENGTH			64	// 对象名.属性名，不包含节点名，所以最多32+32个
#define MAX_TAGDATA_LENGTH			256	// 每个tag点最大数值

// 每个tag点的信息和值。用于从过程库读取tag点值的接口。
typedef struct tagModbusTag
{
	char szName[MAX_TAGNAME_LENGTH];
	// 下面这些缓冲区, 是用于ModbusData.dll 写入读取值的
	char szValue[MAX_TAGDATA_LENGTH];	// 调用modbusdata.dll进行读取和写入时，参数都是转换为字符串进行传递。blob则转换为base64，所以不需要长度
	int nStatus; // 读取或者写入的返回值
	void *pInternalRef; // 内部引用，请不要使用！
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
