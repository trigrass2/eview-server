#!/bin/sh
kill -9 pkhisdb
del ../hisdata/mongod.lock
./pkhisdb -f ../config/pkhisdb.conf
