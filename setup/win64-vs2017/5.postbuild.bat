set CURDIR=%~dp0
cd %CURDIR%

echo on

rd /q /s ..\..\repo\

rem mkdir directories
mkdir ..\..\repo\eview-server
mkdir ..\..\repo\eview-server\bin
mkdir ..\..\repo\eview-server\lib
mkdir ..\..\repo\eview-server\config
mkdir ..\..\repo\eview-server\python

echo copy config dir
xcopy ..\..\config\pkmemdb.conf ..\..\repo\eview-server\config\ /y
xcopy ..\..\config\pkhisdb.conf ..\..\repo\eview-server\config\ /y
xcopy ..\..\config\pkmqtt.conf ..\..\repo\eview-server\config\ /y
xcopy ..\..\template\config\logconfig.xml ..\..\repo\eview-server\config\ /y
rem xcopy ..\..\template\config\windows\pkservermgr.xml ..\..\repo\eview-server\config\ /y

echo copy lib dir
xcopy ..\..\lib\pkdrvcmn.lib ..\..\repo\eview-server\lib\ /y

echo copy bin dir
xcopy ..\..\bin\vcruntime140.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\msvcp140.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\concrt140.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\dbghelp.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkdata.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkce.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkcomm.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pklog.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkinifile.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkdbapi.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkcrashdump.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\libmySQL.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\mongoclient.dll ..\..\repo\eview-server\bin\ /y

xcopy ..\..\bin\libmodbus.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkdrvcmn.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\sqlite3.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkmemdbapi.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pklinkact*.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkmodbusdata.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkmodbusdata_sim.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkserverbase.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pknetque.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkcommunicate.dll ..\..\repo\eview-server\bin\ /y

xcopy ..\..\bin\*pkmqtt*.* ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\libcrypto-1_1.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\libssl-1_1.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\mosquitto.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\mosquittopp.dll ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pthread*.dll ..\..\repo\eview-server\bin\ /y

copy ..\..\bin\pydriver.dll ..\..\bin\pydriver.pyd /y
xcopy ..\..\bin\pydriver.pyd ..\..\repo\eview-server\bin\ /y
copy ..\..\bin\pydata.dll ..\..\bin\pydata.pyd /y
xcopy ..\..\bin\pydata.pyd ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkservermgr_plugin\*.dll ..\..\repo\eview-server\bin\pkservermgr_plugin\ /y

xcopy ..\..\bin\pkdriver.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pknodeserver.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkuppernodeserver.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pksync2uppernodeserver.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkmqttforwardserver.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkhisdataserver.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkmemdb.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkhisdb.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkhostservice.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pklinkserver.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkmodserver.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkdbtransferserver.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkcommforwardserver.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkservermgr.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkmqtt.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\pkhostservice.exe ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\start_pkmemdb.bat ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\start_pkhisdb.bat ..\..\repo\eview-server\bin\ /y
xcopy ..\..\bin\start_pkmqtt.bat ..\..\repo\eview-server\bin\ /y

rem xcopy ..\..\template\bin\windows\mod_RSsim*.rar ..\..\repo\eview-server\bin\ /y
rem xcopy ..\..\template\bin\windows\modscan_modbus.rar ..\..\repo\eview-server\bin\ /y
rem xcopy ..\..\template\bin\windows\TCPTestTool.rar ..\..\repo\eview-server\bin\ /y
rem xcopy ..\..\template\bin\windows\*ComMonitor.rar ..\..\repo\eview-server\bin\ /y

xcopy ..\..\python ..\..\repo\eview-server\python\ /y/S

set CURRENT_DATE=%date:~0,4%-%date:~5,2%-%date:~8,2%
set RAR_FILENAME=eview-server-base-%CURRENT_DATE%.rar
..\rar a -ep1 ../../repo/%RAR_FILENAME%  ../../repo/eview-server/bin ../../repo/eview-server/lib ../../repo/eview-server/config ../../repo/eview-server/python
copy ..\..\repo\%RAR_FILENAME% ..\..\repo\eview-server-base.rar /y

call .\postbuild_driversdk.bat
call .\postbuild_tools.bat
call .\postbuild_pkdata.bat

cd %CURDIR%