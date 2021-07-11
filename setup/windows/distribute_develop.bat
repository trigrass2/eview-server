@echo on 

echo copy include,lib,pdb
rmdir /q /s .\eview-develop
mkdir .\eview-develop
rem copy drivers
xcopy ..\..\include .\eview-develop\include\ /S /Y 
xcopy ..\..\lib\*.lib .\eview-develop\lib\ /y
xcopy ..\..\bin\*.pdb .\eview-develop\pdb\ /y

set CURRENT_DATE=%date:~0,4%-%date:~5,2%-%date:~8,2%
set RAR_FILENAME=eview-win-%CURRENT_DATE%-develop.rar
del %RAR_FILENAME%
rar a ./%RAR_FILENAME%  ./eview-develop/pdb ./eview-develop/include ./eview-develop/lib
