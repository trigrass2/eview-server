rem set SRCPATH=%~dp0\
rem set DESTPATH=%~dp0\dist
set SETUPPATH=%~dp0
set BINPATH=%~dp0\..\..\bin\
set LIBPATH=%~dp0\..\..\lib
set WEBPATH=%~dp0\..\..\web

cd %BINPATH%
rem del *.bsc,*.ilk,*.pdb,*.exp,*.reg,*.dmp,*.txt,*.xml /s
del *.bsc,*.ilk,*.exp,*.reg,*.dmp,*.txt,*.xml /s

cd %LIBPATH%
rem del *.bsc,*.ilk,*.pdb,*.exp,*.reg,*.dmp,*.txt,*.xml /s
del *.bsc,*.ilk,*.exp,*.reg,*.dmp,*.txt,*.xml /s
rmdir /q /s icv,drivers 

cd %WEBPATH%
rem rmdir /q /s tomcat\temp
rem rmdir /q /s  tomcat\logs
rem rmdir /q /s  tomcat\work
rem mkdir tomcat\temp
rem mkdir tomcat\logs
rem mkdir tomcat\work
rem rmdir /q /s  tomcat\webapps\ROOT\page\upimg

rem del tomcat\webapps\*.zip
rem del tomcat\webapps\ROOT\page\*.zip,tomcat\webapps\ROOT\page\*.rar,tomcat\webapps\*.rar

cd %SETUPPATH%
