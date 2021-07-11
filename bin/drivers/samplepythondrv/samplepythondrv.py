#! /usr/bin/python
# coding=utf-8

import sys
import struct
import os

python_file=__file__
bin_path=os.path.dirname(__file__)+ '\\..\\..'
python_path=os.path.dirname(__file__)+'\\..\\..\\..\\python'
print(bin_path,python_path)
sys.path.append(bin_path)
sys.path.append(python_path)
print(sys.path)
import pydriver

def InitDriver(pkDriver):
    print("----InitDriver----")
    print(pkDriver)
    return 0

def UnInitDriver(pkDriver):
    print("----UnInitDriver----")
    print(pkDriver)
    return 0

def InitDevice(pkDevice):
    print("----InitDevice----")
    print(pkDevice)
    tags=[]
    for tag in pkDevice['tags']:
        tags.append(tag)
    print("----InitDevice2----")
    timerObj=pydriver.CreateTimer(pkDevice, 3000, tags)
    return 0

def UnInitDevice(pkDevice):
    print("UnInitDevice")
    return 0

def OnTimer(pkDevice, timerObj, pkTimerParam):
    print("----OnTimer----")
    tags = pkTimerParam
    print(timerObj)
    print(tags)

    id = 1
    tag="100"
    version = 2
    count = 3
    buffer = struct.pack("!H4s2I", id, tag, version, count)

    print("before send...")
    result = pydriver.Send(pkDevice, buffer, 2000)
    print("SendToDevice:",result)
    if(result <= 0):
        print("send to device return <-0")
        print("send to device:", len(buffer) , "return :" , result)
        return

    retCode, bufferRecv = pydriver.Recv(pkDevice, 10000, 200)
    print("Recv,retcode:", retCode, ",length:",len(bufferRecv))

    if(retCode != 0):
        return

    # 解析数据，得到值
    tagValue =struct.unpack("!H", bufferRecv)
    print("recv tagValue:",tagValue[0])
    #print("recv,id:",id,",tag:",tag,",version:",version,",count:",count

    tagAddr = "AI:10"
    print(tagAddr)
    tagList = pydriver.GetTagsByAddr(pkDevice, tagAddr)
    pydriver.SetTagsValue(pkDevice,tagList, str(tagValue[0]))
    pydriver.UpdateTagsData(pkDevice, tagList)
    print("-OnTimer- end")
    return 0

def OnControl(pkDevice, pkTag, strTagValue):
    print("-!--OnControl--!-")
    print(pkDevice,pkTag)
    print("device name:",pkDevice['name'], "tagname:", pkTag['name'], "tagaddress:",pkTag['address'], "tagvalue:",strTagValue)
    return 0

