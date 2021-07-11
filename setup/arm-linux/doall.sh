#!/bin/sh
CURDIR=$(cd `dirname $0`; pwd)
cd ${CURDIR}

sh 1.update.sh
sh 2.download.sh
sh 3.prebuild.sh
sh 4.build.sh
sh 5.postbuild.sh

cd ${CURDIR}