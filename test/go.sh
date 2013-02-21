#!/bin/sh

make || exit $?

rm -f test/flop.img
dd if=/dev/zero of=test/flop.img bs=512 count=2880

bin/mkfs.jgfs2 test/flop.img || exit $?

sudo umount /mnt/0
bin/mount.jgfs2 test/flop.img /mnt/0
