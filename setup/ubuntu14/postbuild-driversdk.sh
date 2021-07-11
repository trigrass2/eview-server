CURDIR=$(cd `dirname $0`; pwd)
cd ${CURDIR}

rm -rf ../../repo/eview-server-driversdk
mkdir -p ../../repo/eview-server-driversdk

mkdir ../../repo/eview-server-driversdk/include
mkdir ../../repo/eview-server-driversdk/bin
mkdir ../../repo/eview-server-driversdk/source
mkdir ../../repo/eview-server-driversdk/doc

echo cp -f include file...
cp -rf ../../include/pkdriver ../../repo/eview-server-driversdk/include/pkdriver/ 
cp -rf ../../include/eviewcomm ../../repo/eview-server-driversdk/include/eviewcomm/ 
cp -rf ../../include/pkcomm ../../repo/eview-server-driversdk/include/pkcomm/ 

cp -f ../../bin/pkdriver* ../../repo/eview-server-driversdk/bin/
cp -f ../../bin/libpkdrvcmn*.* ../../repo/eview-server-driversdk/bin/
cp -f ../../bin/libpkcomm* ../../repo/eview-server-driversdk/bin/

echo cp CMakeLists.txt
cp -rf ../../source/CMakeCommon ../../repo/eview-server-driversdk/source/
cp -rf ../../source/CMakeCommonLib ../../repo/eview-server-driversdk/source/
cp -rf ../../source/AddVersionResource ../../repo/eview-server-driversdk/source/
cp -rf ../../source/versionrc.template ../../repo/eview-server-driversdk/source/

echo cp CMakeLists.txt
mkdir -p ../../repo/eview-server-driversdk/source/drivers/
cp -rf ../../source/CMakeLists.txt ../../repo/eview-server-driversdk/source/CMakeLists-template.txt
cp -rf ../../source/drivers/CMakeLists.txt ../../repo/eview-server-driversdk/source/drivers/CMakeLists-template.txt

#cp -r ./驱动开发说明.txt ../../repo/eview-server-driversdk/doc/
find ../../repo/eview-server-driversdk -name ".svn"|xargs rm -rf
echo now package eview-server-driversdk ......
rm -f ../../repo/eview-server-driversdk.tar.gz
tar -cvzf ../../repo/eview-server-driversdk.tar.gz -C ../../repo/eview-server-driversdk .
CURDATE=$(date +%Y%m%d)
cp  -f ../../repo/eview-server-driversdk.tar.gz ../../repo/eview-server-driversdk-${CURDATE}.tar.gz
echo end package it to ../../repo/eview-server-driversdk.tar.gz

cd ${CURDIR}
