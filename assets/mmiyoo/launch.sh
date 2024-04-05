#!/bin/sh
CUST_CPUCLOCK=1
USE_752x560_RES=0

mydir=`dirname "$0"`

export HOME=$mydir
export PATH=$mydir:$PATH
export LD_LIBRARY_PATH=$mydir/lib:$LD_LIBRARY_PATH
export SDL_VIDEODRIVER=mmiyoo
export SDL_AUDIODRIVER=mmiyoo
export EGL_VIDEODRIVER=mmiyoo

if [ -f /mnt/SDCARD/.tmp_update/script/stop_audioserver.sh ]; then
    /mnt/SDCARD/.tmp_update/script/stop_audioserver.sh
else
    killall audioserver
    killall audioserver.mod
fi

if [  -d "/customer/app/skin_large" ]; then
    USE_752x560_RES=1
fi

if [ "$USE_752x560_RES" == "1" ]; then
    fbset -g 752 560 752 1120 32
fi

cd $mydir

sv=`cat /proc/sys/vm/swappiness`

# 60 by default
echo 10 > /proc/sys/vm/swappiness

cd $mydir

if [ "$CUST_CPUCLOCK" == "1" ]; then
    ./cpuclock 1500
fi

./mupen64plus "$1"
sync

echo $sv > /proc/sys/vm/swappiness

if [  -d "/customer/app/skin_large" ]; then
    USE_752x560_RES=0
fi

if [ "$USE_752x560_RES" == "1" ]; then
    fbset -g 640 480 640 960 32
fi
