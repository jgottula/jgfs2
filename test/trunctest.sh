#!/bin/sh

if [[ "$1" == "" ]]; then
	echo "param: number of truncations"
	exit 1
fi

for i in $(seq 1 $1); do
	truncate -s $RANDOM /mnt/0/file
done
