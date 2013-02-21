#!/bin/sh

PID=$(pidof mount.jgfs2)

if [[ "$PID" == "" ]]; then
  echo "mount.jgfs2 is not running"
  exit 1
fi

gdb bin/mount.jgfs2 $PID
