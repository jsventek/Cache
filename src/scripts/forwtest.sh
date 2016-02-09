#!/bin/bash
#
# usage: ./cachetest.sh <arguments>

{ export count=1;
while true; do
	if [ $count -ge 1000000 ]; then
		count=1;
	fi
	i=$((count++));
	echo "insert into a values ('$i', '$i.0')";
	sleep 1;
done; } | ./cacheclient $*
