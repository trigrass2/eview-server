set CURDIR=%~dp0
cd %CURDIR%

mkdir ..\..\repo\eview-pkdata
mkdir ..\..\repo\eview-pkdata\include
mkdir ..\..\repo\eview-pkdata\bin
mkdir ..\..\repo\eview-pkdata\lib
mkdir ..\..\repo\eview-pkdata\doc

echo cp -f include file...
xcopy /y /S ..\..\include\pkdata ..\..\repo\eview-pkdata\include\pkdata\ 
xcopy /y /S ..\..\lib\pkdata.lib ..\..\repo\eview-pkdata\lib\ 
xcopy /y /S ..\..\bin\pkdata.dll ..\..\repo\eview-pkdata\bin\

echo now package eview-pkdata ......
set CURRENT_DATE=%date:~0,4%-%date:~5,2%-%date:~8,2%
set RAR_FILENAME=eview-pkdata-%CURRENT_DATE%.rar
del /S /Q .\eview-pkdata*.rar
..\rar a -ep1  ..\..\repo\%RAR_FILENAME%  ..\..\repo\eview-pkdata\bin ..\..\repo\eview-pkdata\doc  ..\..\repo\eview-pkdata\include  ..\..\repo\eview-pkdata\lib
copy ..\..\repo\%RAR_FILENAME% ..\..\repo\eview-pkdata.rar /y

echo end package it to .\eview-pkdata.rar

cd %CURDIR%
