#####从ftp服务器上的/release/cccomps/arm-linux 到 本地的thirdparty 
#!/bin/bash
CURDIR=$(cd `dirname $0`; pwd)
cd ${CURDIR}

mkdir ../../thirdparty

ftp -v -n ftp.peakinfo.cn 21 <<EOF
user peak peak
binary
lcd ../../thirdparty
cd /release/cccomps/centos7.0
prompt
mget pkmqtt.tar.gz libdes.tar.gz python2.7.tar.gz pkmemdb.tar.gz pkce.tar.gz cppredis.tar.gz cryptlib.tar.gz jsoncpp.tar.gz pkcomm.tar.gz pkdbapi.tar.gz pklic.tar.gz pklog.tar.gz pkmemdbapi.tar.gz pknetque.tar.gz pkserverbase.tar.gz pksock.tar.gz redisproxy.tar.gz shmqueue.tar.gz snmp_pp.tar.gz sqlapi.tar.gz sqlite3.tar.gz tinyxml2.tar.gz boost1.66.tar.gz pkmqtt.tar.gz pkinifile.tar.gz pkcrashdump.tar.gz pkservermgr.tar.gz pkcommunicate.tar.gz pkcommforward.tar.gz pkprocesscontroller.tar.gz
close
bye
!
cd ${CURDIR}
