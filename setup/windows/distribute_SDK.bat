REM set SRCPATH=%~dp0\..\
REM set DESTPATH=%~dp0\eview-server

rmdir /q /s eview-server\include
rmdir /q /s eview-server\lib
rmdir /q /s eview-server\source

mkdir .\eview-server
mkdir .\eview-server\include
mkdir .\eview-server\lib
mkdir .\eview-server\source

echo copy include file...
xcopy ..\..\include\ace .\eview-server\include\ace\  /Y /E
xcopy ..\..\include\json .\eview-server\include\json\ /Y /E
xcopy ..\..\include\tinyxml2 .\eview-server\include\tinyxml2\ /Y /E
xcopy ..\..\include\pkcomm .\eview-server\include\pkcomm\ /Y /E
xcopy ..\..\include\pklog .\eview-server\include\pklog\ /Y /E
xcopy ..\..\include\pkdriver .\eview-server\include\pkdriver\ /Y /E
xcopy ..\..\include\pkdata .\eview-server\include\pkdata\ /Y /E
xcopy ..\..\include\pkdbapi .\eview-server\include\pkdbapi\ /Y /E
xcopy ..\..\include\pkeviewdbapi .\eview-server\include\pkeviewdbapi\ /Y /E
xcopy ..\..\include\pkmemdbapi .\eview-server\include\pkmemdbapi\ /Y /E
xcopy ..\..\include\pkserver .\eview-server\include\pkserver\ /Y /E
xcopy ..\..\include\pksock .\eview-server\include\pksock\ /Y /E

echo copy lib file...
xcopy ..\..\lib\pkdrvcmn.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\pkdata.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\pkcomm.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\pklog.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\pkdbapi.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\pkeviewdbapi.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\exceptionreport.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\pkce.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\pklb.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\tinyxml2.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\shmqueue.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\pkmemdbapi.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\jsoncpp.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\pkce.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\pklic.lib .\eview-server\lib\ /Y

xcopy ..\..\lib\pksockserver.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\pksockclient.lib .\eview-server\lib\ /Y
xcopy ..\..\lib\pkserverbase.lib .\eview-server\lib\ /Y

echo copy PKSampleServer file...
xcopy ..\..\source\sample\PKSampleServer .\eview-server\source\sample\PKSampleServer\ /Y /E
xcopy ..\..\source\sample\PKSock_Client .\eview-server\source\sample\PKSock_Client\ /Y /E
xcopy ..\..\source\sample\PKSock_ClientServer .\eview-server\source\sample\PKSock_ClientServer\ /Y /E
xcopy ..\..\source\sample\pkdatatest .\eview-server\source\sample\pkdatatest\ /Y /E
del .\eview-server\source\*.sdf
del .\eview-server\source\*.sdf

echo cp CMakeLists.txt
xcopy ..\..\source\CMakeCommon* .\eview-server\source\ /Y 
xcopy ..\..\source\AddVersionResource .\eview-server\source\ /Y 
xcopy ..\..\source\versionrc.template .\eview-server\source\ /Y

echo copy sample driver
mkdir .\eview-server\source\drivers
xcopy ..\..\source\drivers\sampledriver .\eview-server\source\drivers\sampledriver\ /Y /E
copy ..\..\bin\pkdriver.exe .\eview-server\bin\drivers\sampledriver\sampledriver.exe /y
xcopy ..\..\bin\drivers\sampledriver\*.dll .\eview-server\bin\drivers\sampledriver\ /y

echo copy solution generaotor-CMAKE
mkdir .\eview-server\buildtools\
mkdir .\eview-server\buildtools\windows
xcopy ..\..\buildtools\windows\cmake .\eview-server\buildtools\windows\cmake\  /Y /E
xcopy ..\..\buildtools\windows\*-console.bat .\eview-server\buildtools\windows\  /Y /E

set CURRENT_DATE=%date:~0,4%-%date:~5,2%-%date:~8,2%
set RAR_FILENAME=eview-win-%CURRENT_DATE%-sdk.rar
del %RAR_FILENAME%
rar a ./%RAR_FILENAME%  ./eview-server/include  ./eview-server/source ./eview-server/lib 
