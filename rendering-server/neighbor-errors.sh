#!/bin/bash
#Convert video into images
#rm frames/*.bmp 2>/dev/null
mkdir $1
ffmpeg -i $1.avi -r 10 $1/image-%d.bmp
begin=0
pixels=$((1920*1200))
echo "prev i errors delta" > diffs.txt
frames=$(ls -1 $1/*.bmp|wc -l)
for i in $(seq 2 $frames); do
#	echo "test debug 1"
	prev=$((i-1))
	errors=$(compare -metric AE -fuzz 5% $1/image-$prev.bmp $1/image-$i.bmp /dev/null 2>&1)
	echo "$prev $i $errors" >> $1.txt
#	echo "debug 2"
done
DS=$HOME"/data-store/"$1"/"
rm  -rf $1
rm $1.avi
if [ -e $DS ];then
	rm `find $DS -name '*.body'` -rf 
fi

