#!/bin/bash
CURDIR=$(cd `dirname $0`; pwd)
cd ${CURDIR}

ftp -v -n 192.168.10.1 21 <<EOF
user peak peak
binary

prompt
lcd ../../repo 
mkdir /release/eview-server/centos7.0
cd /release/eview-server/centos7.0
mput first-run-once.sh
mput eview-server-base.tar.gz eview-pkdata.tar.gz eview-server-driversdk.tar.gz

mkdir /release/eview-server/centos7.0/backup
cd /release/eview-server/centos7.0/backup
mput eview-server-base-2*.tar.gz eview-pkdata-2*.tar.gz eview-server-driversdk-2*.tar.gz 

close
bye
!

cd ${CURDIR}
