@echo off
cd %~dp0
tasklist |find "pkmqtt.exe" && (taskkill /F /IM pkmqtt.exe ) 

pkmqtt -c  ../config/pkmqtt.conf