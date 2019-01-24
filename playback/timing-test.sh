#!/bin/bash

if [ ! -z "$1" ]; then
	threads=$1
else
	threads=10
fi

test() {
	total=0
	n=0
	for i in $(seq 0 9); do
		t=$(/usr/bin/time -f "%e" curl --silent -o /dev/null http://localhost/ 2>&1)
		if [ $? -ne 0 ]; then
			return
		fi
		total=$(bc -l <<< "$total + $t")
		((n++))	
	done
	avg=$(bc -l <<< "scale=3;$total / $n")
	echo "$n $total $avg"
}

for i in $(seq 1 $threads); do
	test &
done
wait
