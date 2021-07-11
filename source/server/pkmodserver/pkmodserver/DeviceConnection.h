#pragma once

using namespace std;

typedef void(*PFN_ConnCallback)(void *pConnParam, bool bConnected);
class CDeviceConnection
{
public:
	CDeviceConnection()
	{
		m_bEnableConnect = true;
		m_strConnName = m_strConnParam = "";
		m_pfnConnCallback = NULL;
		m_pConCallbackParam = NULL;
	}
	virtual ~CDeviceConnection(void){}
	bool m_bEnableConnect;
	string m_strConnName;
	string m_strConnParam;

	PFN_ConnCallback m_pfnConnCallback;
	void *m_pConCallbackParam;

	void SetConnCallback(PFN_ConnCallback pfnConnCallback, void *pConnCallbackParam)
	{
		m_pfnConnCallback = pfnConnCallback;
		m_pConCallbackParam = pConnCallbackParam;
	}
public:
	virtual long Start() = 0;
	virtual long Stop() = 0;
	virtual void SetConnectName(const char *szName)
	{
		m_strConnName = szName;
	}
	// 形式: IP:192.168.1.5;Port:502;ConnTimeOut:3000
	virtual long SetConnectParam(char *szConnParam)=0;
	// 连接设备
	virtual long CheckAndConnect()=0;
	// 断开连接
	virtual long DisConnect()=0;
	// 发送数据
	virtual long Send(const char* pszSendBuf, long nSendBytes, long lTimeoutMs)=0;
	virtual long Recv(char* szRecvBuf, long nMaxRecvBytes, long lTimeoutMs)=0;
	virtual long Recv_n(char* szRecvBuf, long nExactRecvBytes, long lTimeoutMs)=0;
	virtual bool IsConnected()=0;
	virtual void ClearDeviceRecvBuffer()=0;
	virtual void SetEnableConnect(bool bEnableConnect)
	{
		m_bEnableConnect = bEnableConnect;
		if (!bEnableConnect)
			DisConnect();
	}
	virtual bool GetEnableConnect()
	{
		return m_bEnableConnect;
	}
};

