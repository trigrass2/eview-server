#!/bin/bash
CURDIR=$(cd `dirname $0`; pwd)
echo "CURDIR", ${CURDIR}
cd ${CURDIR}

svn up ../..
#cd ${CURDIR}
