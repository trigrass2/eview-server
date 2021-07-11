  # -*- coding: utf-8 -*-
'''
Created on 2014-9-25

@author: shijunpu
'''

import socket
import datetime,time  # for use of subclass
import json
import random


# sys.path.append("../") 


class DataSimulator:  
    def __init__(self):  
        self.simProjects = {}; # 模拟对象配置
        self.readSimRules();
        
    def logMessage(self,msg):
        curTime = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S');
        print curTime , ":" , msg
        
    # 读取模拟数据的规则配置文件，json格式
    #{"tag1":{type:int,autoinc:1,min:1,max:100},"tag2":{type:int,autoinc:1,min:1,max:100}}
    def readSimRules(self):
        simProjects = []
        # 每一行的格式：Socket;1B7C4501004B1200
        simRuleFilePath = "./simrules.cfg"
        try:
            fh = open(simRuleFilePath)
            cfgStr = fh.read();
            try:
                self.simProjects = json.loads(cfgStr,encoding="ascii")
                self.checkSimConfig();
            except Exception as e:
                print ("file %s,json format invalid:%s" % (simRuleFilePath , e))
            fh.close()
    
        except Exception as e:
            print ("open file %s failed:%s" % (simRuleFilePath , e))
        
        return self.simProjects
        
    #检查各个配置项的合法性
    def checkSimConfig(self):
        if(isinstance(self.simProjects,list) == False):
            print "配置文件simrules.cfg必须包含project的数组!"
            return False
        
        #对每一个项目配置
        for simProject in self.simProjects:
            projectName = simProject.get("project","")
            if(projectName == ""):
                print "root node must have not empty project attribute!"
                self.simProjects = None
                return False
            
            devices = simProject.get("devices",[])            
            if(isinstance(devices,list) == False or len(devices) <= 0):
                print "root node must have devices(list) attributes!"
                self.simProjects = None
                return False
            
            for device in devices:
                deviceName = device.get("device","")
                if(deviceName == ""):
                    print "device node must have not empty device attribute!"
                    self.simProjects = None
                    return False
                
                attrs = device.get("attrs",{})            
                if(isinstance(attrs,dict) != True):
                    print "device node must have attrs(dict) attributes!"
                    self.simProjects = None
                    return False
                
                for (key,simObj) in attrs.items():
                    type = simObj.get("type","none");
                    
                    if(type == "float" or type == "double" or type == "real"):
                        type = "float";
                    elif(type == "int" or type == "integer" or type == "number"):
                        type = "int"
                    elif(type == "string" or type == "str"):
                        type = "string"
                    elif(type == "time" or type == "dt"):
                        type = "time"
                    else:
                        print "sim configs:key:%s,type:%s invalid,assume:string" %(key,type)
                        type = "string"
                        
                    try:
                        if(type == "int" or type == "float"):
                            if(simObj.get("min") == None):
                                simObj["min"] = 0; # 最小值为0
                            simObj["min"] = float(simObj["min"])
                            
                            if(simObj.get("max") == None):
                                simObj["max"] = 100000; # 最小值为0
                            simObj["max"] = float(simObj["max"])
                            
                            if(simObj.get("autoinc") == None or simObj.get("autoinc") == "0"):
                                simObj["autoinc"] = False;
                            else:
                                simObj["autoinc"] = True;
                            
                            if(simObj.get("init")==None): #init simObj
                                simObj["init"] = simObj["min"]; 
                            simObj["init"] = float(simObj["init"])
                            
                            if(simObj.get("interval") == None): #间隔
                                simObj["interval"]= 10 #
                            simObj["interval"] = float(simObj["interval"])
                        elif(type == "time"):
                            if(simObj.get("format","") == ""):
                                simObj["format"] = "%Y-%m-%d %H:%M:%S"
                            simObj["init"] = datetime.datetime.now().strftime(simObj["format"]);
                        else: #string,must have opts
                            if(simObj.get("opts") == None or len(simObj)<=0):
                                simObj["opts"]=["test1","test2","test3"]
                            simObj["init"] = simObj["opts"][0] #初始值
                            
                        simObj["curValue"] = simObj["init"]; #curvalue=initvalue
                    except e,Exception:
                        print "error:%s" %(e)
                        return False
            
        return True   
            
    #所有的数据均产生一次模拟数据
    def simOnce(self):
        if(self.simProjects == None or len(self.simProjects)<=0):
            print "no valid sim rules!"
            return
        
        for project in self.simProjects:
            projectName = project.get("project","")
            devices = project.get("devices",[])
            attrnum = 0
            for device in devices:
                oneObjectAttrs = device.get("attrs",{})
                for (key,devParam) in oneObjectAttrs.items(): #对于每一个设备的一个属性
                    if(devParam["type"] == "float" or devParam["type"]=="int"):
                        if(devParam["autoinc"]): #auto increment
                            randIntv = random.random() * float(devParam["interval"]) #0.0....1
                            devParam["curValue"] += randIntv;
                            if(devParam["curValue"] > devParam["max"]):
                                devParam["curValue"] = devParam["min"]
                        else:
                            randIntv = random.uniform(devParam["min"],devParam["max"]) #0.0....1
                            devParam["curValue"] = randIntv
                        
                        if(devParam["type"] == "float"):
                            devParam["curValue"] = round(devParam["curValue"],2)
                        else:
                            devParam["curValue"] = int(devParam["curValue"])
                    elif(devParam["type"] == "time"):
                        devParam["curValue"] = datetime.datetime.now().strftime(devParam["format"]);
                    else:#string
                        devParam["curValue"] = random.choice(devParam["opts"])
                    attrnum += 1
            self.logMessage("-----------sim project:%s with %d devices %d attributes--------"%(projectName,len(devices),attrnum))
    def getSimData(self):
        if(self.simProjects == None):
            return None
        
        projectsVal = []
        for project in self.simProjects:
            projectVal = {}
            devicesVal = []
            projectName = project.get("project","")
            devices = project.get("devices",[])
            for device in devices:
                deviceName = device.get("device","")
                attrs = device.get("attrs",{})
                attrsVal = {}
                for (key,devParam) in attrs.items(): #对于每一个设备的一个属性
                    try:
                        simVal = devParam["curValue"]
                        attrsVal[key] = str(simVal)
                    except Exception as e:
                        print("except in getSimData,e:%s"%(e))
                deviceVal = {"device":deviceName,"attrs":attrsVal}
                devicesVal.append(deviceVal)
            projectVal = {"project":projectName,"devices":devicesVal}
            projectsVal.append(projectVal)        
        return projectsVal