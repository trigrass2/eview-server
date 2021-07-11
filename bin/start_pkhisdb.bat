@echo off
cd %~dp0
echo  pkhisdb started

tasklist |find "pkhisdb.exe" && taskkill /F /IM pkhisdb.exe 
if not exist ..\hisdb (mkdir ..\hisdb)
if not exist ..\log (mkdir ..\log)
if exist ..\hisdb\mongod.lock (del ..\hisdb\mongod.lock)
if exist ..\hisdb\mongod.lock (ping -n 2 localhost 1>nul 2>nul && del ..\hisdb\mongod.lock)

pkhisdb -f ..\config\pkhisdb.conf --journal --storageEngine=mmapv1
@echo on