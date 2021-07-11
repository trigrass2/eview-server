#~/bin/sh
curdir=$(cd `dirname $0`; pwd)
BINPATH=${curdir}/bin

echo prepare set LD_LIBRARY_PATH
export LD_LIBRARY_PATH=${BINPATH}:$LD_LIBRARY_PATH
echo LD_LIBRARY_PATH set OK

./start_pkmemdb &
./pknodeserver &
#./pkeventserver &
./pklinkserver &
./start_pkmqtt &
./pkmodserver &
./drivers/modbustcp/modbustcp &
./start_pkhisdb &
./pkhisdataserver &
./web/startWeb &
