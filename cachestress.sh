#!/bin/bash
#
# usage: ./cachestress.sh <arguments>

{ echo "create table b (i integer, r real)";
export count=1;
while true; do
	if [ $count -ge 1000000 ]; then
		count=1;
	fi
	for f in 1 2 3 4 5 6 7 8 9 10; do
		i=$((count++));
		echo "insert into b values ('$i', '$i.0')";
	done;
done; } | ./cacheclient -l packets $*
