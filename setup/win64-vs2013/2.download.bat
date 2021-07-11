set CURDIR=%~dp0
cd %CURDIR%
mkdir ..\..\thirdparty

echo open ftp.peakinfo.cn > _download.ftp
echo user peak peak >> _download.ftp
echo binary >> _download.ftp
echo literal pasv >> _download.ftp
echo prompt off >> _download.ftp
echo lcd ../../thirdparty >> _download.ftp
echo cd /release/cccomps/win64-vs2013 >> _download.ftp
echo mget cmake.rar mfc120.rar vcruntime120.rar vcruntime100.rar boost1.66.rar cppredis.rar cryptlib.rar exceptionreport.rar jsoncpp.rar libdes.rar libmodbus.rar pkce.rar pkcomm.rar pkcrashdump.rar sqlapi.rar  >> _download.ftp
echo mget pkdbapi.rar pkinifile.rar pklog.rar pklic.rar pkmemdbapi.rar pkmqtt.rar pknetque.rar pkserverbase.rar redisproxy.rar shmqueue.rar snmp_pp.rar sqlite3.rar >> _download.ftp
echo mget tinyxml2.rar python2.7.rar mongoclient.rar pksock.rar libmysql.rar pkhisdb.rar pkmemdb.rar pkservermgr.rar pkcommunicate.rar pkcommforward.rar pkprocesscontroller.rar >> _download.ftp

echo close >> _download.ftp
echo bye >> _download.ftp

ftp -n -s:_download.ftp
