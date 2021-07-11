set CURDIR=%~dp0
cd %CURDIR%

mkdir ..\..\cmake-temp
cd ..\..\cmake-temp
"%CURDIR%\..\cmake\bin\cmake" -G "Visual Studio 15 2017 Win64" ../source

cd %CURDIR%



