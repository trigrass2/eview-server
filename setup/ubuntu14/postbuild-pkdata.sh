CURDIR=$(cd `dirname $0`; pwd)
cd ${CURDIR}

rm -rf ../../repo/eview-pkdata
mkdir -p ../../repo/eview-pkdata

mkdir -p ../../repo/eview-pkdata/include
mkdir -p ../../repo/eview-pkdata/bin

echo cp -f include file...
cp -rf ../../include/pkdata ../../repo/eview-pkdata/include/
cp -rf ../../bin/libpkdata*.* ../../repo/eview-pkdata/bin/

find ../../repo/eview-pkdata -name ".svn"|xargs rm -rf
echo now package eview-pkdata ......
rm -f ../../repo/eview-pkdata.tar.gz
tar -cvzf ../../repo/eview-pkdata.tar.gz -C ../../repo/eview-pkdata .
CURDATE=$(date +%Y%m%d)
cp  -f ../../repo/eview-pkdata.tar.gz ../../repo/eview-pkdata-${CURDATE}.tar.gz
echo end package it to ../../repo/eview-pkdata.tar.gz

cd ${CURDIR}
