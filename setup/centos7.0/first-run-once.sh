set CURDIR=$(cd `dirname $0`; pwd)
cd ${CURDIR}


cp -f /eview-server/etc/init.d/eview /etc/init.d/eview

#add to autostart service
cd /etc/rc5.d
ln -s /etc/init.d/eview S99eview

echo == PLEASE execute: reboot command, or execute: source /etc/profile ==

