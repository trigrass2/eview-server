#execute once on install, for eview-server on linux

set CURDIR=$(cd `dirname $0`; pwd)
cd ${CURDIR}

#delete boa
/etc/init.d/boa stop
rm /etc/init.d/boa
rm -rf /usr/sbin/boa
rm /etc/rc5.d/S20boa

#copy eview-server, but donot copy eview.db and db.conf!!
rm -rf /eview-server/data /eview-server/log /eview-server/bin /eview-server/python
mkdir /eview-server
cd /eview-server
tar -xzvf ${CURDIR}/eview-server.tar.gz
tar -xzvf ${CURDIR}/eview-server-commondriver.tar.gz

#install service eview
echo copy eview-server service script
cp -f /eview-server/etc/init.d/eview /etc/init.d/eview
chmod +x /etc/init.d/eview
cp -f /eview-server/etc/profile /etc/

#set auto restart eview
echo add to autostart service
cd /etc/rc5.d
rm /etc/rc5.d/S99eview
ln -s /etc/init.d/eview S99eview

#rm -rf /eview-server/etc
echo == eview-server init success,usage: /etc/init.d/eview start or stop ==

