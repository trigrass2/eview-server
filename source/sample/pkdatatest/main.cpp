#include <stdlib.h>
#include <stdio.h>
#include "pkdata/pkdata.h"
//#pragma comment(lib,"pkdata")

#ifdef WIN32
#include <windows.h>
#define strcasecmp stricmp
#else
#include <unistd.h>
#include "math.h"
#endif

void MySleep(int nSleepMS)
{
	#ifdef WIN32
	Sleep(nSleepMS);
	#else
	sleep(ceil(nSleepMS / 1000.0f));
	#endif
}
int main(int argc, char **argv)
{
   //---------[ check the arg count ]----------------------------------------
   if ( argc < 5) {
	  printf("Usage:\n");
	  printf("pkGet example:\t pkdatatest ip -get objectprojpname1|objectprojpname2|... fieldname\n");
	  printf("pkSet example:\t pkdatatest ip -set objectprojpname fieldname tagvalue\n");
	  //printf("pkMGet example:\t pkdatatest ip -mget tag1.value|tag2name1|...\n");
	  printf("pkUpdate: example:\t pkdatatest -update tagname tagvalue\n");
	  MySleep(3);
	  return -1;
   }

   char *pszProcName = argv[0];
   char *pszIp = argv[1];
   char *pszMode = argv[2];
   char *pszTagNames = argv[3];
   
   if(strcasecmp(pszMode, "-get") != 0 && strcasecmp(pszMode, "-mget") != 0
	   && strcasecmp(pszMode, "-set") != 0 && strcasecmp(pszMode, "-update") != 0){
	   printf("mode must be -get/set \n");
	   MySleep(3);
	   return -2;
   } 

   if(strlen(pszTagNames) < 0)
   {
	   printf("tagnames must not be null\n");
	   MySleep(3);
	   return -3;
   }

   PKHANDLE hPkData = pkInit(pszIp, NULL, NULL);
   
   if(strcasecmp(pszMode, "-get") == 0)
   {
	   if(argc < 3){
		   printf("pkSet example:\t pkdatatest -set tagname\n");
		   MySleep(3);
		   return -1;
	   }

	   char szTagData[PKDATA_TAGDATA_MAXLEN]={0};
	   if(strlen(pszTagNames) == 0 )
	   {
		   printf("tagname cannot be null\n");
	   }
	   else
	   {
		   int nTagDataLen = 0;
		   char *pszTagField = argv[4];
		   int nRet = pkGet(hPkData, pszTagNames, pszTagField, szTagData, sizeof(szTagData), &nTagDataLen);
		   if(nRet != 0)
			   printf("pkGet failed,ret:%d", nRet);
		   else
			   printf("pkGet(objectprop:%s, field:%s) success. data:\n%s\n len:%d\n", pszTagNames, pszTagField, szTagData, nTagDataLen);
	   }
   }
   else if (strcasecmp(pszMode, "-set") == 0)
   {
	   if (argc < 6){
		   printf("pkSet example:\t pkdatatest ip -set objectprojpname fieldname tagvalue\n");
		   MySleep(3);
		   return -1;
	   }

	   PKDATA pkData;
	   char *pszTagValue = argv[5];
	   if (strlen(pszTagNames) == 0 || strlen(pszTagValue) == 0)
	   {
		   printf("tagname tagvalue cannot be null\n");
	   }
	   else
	   {
		   char *pszTagField = argv[4];
		   int nRet = pkControl(hPkData, pszTagNames, pszTagField, pszTagValue);
		   if (nRet != 0)
			   printf("pkControl failed,ret:%d", nRet);
		   else
			   printf("pkControl success, objectpropname:%s, fieldname:%s, value:%s\n", pszTagNames, pszTagField, pszTagValue);
	   }
   }

   /*
   else if(strcasecmp(pszMode, "-mget") == 0)
   {
	   PKDATA *pkDatas = new PKDATA[100]();
	   int iTag = 0;
	   char *pTmp = NULL;
	   char *seps = "|,";
	   char *pszTagName = strtok(pszTagNames, seps);
	   while(pszTagName != NULL)
	   {
		   strncpy(pkDatas[iTag].szObjectPropName, pszTagName, sizeof(pkDatas[iTag].szObjectPropName) - 1);
		   strcpy(pkDatas[iTag].szFieldName, pszTagField);
		   pszTagName = strtok(NULL, seps);
		   iTag ++;
	   }

	   // ²»¶Ï¶ÁÈ¡
	   int iReadNum = 0;
	   while(true)
	   {
		   printf("\n[%d]new read request....\n", iReadNum ++);
		   int nRet = pkMGet(hPkData, pkDatas, iTag);
		   if(nRet != 0)
			   printf("pkMGet failed,ret:%d", nRet);
		   else
		   {
			   for(int i = 0; i < iTag; i ++)
				   printf("objectpropname:%s,field:%s,value:%s\n", pkDatas[i].szObjectPropName, pkDatas[i].szFieldName, pkDatas[i].szData);
		   }

		   MySleep(1000);
	   }
   }*/
   else if(strcasecmp(pszMode, "-update") == 0)
   {
	   if(argc < 5){
		   printf("pkUpdate example:\t pkdatatest ip -update tagname tagvalue \n");
		   return -1;
	   }

	   PKDATA pkData;

	   char *pszTagValue = argv[4];
	   if(strlen(pszTagNames) == 0 || strlen(pszTagValue) == 0)
	   {
		   printf("tagname tagvalue cannot be null\n");
	   }
	   else
	   {
		   int nRet = pkUpdate(hPkData, pszTagNames, pszTagValue, 0);
		   if(nRet != 0)
			   printf("pkUpdate(%s=%s) failed,ret:%d", pszTagNames, pszTagValue, nRet);
		   else
			   printf("pkUpdate(%s=%s) success...\n", pszTagNames, pszTagValue);
	   }
   }

   //printf("press any key to exit...");
   //getchar();
   pkExit(hPkData);
}
