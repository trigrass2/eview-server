#! /usr/bin/python
# coding=utf-8

import sys
import struct
import os
from ctypes import *
import ctypes

this_file=os.path.realpath(__file__) #
print("this_file:",this_file)
bin_path=os.path.dirname(this_file)+ '\\..\\..'
python_path=os.path.dirname(this_file)+'\\..\\..\\..\\python'
print(bin_path,python_path)
sys.path.append(bin_path)
sys.path.append(python_path)
print(sys.path)
import pydriver

#from pytestdrv import *

#仅供调试使用，可以直接运行这个python文件进行调试
if __name__ == '__main__':
    pkdrvcmn_path=bin_path+"/pkdrvcmn"
    drv_path=bin_path+"/drivers/pytestdrv"
    fileName = "pytestdrv"
    print(pkdrvcmn_path)
    lib = cdll.LoadLibrary(pkdrvcmn_path)
    lib.drvMain.argtypes = (ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int)
    lib.drvMain("pytestdrv",drv_path,0)

    # pkDriver={'name': '', '_InternalRef': 0, 'param4': '', 'param3': '', 'param2': '', 'param1': '', 'type': '', 'desc': ''}
    # pkDevice={'recvtimeout': 500, 'name': 'pytestdevice', 'tags': [{'_ctag': 20857952, 'name': 'CELL06_BLD1', 'lenbits': 16, 'datatype': 'uint16', 'address': 'AI:17023', 'pollrate': 3000}, {'_ctag': 20859696, 'name': 'CELL06_BLD2', 'lenbits': 16, 'datatype': 'uint16', 'address': 'AI:17023', 'pollrate': 3000}, {'_ctag': 20861440, 'name': 'CELL06_OKjishu', 'lenbits': 16, 'datatype': 'uint16', 'address': 'AI:17007', 'pollrate': 3000}, {'_ctag': 20863184, 'name': 'CELL06_NGjishu', 'lenbits': 16, 'datatype': 'uint16', 'address': 'AI:17036', 'pollrate': 3000}], '_InternalRef': 0, 'driver': {'name': '', '_InternalRef': 0, 'param4': '', 'param3': '', 'param2': '', 'param1': '', 'type': '', 'desc': ''}, 'conntype': 'tcpclient', 'param4': '', 'param3': '', 'param2': '', 'param1': '', 'conntimeout': 3000, 'connparam': 'ip=127.0.0.1;port=502', 'desc': ''}
    # timerObj={'period': 3000}
    # pkTimerParam=[{'_ctag': 23044952, 'name': 'CELL06_BLD1', 'lenbits': 16, 'datatype': 'uint16', 'address': 'AI:17023', 'pollrate': 3000}, {'_ctag': 23012072, 'name': 'CELL06_BLD2', 'lenbits': 16, 'datatype': 'uint16', 'address': 'AI:17023', 'pollrate': 3000}, {'_ctag': 23013816, 'name': 'CELL06_OKjishu', 'lenbits': 16, 'datatype': 'uint16', 'address': 'AI:17007', 'pollrate': 3000}, {'_ctag': 23015560, 'name': 'CELL06_NGjishu', 'lenbits': 16, 'datatype': 'uint16', 'address': 'AI:17036', 'pollrate': 3000}]
    # InitDriver(pkDriver)
    # InitDevice(pkDevice)
    # OnTimer(pkDevice, timerObj, pkTimerParam)

