# jgfs2 redo script
#
# (c) 2013 Justin Gottula
# The source code of this project is distributed under the terms of the
# simplified BSD license. See the LICENSE file for details.
#

exec >&2


TARGET=$1
TARGET_BASE=$2
OUTPUT=$3


export CC="ccache gcc"
export AR=ar

export CFLAGS="-std=gnu11 -O0 -ggdb -Wall -Wextra -Wno-unused-parameter \
-Wno-unused-function -include stdbool.h -include stdint.h -Isrc"


DEFINES="-D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26"

JGFS_OUT="bin/libjgfs2.a"
JGFS_SRC=(lib/*.c)
JGFS_OBJS=${JGFS_SRC[@]//.c/.o}
JGFS_LIBS=(-lbsd -luuid)

FUSE_OUT="bin/jgfs2fuse"
FUSE_SRC=(src/fuse/*.c)
FUSE_OBJS=${FUSE_SRC[@]//.c/.o}
FUSE_LIBS=(-lbsd -lfuse)

MKFS_OUT="bin/jgfs2mkfs"
MKFS_SRC=(src/mkfs/*.c)
MKFS_OBJS=${MKFS_SRC[@]//.c/.o}
MKFS_LIBS=(-lbsd -luuid)

FSCK_OUT="bin/jgfs2fsck"
FSCK_SRC=(src/fsck/*.c)
FSCK_OBJS=${FSCK_SRC[@]//.c/.o}
FSCK_LIBS=()

DEFRAG_OUT="bin/jgfs2defrag"
DEFRAG_SRC=(src/defrag/*.c)
DEFRAG_OBJS=${DEFRAG_SRC[@]//.c/.o}
DEFRAG_LIBS=()

FSCTL_OUT="bin/jgfs2ctl"
FSCTL_SRC=(src/fsctl/*.c)
FSCTL_OBJS=${FSCTL_SRC[@]//.c/.o}
FSCTL_LIBS=()

ATTR_OUT="bin/jgfs2attr"
ATTR_SRC=(src/attr/*.c)
ATTR_OBJS=${ATTR_SRC[@]//.c/.o}
ATTR_LIBS=()


function target_gcc_dep {
	$CC $CFLAGS $DEFINES -o${TARGET//.o/.dep} -MM -MG ${TARGET//.o/.c}
}

function target_gcc {
	target_gcc_dep
	read DEPS <${TARGET//.o/.dep}
	redo-ifchange ${DEPS#*:}
	
	$CC $CFLAGS $DEFINES -o$OUTPUT -c ${TARGET//.o/.c}
}

function target_link {
	redo-ifchange $OBJS
	
	$CC $CFLAGS $LIBS -o$OUTPUT $OBJS
}

function target_lib {
	redo-ifchange $OBJS
	
	$AR rcs $OUTPUT $OBJS
}


case "$TARGET" in
all)
	redo lib fuse mkfs fsck defrag fsctl attr ;;
lib)
	redo-ifchange $JGFS_OUT ;;
fuse)
	redo-ifchange $FUSE_OUT ;;
mkfs)
	redo-ifchange $MKFS_OUT ;;
fsck)
	redo-ifchange $FSCK_OUT ;;
defrag)
	redo-ifchange $DEFRAG_OUT ;;
fsctl)
	redo-ifchange $FSCTL_OUT ;;
attr)
	redo-ifchange $ATTR_OUT ;;
$JGFS_OUT)
	LIBS="${JGFS_LIBS[@]}"
	OBJS="${JGFS_OBJS[@]}"
	target_lib
	;;
$FUSE_OUT)
	LIBS="${FUSE_LIBS[@]}"
	OBJS="${FUSE_OBJS[@]} $JGFS_OUT"
	target_link
	;;
$MKFS_OUT)
	LIBS="${MKFS_LIBS[@]}"
	OBJS="${MKFS_OBJS[@]} $JGFS_OUT"
	target_link
	;;
$FSCK_OUT)
	LIBS="${FSCK_LIBS[@]}"
	OBJS="${FSCK_OBJS[@]} $JGFS_OUT"
	target_link
	;;
$DEFRAG_OUT)
	LIBS="${DEFRAG_LIBS[@]}"
	OBJS="${DEFRAG_OBJS[@]} $JGFS_OUT"
	target_link
	;;
$FSCTL_OUT)
	LIBS="${FSCTL_LIBS[@]}"
	OBJS="${FSCTL_OBJS[@]} $JGFS_OUT"
	target_link
	;;
$ATTR_OUT)
	LIBS="${ATTR_LIBS[@]}"
	OBJS="${ATTR_OBJS[@]} $JGFS_OUT"
	target_link
	;;
*.o)
	target_gcc
	;;
clean)
	rm -rf $(find bin/ -type f)
	rm -rf $(find src/ lib/ -type f -iname *.o)
	rm -rf $(find src/ lib/ -type f -iname *.dep)
	;;
backup)
	cd .. && tar -acvf backup/jgfs2-$(date +'%Y%m%d-%H%M').tar.xz jgfs2/
	;;
*)
	echo "unknown target '$TARGET'"
	exit 1
	;;
esac
