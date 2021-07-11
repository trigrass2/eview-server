#!/bin/sh
CURDIR=$(cd `dirname $0`; pwd)
cd ${CURDIR}

echo prepare eveiw-server directory
rm -rf ../../repo/eview-server
mkdir ../../repo/eview-server

#python plugin. linux下必须用so扩展名而不是pyd，且需要去掉lib。不需要libpydriver.pyd、libpydriver.so,只需要pydriver.so
rm ../../bin/pydriver.so
mv  ../../bin/libpydriver.so ../../bin/pydriver.so

rm ../../bin/pydata.so
mv -f ../../bin/libpydata.so ../../bin/pydata.so

echo copy all bin with drivers
cp -r ../../bin ../../repo/eview-server/

echo 复制python环境文件
rmdir -rf ../../repo/eview-server/python
mkdir ../../repo/eview-server/python
tar -xzvf ../../thirdparty/python2.7.tar.gz -C ../../repo/eview-server/python/
rm -rf ../../repo/eview-server/python/include/

echo copy config directory
mkdir ../../repo/eview-server/config
mkdir -p ../../repo/eview-server/etc/init.d

cp -r ../../template/config/logconfig.xml ../../repo/eview-server/config/
cp -r ../../config/pkhisdb.conf ../../repo/eview-server/config/
cp -r ../../config/pkmemdb.conf ../../repo/eview-server/config/
cp -r ../../config/pkmqtt.conf ../../repo/eview-server/config/
cp -r ../../template/config/arm-linux/pkservermgr.xml ../../repo/eview-server/config/
chmod +x ../../template/etc/init.d/eview
cp -r ../../template/etc/init.d/eview ../../repo/eview-server/etc/init.d/
cp -r ../../template/etc/arm-linux/profile ../../repo/eview-server/etc/

echo copy start.sh...
cp -r ../../template/bin/linux/start_all.sh ../../repo/eview-server/bin/
cp -r ../../template/bin/linux/stop_all.sh ../../repo/eview-server/bin/
chmod +x ../../repo/eview-server/bin/*.sh
chmod +x ../../repo/eview-server/bin/*

echo 执行脚本和readme
chmod +x ../../repo/eview-server/*.sh

echo now package it ......

find ../../repo/eview-server -name ".svn"|xargs rm -rf

rm -f  ../../repo/eview-server/bin/libboost_locale* ../../repo/eview-server/bin/libboost_context* ../../repo/eview-server/bin/libboost_coroutine* ../../repo/eview-server/bin/libboost_graph* ../../repo/eview-server/bin/libboost_log* 
rm -f  ../../repo/eview-server/bin/libboost_math* ../../repo/eview-server/bin/libboost_prg* ../../repo/eview-server/bin/libboost_program_options* ../../repo/eview-server/bin/libboost_random* ../../repo/eview-server/bin/libboost_serialization*
rm -f   ../../repo/eview-server/bin/libboost_signals* ../../repo/eview-server/bin/libboost_timer* ../../repo/eview-server/bin/libboost_unit_test_framework* ../../repo/eview-server/bin/libboost_wave* ../../repo/eview-server/bin/libboost_wserialization* 
#rm -f   ../../repo/eview-server/bin/libboost_regex*

rm -f ../../repo/eview-server-2*.tar.gz
tar -cvzf ../../repo/eview-server-base.tar.gz -C ../../repo/eview-server/ . 
#zip -r ../../repo/eview-server-base.zip ./../repo/eview-server/
CURDATE=$(date +%Y%m%d)
cp  -f ../../repo/eview-server-base.tar.gz ../../repo/eview-server-base-${CURDATE}.tar.gz

cd ${CURDIR}

sh postbuild-driversdk.sh
sh postbuild-pkdata.sh

cd ${CURDIR}
