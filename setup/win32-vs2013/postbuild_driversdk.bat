set CURDIR=%~dp0
cd %CURDIR%

mkdir ..\..\repo\eview-server-driversdk
mkdir ..\..\repo\eview-server-driversdk\include
mkdir ..\..\repo\eview-server-driversdk\bin
mkdir ..\..\repo\eview-server-driversdk\source
mkdir ..\..\repo\eview-server-driversdk\doc

echo cp -f include file...
xcopy /y /S ..\..\include\pkdriver ..\..\repo\eview-server-driversdk\include\pkdriver\ 
xcopy /y /S ..\..\include\eviewcomm ..\..\repo\eview-server-driversdk\include\eviewcomm\ 
xcopy /y /S ..\..\include\pkcomm ..\..\repo\eview-server-driversdk\include\pkcomm\ 

xcopy /y /S ..\..\lib\pkcomm.lib ..\..\repo\eview-server-driversdk\lib\ 
xcopy /y /S ..\..\lib\pkdrvcmn.lib ..\..\repo\eview-server-driversdk\lib\ 

xcopy /y /S ..\..\bin\pkdrvcmn.dll ..\..\repo\eview-server-driversdk\bin\
xcopy /y /S ..\..\bin\pkdriver.exe ..\..\repo\eview-server-driversdk\bin\

echo cp CMakeLists.txt
xcopy /y /S ..\..\source\CMakeCommon ..\..\repo\eview-server-driversdk\source\
xcopy /y /S ..\..\source\CMakeCommonLib ..\..\repo\eview-server-driversdk\source\
xcopy /y /S ..\..\source\AddVersionResource ..\..\repo\eview-server-driversdk\source\
xcopy /y /S ..\..\source\versionrc.template ..\..\repo\eview-server-driversdk\source\

echo copy C++ sample driver
mkdir ..\..\repo\eview-server-driversdk\bin\drivers\sampledrv

xcopy /y /S ..\..\source\drivers\sampledrv\* ..\..\repo\eview-server-driversdk\source\drivers\sampledrv\
xcopy ..\..\bin\drivers\sampledrv\*.dll ..\..\repo\eview-server-driversdk\bin\drivers\sampledrv\ /y /S
copy ..\..\bin\pkdriver.exe ..\..\repo\eview-server-driversdk\bin\drivers\sampledrv\sampledrv.exe /y 

echo copy Python sample driver
xcopy  /y /S ..\..\source\drivers\samplepythondrv\* ..\..\repo\eview-server-driversdk\source\drivers\samplepythondrv\
xcopy /y /S ..\..\source\drivers\samplepythondrv\* ..\..\repo\eview-server-driversdk\bin\drivers\samplepythondrv\
copy ..\..\bin\pkdriver.exe ..\..\repo\eview-server-driversdk\bin\drivers\samplepythondrv\samplepythondrv.exe /y 

echo cp CMakeLists.txt
mkdir ..\..\repo\eview-server-driversdk\setup\win32-vs2013\
copy /y  .\4.build.bat ..\..\repo\eview-server-driversdk\setup\win32-vs2013\build-driver.bat
copy /y ..\..\template\source\CMakeLists.txt ..\..\repo\eview-server-driversdk\source\
copy /y ..\..\template\source\drivers\CMakeLists.txt ..\..\repo\eview-server-driversdk\source\drivers\CMakeLists-template.txt

copy /y ..\..\doc\二次开发手册\eview驱动二次开发手册*.* ..\..\repo\eview-server-driversdk\doc\

echo now package eview-driversdk ......
set CURRENT_DATE=%date:~0,4%-%date:~5,2%-%date:~8,2%
set RAR_FILENAME=eview-server-driversdk-%CURRENT_DATE%.rar
del /S /Q .\eview-server*.rar
..\rar a -ep1  ..\..\repo\%RAR_FILENAME%  ..\..\repo\eview-server-driversdk\bin ..\..\repo\eview-server-driversdk\doc  ..\..\repo\eview-server-driversdk\include  ..\..\repo\eview-server-driversdk\source  ..\..\repo\eview-server-driversdk\setup  ..\..\repo\eview-server-driversdk\lib 
copy ..\..\repo\%RAR_FILENAME% ..\..\repo\eview-server-driversdk.rar /y

echo end package it to .\eview-server-driversdk.rar

cd %CURDIR%
