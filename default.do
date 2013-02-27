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
-Isrc"
export CXXFLAGS="-std=c++11 -O0 -ggdb -Wall -Wextra -Wno-unused-parameter \
-Wno-unused-function -include cstddef -include cstdint -Isrc"


DEFINES="-D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26"

LIB_OUT="bin/libjgfs2.a"
LIB_SRC=(lib/*.c lib/*/*.c)
LIB_OBJS=${LIB_SRC[@]//.c/.o}
LIB_LIBS=(-lbsd -luuid)

FUSE_OUT="bin/fuse.jgfs2"
FUSE_SRC=(src/fuse/*.c)
FUSE_OBJS=${FUSE_SRC[@]//.c/.o}
FUSE_LIBS=(-lbsd -lfuse)

TEST_OUT="bin/test.jgfs2"
TEST_SRC=(src/test/*.c)
TEST_OBJS=${TEST_SRC[@]//.c/.o}
TEST_LIBS=(-lbsd)

TREE_OUT="bin/tree.jgfs2"
TREE_SRC=(src/tree/*.cxx)
TREE_OBJS=${TREE_SRC[@]//.cxx/.o}
TREE_LIBS=(-lQtCore)

VIEW_OUT="bin/view.jgfs2"
VIEW_SRC=(src/view/*.c)
VIEW_OBJS=${VIEW_SRC[@]//.c/.o}
VIEW_LIBS=()

MKFS_OUT="bin/mkfs.jgfs2"
MKFS_SRC=(src/mkfs/*.c)
MKFS_OBJS=${MKFS_SRC[@]//.c/.o}
MKFS_LIBS=(-lbsd -luuid)

FSCK_OUT="bin/fsck.jgfs2"
FSCK_SRC=(src/fsck/*.c)
FSCK_OBJS=${FSCK_SRC[@]//.c/.o}
FSCK_LIBS=()

DEFRAG_OUT="bin/defrag.jgfs2"
DEFRAG_SRC=(src/defrag/*.c)
DEFRAG_OBJS=${DEFRAG_SRC[@]//.c/.o}
DEFRAG_LIBS=()

FSCTL_OUT="bin/fsctl.jgfs2"
FSCTL_SRC=(src/fsctl/*.c)
FSCTL_OBJS=${FSCTL_SRC[@]//.c/.o}
FSCTL_LIBS=()

ATTR_OUT="bin/attr.jgfs2"
ATTR_SRC=(src/attr/*.c)
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
	redo lib fuse test tree view mkfs fsck defrag fsctl attr ;;
lib)
	redo-ifchange $LIB_OUT ;;
fuse)
	redo-ifchange $FUSE_OUT ;;
test)
	redo-ifchange $TEST_OUT ;;
tree)
	redo-ifchange $TREE_OUT ;;
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
	LIBS="${LIB_LIBS[@]}"
	OBJS="${LIB_OBJS[@]}"
	target_lib
	;;
$FUSE_OUT)
	LIBS="${FUSE_LIBS[@]}"
	OBJS="${FUSE_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$TEST_OUT)
	LIBS="${TEST_LIBS[@]}"
	OBJS="${TEST_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$TREE_OUT)
	LIBS="${TREE_LIBS[@]}"
	OBJS="${TREE_OBJS[@]} $LIB_OUT"
	target_cxx_link
	;;
$VIEW_OUT)
	LIBS="${VIEW_LIBS[@]}"
	OBJS="${VIEW_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$MKFS_OUT)
	LIBS="${MKFS_LIBS[@]}"
	OBJS="${MKFS_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$FSCK_OUT)
	LIBS="${FSCK_LIBS[@]}"
	OBJS="${FSCK_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$DEFRAG_OUT)
	LIBS="${DEFRAG_LIBS[@]}"
	OBJS="${DEFRAG_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$FSCTL_OUT)
	LIBS="${FSCTL_LIBS[@]}"
	OBJS="${FSCTL_OBJS[@]} $LIB_OUT"
	target_c_link
	;;
$ATTR_OUT)
	LIBS="${ATTR_LIBS[@]}"
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
