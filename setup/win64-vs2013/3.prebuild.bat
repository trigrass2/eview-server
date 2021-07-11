set CURDIR=%~dp0
cd %CURDIR%

echo extract depends component......
del /S /Q ..\..\bin
del /S /Q ..\..\lib
cd ..\..\

setup\rar.exe x -y thirdparty\cmake.rar
setup\rar.exe x -y thirdparty\mfc120.rar
setup\rar.exe x -y thirdparty\vcruntime120.rar
setup\rar.exe x -y thirdparty\vcruntime100.rar
setup\rar.exe x -inul -y thirdparty\boost1.66.rar 
setup\rar.exe x -inul -y thirdparty\pkce.rar
setup\rar.exe x -y thirdparty\cppredis.rar
setup\rar.exe x -y thirdparty\cryptlib.rar
setup\rar.exe x -y thirdparty\exceptionreport.rar
setup\rar.exe x -y thirdparty\jsoncpp.rar
setup\rar.exe x -y thirdparty\libdes.rar
setup\rar.exe x -y thirdparty\libmodbus.rar

setup\rar.exe x -y thirdparty\pkcomm.rar
setup\rar.exe x -y thirdparty\pkcrashdump.rar
setup\rar.exe x -y thirdparty\pkdbapi.rar
setup\rar.exe x -y thirdparty\pkinifile.rar
setup\rar.exe x -y thirdparty\pklog.rar
setup\rar.exe x -y thirdparty\pklic.rar
setup\rar.exe x -y thirdparty\pkmemdbapi.rar
setup\rar.exe x -y thirdparty\pkmqtt.rar
setup\rar.exe x -y thirdparty\pknetque.rar
setup\rar.exe x -y thirdparty\pkserverbase.rar
setup\rar.exe x -y thirdparty\redisproxy.rar
setup\rar.exe x -y thirdparty\shmqueue.rar
setup\rar.exe x -y thirdparty\snmp_pp.rar
setup\rar.exe x -y thirdparty\sqlite3.rar
setup\rar.exe x -y thirdparty\tinyxml2.rar
setup\rar.exe x -inul -y thirdparty\python2.7.rar
setup\rar.exe x -y thirdparty\pksock.rar
setup\rar.exe x -y thirdparty\libmysql.rar
setup\rar.exe x -y thirdparty\pkhisdb.rar
setup\rar.exe x -y thirdparty\pkmemdb.rar
setup\rar.exe x -y thirdparty\mongoclient.rar
setup\rar.exe x -y thirdparty\sqlapi.rar
setup\rar.exe x -y thirdparty\pkservermgr.rar
setup\rar.exe x -y thirdparty\pkcommunicate.rar
setup\rar.exe x -y thirdparty\pkcommforward.rar
setup\rar.exe x -y thirdparty\pkprocesscontroller.rar

cd %CURDIR%
