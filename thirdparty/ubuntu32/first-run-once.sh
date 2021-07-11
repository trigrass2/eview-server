curdir=$(cd `dirname $0`; pwd)
BINPATH=${curdir}/bin
BINPATH=/home/root/eview-server/bin

echo prepare set LD_LIBRARY_PATH
echo export LD_LIBRARY_PATH=${BINPATH}:$LD_LIBRARY_PATH >> /etc/profile
echo OK to set LD_LIBRARY_PATH to /etc/profile
source /etc/profile


