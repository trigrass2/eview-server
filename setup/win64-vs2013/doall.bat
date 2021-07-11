set CURDIR=%~dp0
cd %CURDIR%

call 1.update.bat
call 2.download.bat
call 3.prebuild.bat
call 4.build.bat
call 5.postbuild.bat
rem please call 6.upload.bat
pause
