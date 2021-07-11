#ifndef _PK_INIFILE_H_
#define _PK_INIFILE_H_

#ifdef _WIN32
#	ifdef pkinifile_EXPORTS
#		define PKINIFILE_API __declspec(dllexport)
#	else
#		define PKINIFILE_API __declspec(dllimport)
#	endif
#else // LINUX
#	define PKINIFILE_API __attribute__ ((visibility ("default")))
#endif//_WIN32

#include "stdlib.h"
#include <string>
#include <vector>

class CPKIniFileImpl;
class PKINIFILE_API CPKIniFile
{
public:
	//default constructor
	CPKIniFile();
	CPKIniFile(const char * szFileName, const char *szFilePath = NULL);
	virtual ~CPKIniFile();


public:
    // 如果构造函数中未指定文件，那么这里需要打开一次
	void SetFilePath(const char * szFileName, const char *szFilePath = NULL);

	//get Item and value of string to ini file
	const char * GetString(const char * szSectionName, const char * szItemName, const char * szDefaultValue);

	//get Item and value of int to ini file
	int GetInt(const char * szSectionName, const char * szItemName, int nDefaultValue);

	//get Item and value of float to ini file
	float GetFloat(const char * szSectionName, const char * szItemName, float fDefaultValue);

	//write Item and value of CString to ini file
	bool WriteString(const char * szSectionName, const char * szItemName, const char * szValue);

	//write Item and value of string to ini file
	bool WriteInt(const char * szSectionName, const char * szItemName, int nValue);

	//write Item and value of float to ini file
	bool WriteFloat(const char * szSectionName, const char * szItemName, float fValue);

	// 获得某个小节下的所有的Item项名称数组
	int GetItemNames(const char * szSectionName, std::vector<std::string> &vecItemName);
protected:
	CPKIniFileImpl *m_pIniFileImpl;
};

#endif // _PK_INIFILE_H_