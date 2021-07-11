#! /usr/bin/env python 
# -*- coding: utf-8 -*- 

import sys  
reload(sys)  
sys.setdefaultencoding('utf-8')

import os
import sqlite3
import time
import re

import datetime



homeDir = os.path.dirname(os.path.abspath(__file__)) + os.sep + ".."
configDir = homeDir + os.sep + "config"
monitorDbPath = configDir + os.sep + "proces_status.db"
tasklistLogPath = configDir + os.sep + "processlist_temp.txt"
mapProcessToMemory = {}
conn = sqlite3.connect(monitorDbPath)
regex = re.compile('\s+')

def CreateProcessTable():
    sql = "CREATE TABLE IF NOT EXISTS t_process_status(id integer PRIMARY KEY autoincrement, name varchar(32),phymemory integer,phymemory_inc integer, updatetime varchar(20))"

    cursor = conn.cursor()

    cursor.execute(sql)
    print("create table OK, %s,path:%s"%(sql, monitorDbPath))
    cursor.close()
    conn.commit()

def DeleteOldRecord():
    curTime = datetime.datetime.now()
    lastlastMonth =  curTime - datetime.timedelta(days = 31)
    lastMonthStr = lastlastMonth.strftime('%Y-%m-%d %H:%M:%S')
    sql = "delete from t_process_status where updatetime < '%s'"%(lastMonthStr)

    print("delte old records, sql:%s"%(sql))
    cursor = conn.cursor()
    cursor.execute(sql)

    cursor.close()
    conn.commit()

    pass

def LogProcessToDb():
    DeleteOldRecord()
    if(os.path.exists(tasklistLogPath)):
        os.remove(tasklistLogPath)

    cmd="tasklist > %s"%(tasklistLogPath)
    os.system(cmd)
    if(os.path.exists(tasklistLogPath) == False):
        print('create tasklist file failed,%s'%(tasklistLogPath))
        return

    cursor = conn.cursor()

    nowStr = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    totalMemory = 0
    processCount = 0
    with open(tasklistLogPath, 'r') as file_to_read:
        while True:
            line = file_to_read.readline() 
            if not line:
                break
            if len(line) < 10 or line.find('映像名称') >= 0  or line.find('===') >= 0:
                continue

            line = line.replace('System Idle Process','SystemIdleProcess')
            line = line.replace('Memory Compression','MemoryCompression')

            #if(line.find('System Idle Process') >= 0):
            #    continue #System Idle Process replace blank--->System Idle Process
            line = line.replace(',','') #113,640 K--->113640
            parts = regex.split(line) #['QQBrowser.exe', '20884', 'Console', '1', '636,364', 'K']
            if(len(parts) != 7):
                print('process info section num < 6,invalid:%s'%(line))
                continue
            processName = parts[0]
            processId = parts[1]
            processsPhyMemoryK = int(parts[4])
            phyMemIncr = 0
            if(processId not in mapProcessToMemory.keys()):
                mapProcessToMemory[processId] = processsPhyMemoryK

            phyMemLast = mapProcessToMemory[processId]
            phyMemIncr = processsPhyMemoryK - phyMemLast

            totalMemory = totalMemory + processsPhyMemoryK
            sql = "insert into t_process_status(name, phymemory, phymemory_inc,updatetime) values('%s',%d,%d,'%s')"%(processName,processsPhyMemoryK, phyMemIncr, nowStr)
            cursor.execute(sql)
            processCount = processCount + 1

    sql = "insert into t_process_status(name, phymemory, updatetime) values('%s',%s,'%s')" % (
        'allprocess', totalMemory, nowStr)
    cursor.execute(sql)
    cursor.close()
    conn.commit()
    print("%s log success, total memory:%d, process count:%d"%(nowStr, totalMemory, processCount))

if __name__ == '__main__':
    lastExecuteHour = None
    lastExecuteMinute = None
    CreateProcessTable()

    while(True):
        curTime = datetime.datetime.now()
        curHour = curTime.hour
        curMinute = curTime.minute

        if(lastExecuteMinute == None or (curHour != lastExecuteHour and curMinute == 0)):
            LogProcessToDb()
            lastExecuteHour = curHour
            lastExecuteMinute = curMinute
        time.sleep(3)

