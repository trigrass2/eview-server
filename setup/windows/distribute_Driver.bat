@echo on 

echo copy drivers
rem copy drivers
rem 已经内置在nodeserver xcopy ..\..\bin\drivers\simdriver\*.dll .\eview-server\bin\drivers\simdriver\ /y
rem 已经内置在nodeserver xcopy ..\..\bin\drivers\memvardriver\*.dll .\eview-server\bin\drivers\memvardriver\ /y
xcopy ..\..\bin\drivers\abcipdrv\*.dll .\eview-server\bin\drivers\abcipdrv\ /y
xcopy ..\..\bin\drivers\cdt\*.dll .\eview-server\bin\drivers\cdt\ /y
xcopy ..\..\bin\drivers\dbdrv\*.dll .\eview-server\bin\drivers\dbdrv\ /y
xcopy ..\..\bin\drivers\gwmqttdrv\*.dll .\eview-server\bin\drivers\gwmqttdrv\ /y
xcopy ..\..\bin\drivers\hostdriver\*.dll .\eview-server\bin\drivers\hostdriver\ /y
xcopy ..\..\bin\drivers\iec104\*.dll .\eview-server\bin\drivers\iec104\ /y
xcopy ..\..\bin\drivers\mnmqttdrv\*.dll .\eview-server\bin\drivers\mnmqttdrv\ /y
xcopy ..\..\bin\drivers\modbusrtu\*.dll .\eview-server\bin\drivers\modbusrtu\ /y
xcopy ..\..\bin\drivers\modbustcp\*.dll .\eview-server\bin\drivers\modbustcp\ /y
xcopy ..\..\bin\drivers\omrondrv\*.dll .\eview-server\bin\drivers\omrondrv\ /y
xcopy ..\..\bin\drivers\opcdrv\*.dll .\eview-server\bin\drivers\opcdrv\ /y
xcopy ..\..\bin\drivers\snmpdrv\*.dll .\eview-server\bin\drivers\snmpdrv\ /y
xcopy ..\..\bin\drivers\pkpingdrv\*.dll .\eview-server\bin\drivers\pkpingdrv\ /y
xcopy ..\..\bin\drivers\mitubishifxdrv\*.dll .\eview-server\bin\drivers\mitubishifxdrv\ /y
xcopy ..\..\bin\drivers\simenss7drv\*.dll .\eview-server\bin\drivers\simenss7drv\ /y
echo python 版本的驱动示例
xcopy ..\..\source\drivers\samplepythondrv\*.py .\eview-server\bin\drivers\samplepythondrv\  /y

copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\abcipdrv\abcipdrv.exe /y
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\cdt\cdt.exe /y
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\dbdrv\dbdrv.exe /y
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\gwmqttdrv\gwmqttdrv.exe /y
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\iec104\iec104.exe /y
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\mnmqttdrv\mnmqttdrv.exe /y
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\modbusrtu\modbusrtu.exe /y
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\modbustcp\modbustcp.exe /y
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\omrondrv\omrondrv.exe /y
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\opcdrv\opcdrv.exe /y
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\snmpdrv\snmpdrv.exe /y
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\pkpingdrv\pkpingdrv.exe /y
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\mitubishifxdrv\mitubishifxdrv.exe /y
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\simenss7drv\simenss7drv.exe /y



