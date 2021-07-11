# -*- coding: utf-8 -*-
'''
Created on 2014-9-25

@author: shijunpu
'''

import sys;
import socket,select
#import dataSimulator
from dataSimulator import *
import datetime,time
from time import ctime
import httplib, urllib
import globalVars

#根据所有的配置文件，产生模拟数据
httpClient = None
httpHeaders = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}
datasim = DataSimulator()

def postDataToEViewService(eviewip,eviewport):    
    try:
        httpClient = None
        datasim.simOnce();
        projects = datasim.getSimData()
        for project in projects:
            projectName = project.get("project","")
            devices = project.get("devices",[])
            for device in devices:
                deviceName = device.get("device","")
                attrs = device.get("attrs",{})
                attrs["device"]=deviceName #设备名称作为一个tag点属性
                tagsStr = json.dumps(attrs)
                params = {"project":projectName,"name":tagsStr}
                #strDataObj = json.dumps(dataObj)
                postParams = urllib.urlencode(params)

                httpClient = httplib.HTTPConnection(eviewip, eviewport, timeout=2)
                httpClient.request("POST", "/tags", postParams, httpHeaders)
                globalVars.logMessage("post to %s:%d/tags,project:%s,data:%s"%(eviewip,eviewport,projectName,tagsStr))
                response = httpClient.getresponse()
                #print response.status
                #print response.reason
                #print response.read()
                #print response.getheaders() #获取头信息        
    except Exception, e:
        globalVars.logMessage("post to %s:%d/tags,project:%s failed,e:%s"%(eviewip,eviewport,projectName,e))
    finally:
        if httpClient:
            httpClient.close()
            httpClient = None
                
if __name__ == '__main__':  
    if len(sys.argv) >= 2:     
        option = sys.argv[1];
        if(cmp(option[0:1],"-") == 0):
            print "usage: main ip port.e.g.main 127.0.0.1 8080"
            sys.exit();

    eviewip="127.0.0.1"
    eviewport=8080
    if len(sys.argv) >= 2:     
        eviewip = sys.argv[1];
    if len(sys.argv) >= 3:     
        eviewport = int(sys.argv[2]);
        
    print "start eview sim device data to:%s:%d"%(eviewip,eviewport)

    tmLast = 0
    while True:                
        #定时检查有没有设备需要主动发送数据
        tmNow = time.time()
        if(abs(tmNow - tmLast) < 2):
            time.sleep(1)
            continue
    
        tmLast = tmNow
        try:
            postDataToEViewService(eviewip,eviewport)
        except Exception as e:
            globalVars.logMessage("except in TimerProcess,e:%s"%(e))
        
