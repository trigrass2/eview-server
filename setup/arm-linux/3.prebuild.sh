#!/bin/sh
CURDIR=$(cd `dirname $0`; pwd)
cd ${CURDIR}

echo extract depends component......
rm -rf ../../bin/
rm -rf ../../lib/
mkdir ../../bin ../../lib
#extract
mkdir ../../python
tar -xvzf ../../thirdparty/python2.7.tar.gz -C ../../python
rm -rf ../../include/python
mv ../../python/include/python ../../include/python
rm -rf ../../python/include
cp ../../python/bin/lib* ../../bin/
cp ../../python/lib/lib* ../../bin/
tar -xvzf ../../thirdparty/pkmemdb.tar.gz -C ../../
tar -xzf ../../thirdparty/pkce.tar.gz -C ../../
tar -xvzf ../../thirdparty/cppredis.tar.gz -C ../../
tar -xvzf ../../thirdparty/cryptlib.tar.gz -C ../../
tar -xvzf ../../thirdparty/jsoncpp.tar.gz -C ../../
tar -xvzf ../../thirdparty/libdes.tar.gz -C ../../
tar -xvzf ../../thirdparty/libmodbus.tar.gz -C ../../
tar -xvzf ../../thirdparty/pkcomm.tar.gz -C ../../
tar -xvzf ../../thirdparty/pkdbapi.tar.gz -C ../../
tar -xvzf ../../thirdparty/pkinifile.tar.gz -C ../../
tar -xvzf ../../thirdparty/pklic.tar.gz -C ../../
tar -xvzf ../../thirdparty/pklog.tar.gz -C ../../
tar -xvzf ../../thirdparty/pkmemdbapi.tar.gz -C ../../
tar -xvzf ../../thirdparty/pknetque.tar.gz -C ../../
tar -xvzf ../../thirdparty/pkserverbase.tar.gz -C ../../
tar -xvzf ../../thirdparty/pksock.tar.gz -C ../../
tar -xvzf ../../thirdparty/redisproxy.tar.gz -C ../../
tar -xvzf ../../thirdparty/shmqueue.tar.gz -C ../../
tar -xvzf ../../thirdparty/snmp_pp.tar.gz -C ../../
tar -xvzf ../../thirdparty/sqlapi.tar.gz -C ../../
tar -xvzf ../../thirdparty/sqlite3.tar.gz -C ../../
tar -xvzf ../../thirdparty/tinyxml2.tar.gz -C ../../
tar -xzf ../../thirdparty/boost1.55.tar.gz -C ../../
tar -xvzf ../../thirdparty/pkmqtt.tar.gz -C ../../
tar -xvzf ../../thirdparty/libdes.tar.gz -C ../../
tar -xvzf ../../thirdparty/pkinifile.tar.gz -C ../../
tar -xvzf ../../thirdparty/pkcrashdump.tar.gz -C ../../
tar -xvzf ../../thirdparty/pkmemdb.tar.gz -C ../../
#tar -xvzf ../../thirdparty/mongoclient.tar.gz -C ../../
tar -xvzf ../../thirdparty/pkservermgr.tar.gz -C ../../
tar -xvzf ../../thirdparty/pkcommunicate.tar.gz -C ../../
tar -xvzf ../../thirdparty/pkcommforward.tar.gz -C ../../
tar -xvzf ../../thirdparty/pkprocesscontroller.tar.gz -C ../../

cd ${CURDIR}
