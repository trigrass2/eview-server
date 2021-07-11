#include "ftpclient.h"
#include <stdio.h>

int main()
{
	ftpclient oFtp("port", "bin");

	//int nRet = oFtp.connect("192.168.10.1", 21);
	int nRet = oFtp.connect("192.168.10.236", 21);
	if(nRet != 0)
	{
		printf("connect failed\n");
		getchar();
		return 0;
	}

	//nRet = oFtp.login("Anonymous", "2@2.net");
	nRet = oFtp.login("admin", "admin");
	if(nRet != 0)
	{
		printf("login  failed\n");
		getchar();
		return 0;
	}

	//oFtp.size("./test_put.txt");
	//oFtp.list("/",buf);
	//nRet = oFtp.gets("./desktop.ini", "d:\\temp\\");
	/*nRet = oFtp.get("./3.txt", "d:\\temp\\3.txt");
	if(nRet != 0)
	{
		printf("gets file failed\n");
		getchar();
		return 0;
	}*/

	nRet = oFtp.put("d:\\temp\\2.xxx.txt", "./32.xxx");
	if(nRet != 0)
	{
		printf("put file failed\n");
		getchar();
		return 0;
	}

	nRet = oFtp.put("d:\\temp\\2.xxx.txt", "./2.xxx");
	if(nRet != 0)
	{
		printf("put file failed\n");
		getchar();
		return 0;
	}
	//oFtp.dir("./");
	//oFtp.puts("D:\\dir1", "./test");
	//oFtp.put("d:/test2.txt", "./test/test2.txt");
	//oFtp.put("d:/tstmrapi.ini", "./test/tstmrapi.ini");
	//oFtp.put("d:/testapi.ini", "./test/testapi.ini");


	getchar();
	return 0;
}