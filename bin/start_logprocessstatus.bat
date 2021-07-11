set CURDIR=%~dp0
cd /D %CURDIR%
set PKHOME_DIR=%CURDIR%..
set PYTHONHOME=%PKHOME_DIR%\python
set PYTHONPATH=%PKHOME_DIR%/python/Lib/site-packages

%PYTHONHOME%\python.exe logProcessStatus.py