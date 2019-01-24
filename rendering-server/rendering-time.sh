#!/bin/bash

if [ -z "$1" ]; then
        echo "Usage: $0 <URL>"
        exit 1
fi
#trap killer SIGINT
killer(){
        echo "Closing ..."
        kill 0
}

#set display
#export DISPLAY=:99
export DISPLAY=:$4
Xvfb :$4 -screen 0 1920x1200x24 &
#rm log.txt
starttime=$(date +%Y%m%d-%H:%M:%S)
java -jar CaptureScreen.jar $1 $3
./neighbor-errors.sh $3
url=$(basename $1)
./detect.py $2 $3 $url
endtime=$(date +%Y%m%d-%H:%M:%S)
rm -f ".X"$4"-lock"
echo $3"\t"$starttime"\t"$endtime >> processing
