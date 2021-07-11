@echo off
cd %~dp0
tasklist |find "pkmemdb.exe" && (taskkill /F /IM pkmemdb.exe ) 
if not exist ..\log (mkdir ..\log)
@echo on
pkmemdb.exe ../config/pkmemdb.conf
