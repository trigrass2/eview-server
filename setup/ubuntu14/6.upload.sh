#!/bin/bash 当前目录需是setup/arm-linux
CURDIR=$(cd `dirname $0`; pwd)
cd ${CURDIR}

ftp -v -n ftp.peakinfo.cn 2121 <<EOF
user peak peak
binary
passive
prompt

lcd ../../repo 
mkdir /release/eview-server/ubuntu14
mkdir /release/eview-server/ubuntu14/backup
cd /release/eview-server/ubuntu14 
mput eview-server-base.tar.gz eview-pkdata.tar.gz eview-server-driversdk.tar.gz

cd /release/eview-server/ubuntu14/backup
mput eview-server-base-2*.tar.gz eview-pkdata-2*.tar.gz eview-server-driversdk-2*.tar.gz 

close
bye
!

cd ${CURDIR}
