#!/bin/sh
CURDIR=$(cd `dirname $0`; pwd)
cd ${CURDIR}


if [ ! -f "../../cmake-temp" ]; then
	mkdir ../../cmake-temp
fi

cd ../../cmake-temp
cmake -DARM=1 ../source && make

cd ${CURDIR}