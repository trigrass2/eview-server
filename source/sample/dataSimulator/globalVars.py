# -*- coding: utf-8 -*-
'''
Created on 2014-9-26

@author: shijunpu
'''
import thread
import datetime
import struct
import json
import platform

if (platform.system() == "Windows"):   #debug mode
    g_deviceIp = "168.2.237.139"
    g_deviceIp = "127.0.0.1"
    

def logMessage(msg):
    curTime = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S');
    print curTime , ":" , msg
    

def getHostId():
    global g_hostId
    #获取hostID
    try:
        hostIdFilePath = "../etc/hostid";
        fHostId = open(hostIdFilePath,'r')
        for  line in  fHostId.readlines(): 
            g_hostId = line
            g_hostId = g_hostId.rstrip()
            break
        print "hostid:%s" % (g_hostId)      
          
        #os.system("echo %s > %s" % (macAddr, hostIdFilePath))
    except Exception as e:
        logMessage("open file %s failed:%s,hostid is -no-host-id-" % (hostIdFilePath ,e))   

def init():
    getHostId()
    
def logHex(tip,buffer):
    curTime = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S');
    disp = "%s:%s,bufLen:%d,hex:" % (curTime,tip,len(buffer))
    for oneByte in buffer:
        oneStr = struct.unpack("B",oneByte)
        disp = disp + " %02X" % (oneStr)
    print disp
    
 
