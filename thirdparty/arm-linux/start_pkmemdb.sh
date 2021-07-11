#!/bin/sh
logpath="../log/"
#if [ ! -d "$logpath"]; then  
#　　mkdir "$logpath"  
#fi  

mkdir ../log 
./pkmemdb ../config/pkmemdb.conf
