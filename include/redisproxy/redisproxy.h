#ifndef CREDISPROXY_HXX
#define CREDISPROXY_HXX

#include <string>
#include <list>
#include <set>
#include <map>
#include <vector>

using namespace std;
typedef void * HREDIS;
typedef map<string, bool> SubChannelMap;
typedef SubChannelMap::iterator SubChannelMapIter;
struct redisReply;

class CRedisProxy
{
public:
	CRedisProxy(void);
	~CRedisProxy(void);
	int Initialize(string strAddr, unsigned short usPort, string strPass, int nDBIndex = 0);
	int Finalize();
	int PSubChannel(string strLesseeGB, string strDataSourceGB);
	int PSubChannel(string strChannelGB);
	int PublishMsg(string strChannelGB, string strMsgGB);
	int UnPSubChannel(string strLesseeGB, string strDataSourceGB);
	int UnPSubChannel(string strChannelGB);
	int ExecuteCmd(vector<string> &cmdArgvGB);
	int ExecuteCmd(string strCmdGB);
	int ExecuteCmd( vector<string> &cmdArgvGB , vector<string>& strElemsGB);
	int RecvChannelMsg(string &strChannelGB, string &strMsgGB, int nTimeoutMS = 2000);
	string LoadLuaScript(string strLuaScript);
	int Connect(bool *bReconnect = NULL);
	int DisConnect();
	int SubChannel(string strChannelGB);
	int UnSubChannel(string strChannelGB);
	int ExecuteCmd( vector<string> &cmdArgvGB , string& strOutGB);
	int redisCmds(vector<string> &vecCmdsGB , vector<redisReply *>& vecRepliesUTF);
	redisReply * redisCmd(const char *szCmdGB);

	int checkConnect();
	int get(const char *szKeyGB, string & strValueGB);
	int set(const char *szKeyGB, const char *szValueGB);
	int del(const char *szKeyGB);

	int mset(vector<string>& keysGB, vector<string>& valuesGB);
	int mget(vector<string> keysGB, vector<string> & valuesGB);

	int rpush(const char *szKeyGB, const char *szValueGB);

	int redisCommon(const char *formatUTF, ...);
	int redisCommon(vector<string>& vecOutUTF, const char *formatUTF, ...);

	int redisCommon(vector<string> &cmdsGB);
	int hset(const char *szKeyGB, const char *szFieldNameGB, const char *szFieldValueGB);
	int hdel(const char *szKeyGB, const char *szFieldNameGB);
	int hget(const char *szKeyGB, const char *szFieldNameGB, string &szFieldValueGB);
	int hmget(const char *szKeyGB, vector<string> fieldsGB, vector<string>& valuesGB);
	int hmset_pipe(vector<string> & vecKeys, vector<string> & vecField, vector<string> & vecValue);
	int pipe_append(const char *formatUTF, ...);
	int pipe_getReply(int nCmdNum, vector<string> &vecResult);
private:
	HREDIS m_hRedis;
	SubChannelMap m_subChannelMap;
	int	m_nDBIndex; // 连接的是哪个DB（0开始）
	char *m_szCmdBuf;
	int getStringFromReply(void *pContextInfo, redisReply *reply, string &strValueGB);

};

#endif