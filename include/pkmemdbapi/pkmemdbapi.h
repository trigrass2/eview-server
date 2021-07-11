#ifndef _PEAK_MEMORYDB_H_
#define _PEAK_MEMORYDB_H_

#ifdef _WIN32
#	ifdef pkmemdbapi_EXPORTS
#		define PKMEMDB_DLLEXPORT __declspec (dllexport)
#	else
#		define PKMEMDB_DLLEXPORT __declspec (dllimport)
#	endif
#else
#	define PKMEMDB_DLLEXPORT //extern "C"
#endif

#include <string>
#include <vector>
#include <map>
#include "stdlib.h"
#include "stdio.h"

typedef void (*PFN_OnDisconnect)(void *pCallbackParam);
typedef void (*PFN_OnChannelMsg)(const char *szChannelName, const char *szMessage, void *pCallbackParam); // ���͵Ļص�����������Ҫ��������ʱ�������м���Ҫ���ؽ��ȣ���Ҫ�趨ÿ�����ĳ���

class PKMEMDB_DLLEXPORT CMemDBClientAsync
{
public:
	CMemDBClientAsync();
	virtual ~CMemDBClientAsync();
	int initialize(const char * szAddr, unsigned short usPort, const char * szPassword, int nDBIndex, 
		PFN_OnDisconnect pfnOnDisconnect = NULL, void *pCallbackParam = NULL);
	void finalize();
	void disconnect();
	bool isconnect();
	int select(int index);

	int commit();
	int sync_commit();

	int publish(const char * szChannelGB, const char * szMsgGB);
	int set(const char * szKeyGB, const char * szValueGB);
	const char * get(const char *szKeyGB);
	int mset(const char *szKeysGB, const char *szValueGB);
	const char * mget(const char *szKeysGB);

	int mset(const std::vector<std::string> &vecKeys, const std::vector<std::string> &vecValuess);
	int mget(const std::vector<std::string> &vecKeys, std::vector<std::string> &vecValues);
	int keys(const char *szPattern, std::vector<std::string> &vecKeys);

	//int mset(const std::vector<std::pair<std::string, std::string>>& key_vals);
	int hset(const char * szkey, const char * szfield, const char * szvalue);
	//int hmset(const char * szkey, const std::vector<std::pair<std::string, std::string>>& field_val);
	int del(const char * szkey);
	const char * keys(const char *szPattern); // �����ַ������������ַ�����^����

	int  flushall();
	int  flushdb();

public:
	char m_szAddr[32];
	char m_szPassword[32];
	int	m_nDBIndex;
	int m_nPort;
	PFN_OnDisconnect m_pfnOnDisconnect;
	void *m_pOnDisconnectParam;

protected:
	void *m_pMemDB;
	time_t	m_tmLastConn;

protected:
	int connect();
	bool checkConnect();
};

class PKMEMDB_DLLEXPORT CMemDBSubscriberAsync
{
public:
	CMemDBSubscriberAsync();
	~CMemDBSubscriberAsync();
	int initialize(const char *szAddr, unsigned short usPort, const char * szPassword, 
		PFN_OnDisconnect pfnOnDisconnect = NULL, void *pCallbackParam = NULL);
	void finalize();
	void disconnect();
	bool isconnect();

    int subscribe(const char *szChannel, PFN_OnChannelMsg pfnOnChannelMsg, void *pCallbackParam);
    int unsubscribe(const char *szChannel);
    int commit();

	bool checkConnect();
	bool connect();
public:
	char m_szAddr[32];
	char m_szPassword[32];
	int	m_nDBIndex;
	int m_nPort;

	static PFN_OnDisconnect m_pfnOnDisconnect;
	static void *m_pOnDisconnectParam;
protected:
	void *m_pMemDB;


	static PFN_OnChannelMsg m_pfnOnChannelMsg;
	static void *m_pOnChannelMsgParam;

	time_t	m_tmLastConn;

protected:

	void onSubscriberSubscribed(const std::string& channel, const std::string& msg);// client_disconnection_callback_t;
};

class PKMEMDB_DLLEXPORT CMemDBClientSync
{
public:
	CMemDBClientSync();
	virtual ~CMemDBClientSync();
	int initialize(const char * szAddr, unsigned short usPort, const char * szPassword, int nDBIndex);
	void finalize();
	bool checkConnect();

	int select(int index);

	int publish(const char * szChannelGB, const char * szMsgGB);
	int set(const char * szKeyGB, const char * szValueGB);
	int get(const char *szKeyGB, std::string &strValueGB);
	int mset(const std::vector<std::string> &vecKeys, const std::vector<std::string> &vecValuess);
	int mget(const std::vector<std::string> &vecKeys, std::vector<std::string> &vecValues);
	int keys(const char *szPattern, std::vector<std::string> &vecKeys);
	int hset(std::string &strKey, std::string &strfield, std::string &strValue);
	int del(std::string &strKey);
 
	int  flushall();
	int  flushdb();

public:
	char m_szAddr[32];
	char m_szPassword[32];
	int	m_nDBIndex;
	int m_nPort;

protected:
	void *m_pMemDB;
	time_t	m_tmLastConn; 	// �ϴγ������ӵ�ʱ��
};


class PKMEMDB_DLLEXPORT CMemDBSubscriberSync
{
public:
	CMemDBSubscriberSync();
	~CMemDBSubscriberSync();
	int initialize(const char *szAddr, unsigned short usPort, const char * szPassword);
	void finalize();
	bool checkConnect();


	int subscribe(const char *szChannel); // ���Ե��ö�Σ��Ա�ͬʱ���Ķ��ͨ����
	int unsubscribe(const char *szChannel);
	int receivemessage(char *szChannel, int nChannelLen, int *pnActChannelLen, char *szMsg, int nMsgLen, int *pnActMsgLen, int nTimeOutMS); // channel��message��Ϊ�������
	int receivemessage(std::string &strChannelOut, std::string &strMessageOut, int nTimeOutMS); // channel��message��Ϊ�������
public:
	char m_szAddr[32];
	char m_szPassword[32];
	int m_nPort;

protected:
	void *m_pMemDB;
	time_t	m_tmLastConn;
};


class CVecHelper
{
public:
	CVecHelper() {};
	~CVecHelper() {};
	static int String2Vector(std::string strInput, std::vector<std::string> &vecResult) // ��^����������^��һ������^��һ������^�ڶ�������^�ڶ�������......
	{
		vecResult.clear();
		char chSeparator = '^';
		// �õ�Ԫ�ظ�����
		int nNum = 0;
		int nPos = strInput.find(chSeparator);
		if (nPos >= 0)
		{
			std::string strNum = strInput.substr(0, nPos);
			strInput = strInput.substr(nPos + 1);
			nNum = ::atoi(strNum.c_str());
		}
		if (nNum < 0)
			return -1;

		// ����ȡ��ÿһ��Ԫ�أ�
		for (int i = 0; i < nNum; i++)
		{
			// ȡ�õ�i��Ԫ�صĳ���
			nPos = strInput.find(chSeparator);
			if (nPos < 0)
				break;

			std::string strElemLen = strInput.substr(0, nPos);
            unsigned int nElemLen = ::atoi(strElemLen.c_str());
			if (strInput.length() < nElemLen)
				return -2;

			strInput = strInput.substr(nPos + 1); // ʣ�಻�����ȵ��ַ���
			std::string strElem = strInput.substr(0, nElemLen);
			vecResult.push_back(strElem);
			strInput = strInput.substr(nElemLen + 1); //ȥ����Ԫ��
		}

		return 0;
	}

	static int Vector2String(std::vector<std::string> &vecResult, std::string &strOutput) // ��^����������^��һ������^��һ������^�ڶ�������^�ڶ�������......
	{
		strOutput = "";
		char szLen[32] = { 0 };
		char chSeparator = '^';
		// ��Ԫ�ظ���
		sprintf(szLen, "%d%c", vecResult.size(), chSeparator); // ����
		strOutput = szLen;

		for (std::vector<std::string>::iterator itElem = vecResult.begin(); itElem != vecResult.end(); itElem++)
		{
			std::string &strElem = *itElem;
			sprintf(szLen, "%d%c", strElem.length(), chSeparator); // ��i���ַ���Ԫ�صĳ���
			strOutput += szLen;
			strOutput = strOutput + strElem + chSeparator;
		}

		return 0;
	}
};
#endif // _PEAK_MEMORYDB_H_
