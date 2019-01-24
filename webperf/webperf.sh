#!/bin/sh

for t in $(cat /tmp/ispmon/targets/webperf ); do
    /tmp/ispmon/scripts/webperfconf.sh $t;
done
