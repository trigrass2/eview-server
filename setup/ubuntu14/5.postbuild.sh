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
cp -r ../../template/config/linux/pkservermgr.xml ../../repo/eview-server/config/
chmod +x ../../template/etc/init.d/eview
cp -r ../../template/etc/init.d/eview ../../repo/eview-server/etc/init.d/
cp -r ../../repo/eview-server/config/* ../../config/

echo copy start.sh...
cp -r ../../template/bin/linux/start_all.sh ../../repo/eview-server/bin/
cp -r ../../template/bin/linux/stop_all.sh ../../repo/eview-server/bin/
chmod +x ../../repo/eview-server/bin/*.sh
chmod +x ../../repo/eview-server/bin/*
cp -r ../../repo/eview-server/bin/*.sh ../../bin/

echo 执行脚本和readme
chmod +x ../../repo/eview-server/*.sh

echo now package it ......
CURDATE=$(date +%Y%m%d)

find ../../repo/eview-server -name ".svn"|xargs rm -rf
#rm -rf ../../repo/eview-server/bin/libboost*
rm -f ../../repo/eview-server-2*.tar.gz
tar -cvzf ../../repo/eview-server-base.tar.gz -C ../../repo/eview-server/ . 
cp -f ../../repo/eview-server-base.tar.gz ../../repo/eview-server-base-${CURDATE}.tar.gz 

cd ${CURDIR}

sh postbuild-driversdk.sh
sh postbuild-pkdata.sh

cd ${CURDIR}
