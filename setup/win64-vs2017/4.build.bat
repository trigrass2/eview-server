set CURDIR=%~dp0
cd %CURDIR%

call create_solution.bat
echo create_solution over

echo please SET VS2017_HOME=D:\software\Visual Studio\Visual Studio 2017 Enterprise
set path=%VS2017_HOME%\VC\Auxiliary\Build;%path%
call vcvars64.bat

echo ready to build solution...
call devenv ..\..\cmake-temp\eview-server.sln /build "RelWithDebInfo|x64" /useenv


