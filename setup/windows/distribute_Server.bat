@echo on

rem 删除eview目录
rmdir /q /s eview-server

echo 复制python环境文件
rem rmdir /q /s .\eview-server\python
rem .\rar.exe x -y ..\..\thirdparty\windows\python27-win32-distribute.rar .\eview-server\


rem 新建缺省目录
mkdir eview-server
mkdir eview-server\bin
mkdir eview-server\config
mkdir eview-server\python

echo copy config dir
xcopy ..\..\template\config\db.conf .\eview-server\config\ /y
xcopy ..\..\template\config\eview.db .\eview-server\config\ /y
xcopy ..\..\template\config\logconfig.xml .\eview-server\config\ /y
xcopy ..\..\template\config\pkhisdb.conf .\eview-server\config\ /y
xcopy ..\..\template\config\pkmemdb.conf .\eview-server\config\ /y
xcopy ..\..\template\config\pkservermgr.xml .\eview-server\config\ /y
xcopy ..\..\config\pkmqtt.conf .\eview-server\config\ /y

echo copy bin dir
xcopy ..\..\bin\mfc120.dll .\eview-server\bin\ /y
xcopy ..\..\bin\msvcp120.dll .\eview-server\bin\ /y
xcopy ..\..\bin\msvcr120.dll .\eview-server\bin\ /y
xcopy ..\..\bin\dbghelp.dll .\eview-server\bin\ /y
xcopy ..\..\bin\pkdata.dll .\eview-server\bin\ /y
xcopy ..\..\bin\pkce.dll .\eview-server\bin\ /y
xcopy ..\..\bin\pkcomm.dll .\eview-server\bin\ /y
xcopy ..\..\bin\pklog.dll .\eview-server\bin\ /y
xcopy ..\..\bin\libmySQL.dll .\eview-server\bin\ /y
xcopy ..\..\bin\python27.dll .\eview-server\bin\ /y
xcopy ..\..\bin\mongoclient.dll .\eview-server\bin\ /y

xcopy ..\..\bin\libmodbus.dll .\eview-server\bin\ /y
xcopy ..\..\bin\pkdbapi.dll .\eview-server\bin\ /y
xcopy ..\..\bin\pkdrvcmn.dll .\eview-server\bin\ /y
xcopy ..\..\bin\sqlite3.dll .\eview-server\bin\ /y
xcopy ..\..\bin\pkmemdbapi.dll .\eview-server\bin\ /y
xcopy ..\..\bin\pklinkact*.dll .\eview-server\bin\ /y
xcopy ..\..\bin\pkmodbusdata.dll .\eview-server\bin\ /y
xcopy ..\..\bin\pkmodbusdata_sim.dll .\eview-server\bin\ /y
xcopy ..\..\bin\pkserverbase.dll .\eview-server\bin\ /y
xcopy ..\..\bin\pknetque.dll .\eview-server\bin\ /y

xcopy ..\..\bin\*pkmqtt*.* .\eview-server\bin\ /y
xcopy ..\..\bin\libcrypto-1_1.dll .\eview-server\bin\ /y
xcopy ..\..\bin\libssl-1_1.dll .\eview-server\bin\ /y
xcopy ..\..\bin\mosquitto.dll .\eview-server\bin\ /y
xcopy ..\..\bin\mosquittopp.dll .\eview-server\bin\ /y
xcopy ..\..\bin\pthread*.dll .\eview-server\bin\ /y

copy ..\..\bin\pydriver.dll .\eview-server\bin\pydriver.pyd /y
xcopy ..\..\bin\pydriver.pyd .\eview-server\bin\ /y

xcopy ..\..\bin\pkdriver.exe .\eview-server\bin\ /y
xcopy ..\..\bin\pknodeserver.exe .\eview-server\bin\ /y
xcopy ..\..\bin\pkhisdataserver.exe .\eview-server\bin\ /y
xcopy ..\..\bin\pkhisdb.exe .\eview-server\bin\ /y
xcopy ..\..\bin\pkhostservice.exe .\eview-server\bin\ /y
xcopy ..\..\bin\pklinkserver.exe .\eview-server\bin\ /y
xcopy ..\..\bin\pkmemdb.exe .\eview-server\bin\ /y
xcopy ..\..\bin\pkmodserver.exe .\eview-server\bin\ /y
xcopy ..\..\bin\pkservermgr.exe .\eview-server\bin\ /y
xcopy ..\..\bin\pkTagMonitor.exe .\eview-server\bin\ /y
xcopy ..\ ..\thirdparty\windows\start_*.bat .\eview-server\bin\ /y
xcopy ..\ ..\thirdparty\windows\mod_RSsim*.exe .\eview-server\bin\tools\ /y
xcopy ..\ ..\thirdparty\windows\TCPTestTool.exe .\eview-server\bin\tools\ /y
xcopy ..\ ..\thirdparty\windows\*VSPD*.exe .\eview-server\bin\tools\ /y

xcopy ..\..\doc\eview快速使用指南.pdf .\eview-server\doc\ /y
xcopy ..\..\doc\用户手册\控件使用手册.doc .\eview-server\doc\ /y
xcopy ..\..\doc\用户手册\权限管理用户手册.docx .\eview-server\doc\ /y
xcopy ..\..\doc\用户手册\设备管理使用手册.docx .\eview-server\doc\ /y
xcopy ..\..\doc\用户手册\视频监控系统用户手册.docx .\eview-server\doc\ /y
xcopy ..\..\doc\用户手册\联动系统用户手册.docx .\eview-server\doc\ /y
xcopy ..\..\doc\用户手册\脚本接口用户手册.docx .\eview-server\doc\ /y
xcopy ..\..\doc\用户手册\报警系统使用手册.doc .\eview-server\doc\ /y

xcopy ..\..\python .\eview-server\python\ /y/S

echo  call .\distribute_Driver.bat
call .\distribute_Driver.bat

set CURRENT_DATE=%date:~0,4%-%date:~5,2%-%date:~8,2%
set RAR_FILENAME=eview-win-%CURRENT_DATE%-server.rar
del %RAR_FILENAME%
rar a ./%RAR_FILENAME%  ./eview-server/bin ./eview-server/config ./eview-server/python ./eview-server/doc 
echo 压缩文件成功,%RAR_FILENAME%
