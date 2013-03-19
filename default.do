# jgfs2
# (c) 2013 Justin Gottula
# The source code of this project is distributed under the terms of the
# simplified BSD license. See the LICENSE file for details.


exec >&2


TARGET=$1
TARGET_BASE=$2
OUTPUT=$3


export CC="ccache gcc"
export CXX="ccache g++"
export AR=ar

export CFLAGS="-std=gnu11 -O0 -ggdb -Wall -Wextra -Wno-unused-parameter \
-Wno-unused-function -include stddef.h -include stdbool.h -include stdint.h \
-Isrc -flto"
export CXXFLAGS="-std=c++11 -O0 -ggdb -Wall -Wextra -Wno-unused-parameter \
-Wno-unused-function -include cstddef -include cstdint -Isrc -flto"


DEFINES="-D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 \
-DDEBUG_FATAL"

LIB_OUT="bin/libjgfs2.a"
LIB_SRC=$(find lib -type f -iname '*.c')
LIB_OBJS=${LIB_SRC[@]//.c/.o}
LIB_LIBS=(-lm -lbsd -luuid)

FUSE_OUT="bin/fuse.jgfs2"
FUSE_SRC=$(find src/fuse -type f -iname '*.c')
FUSE_OBJS=${FUSE_SRC[@]//.c/.o}
FUSE_LIBS=(-lfuse)

TEST_OUT="bin/test.jgfs2"
TEST_SRC=$(find src/test -type f -iname '*.c')
TEST_OBJS=${TEST_SRC[@]//.c/.o}
TEST_LIBS=()

VIEW_OUT="bin/view.jgfs2"
VIEW_SRC=$(find src/view -type f -iname '*.c')
VIEW_OBJS=${VIEW_SRC[@]//.c/.o}
VIEW_LIBS=()

MKFS_OUT="bin/mkfs.jgfs2"
MKFS_SRC=$(find src/mkfs -type f -iname '*.c')
MKFS_OBJS=${MKFS_SRC[@]//.c/.o}
MKFS_LIBS=()

FSCK_OUT="bin/fsck.jgfs2"
FSCK_SRC=$(find src/fsck -type f -iname '*.c')
FSCK_OBJS=${FSCK_SRC[@]//.c/.o}
FSCK_LIBS=()

DEFRAG_OUT="bin/defrag.jgfs2"
DEFRAG_SRC=$(find src/defrag -type f -iname '*.c')
DEFRAG_OBJS=${DEFRAG_SRC[@]//.c/.o}
DEFRAG_LIBS=()

FSCTL_OUT="bin/fsctl.jgfs2"
FSCTL_SRC=$(find src/fsctl -type f -iname '*.c')
FSCTL_OBJS=${FSCTL_SRC[@]//.c/.o}
FSCTL_LIBS=()

ATTR_OUT="bin/attr.jgfs2"
ATTR_SRC=$(find src/attr -type f -iname '*.c')
ATTR_OBJS=${ATTR_SRC[@]//.c/.o}
ATTR_LIBS=()


function target_cxx_dep {
	$CXX $CXXFLAGS $DEFINES -o${TARGET//.o/.dep} -MM -MG ${TARGET//.o/.cxx}
}

function target_cxx {
	target_cxx_dep
	read DEPS <${TARGET//.o/.dep}
	redo-ifchange ${DEPS#*:}
	
	$CXX $CXXFLAGS $DEFINES -o$OUTPUT -c ${TARGET//.o/.cxx}
}

function target_cxx_link {
	redo-ifchange $OBJS
	
	$CXX $CFXXLAGS $LIBS -o$OUTPUT $OBJS
}

function target_c_dep {
	$CC $CFLAGS $DEFINES -o${TARGET//.o/.dep} -MM -MG ${TARGET//.o/.c}
}

function target_c {
	target_c_dep
	read DEPS <${TARGET//.o/.dep}
	redo-ifchange ${DEPS#*:}
	
	$CC $CFLAGS $DEFINES -o$OUTPUT -c ${TARGET//.o/.c}
}

function target_c_link {
	redo-ifchange $OBJS
	
	$CC $CFLAGS $LIBS -o$OUTPUT $OBJS
}

function target_lib {
	redo-ifchange $OBJS
	
	$AR rcs $OUTPUT $OBJS
}


case "$TARGET" in
all)
	redo lib fuse test view mkfs fsck defrag fsctl attr ;;
lib)
	redo-ifchange $LIB_OUT ;;
fuse)
	redo-ifchange $FUSE_OUT ;;
test)
	redo-ifchange $TEST_OUT ;;
view)
	redo-ifchange $VIEW_OUT ;;
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
$LIB_OUT)
	LIBS=""
	OBJS="${LIB_OBJS[@]}"
	target_lib
	;;
$FUSE_OUT)
	LIBS="${FUSE_LIBS[@]} ${LIB_LIBS[@]}"
	OBJS="${FUSE_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$TEST_OUT)
	LIBS="${TEST_LIBS[@]} ${LIB_LIBS[@]}"
	OBJS="${TEST_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$VIEW_OUT)
	LIBS="${VIEW_LIBS[@]} ${LIB_LIBS[@]}"
	OBJS="${VIEW_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$MKFS_OUT)
	LIBS="${MKFS_LIBS[@]} ${LIB_LIBS[@]}"
	OBJS="${MKFS_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$FSCK_OUT)
	LIBS="${FSCK_LIBS[@]} ${LIB_LIBS[@]}"
	OBJS="${FSCK_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$DEFRAG_OUT)
	LIBS="${DEFRAG_LIBS[@]} ${LIB_LIBS[@]}"
	OBJS="${DEFRAG_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$FSCTL_OUT)
	LIBS="${FSCTL_LIBS[@]} ${LIB_LIBS[@]}"
	OBJS="${FSCTL_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$ATTR_OUT)
	LIBS="${ATTR_LIBS[@]} ${LIB_LIBS[@]}"
	OBJS="${ATTR_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
*.o)
	if [[ -e "${TARGET//.o/.cxx}" ]]; then
		target_cxx
	elif [[ -e "${TARGET//.o/.c}" ]]; then
		target_c
	else
		echo "cannot find source file for '$TARGET'"
		exit 1
	fi
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
