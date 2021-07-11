
mkdir ..\..\repo\eview-tools
mkdir ..\..\repo\eview-tools\bin
mkdir ..\..\repo\eview-tools\doc
mkdir ..\..\repo\eview-tools\config

echo cp -f include file...
xcopy /y /S ..\..\bin\pkservermgr.exe ..\..\repo\eview-tools\bin\
xcopy /y /S ..\..\bin\pkdbtoeviewserver.exe ..\..\repo\eview-tools\bin\ 
xcopy /y /S ..\..\bin\pkce.dll ..\..\repo\eview-tools\bin\
xcopy /y /S ..\..\bin\pkcomm.dll ..\..\repo\eview-tools\bin\
xcopy /y /S ..\..\bin\pkdbapi.dll ..\..\repo\eview-tools\bin\
xcopy /y /S ..\..\bin\pkcrashdump.dll ..\..\repo\eview-tools\bin\
xcopy /y /S ..\..\bin\pkinifile.dll ..\..\repo\eview-tools\bin\
xcopy /y /S ..\..\bin\pkdata.dll ..\..\repo\eview-tools\bin\
xcopy /y /S ..\..\bin\pkmemdbapi.dll ..\..\repo\eview-tools\bin\
xcopy /y /S ..\..\bin\pklog.dll ..\..\repo\eview-tools\bin\
xcopy /y /S ..\..\bin\pkserverbase.dll ..\..\repo\eview-tools\bin\

xcopy /y /S ..\..\template\config\pkdbtoeviewserver.conf ..\..\repo\eview-tools\config\ 
xcopy /y /S ..\..\doc\第三方关系数据库*.docx ..\..\repo\eview-tools\doc\ 

echo now package eview-tools ......
set CURRENT_DATE=%date:~0,4%-%date:~5,2%-%date:~8,2%
set RAR_FILENAME=eview-tools-%CURRENT_DATE%.rar
del /S /Q .\eview-server*.rar
..\rar a -ep1  ..\..\repo\%RAR_FILENAME%  ..\..\repo\eview-tools\bin ..\..\repo\eview-tools\doc  ..\..\repo\eview-tools\config 
copy ..\..\repo\%RAR_FILENAME% ..\..\repo\eview-tools.rar /y

 echo end package it to .\eview-tools.rar

