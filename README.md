jgfs2
=====
`jgfs2` is intended to be a modern, but relatively simple, filesystem based on
some of the positive design aspects of `btrfs`. In particular, `jgfs2` borrows
the concept of using a single tree for all filesystem metadata from `btrfs`;
however, `jgfs2` has no copy-on-write functionality, and uses B+ trees, as
contrasted with `btrfs`'s modified B-trees.

The primary purpose of jgfs2 is to provide a relatively modern, but not overly
complicated, filesystem for use with the `justix` operating system project.

building
--------
This project uses Avery Pennarun's implementation of D. J. Bernstein's `redo`
build system. For more information, see [djb's redo page][1] and [apenwarr's
redo implementation][2].

To build `libjgfs2`, the `FUSE` program, and associated utilities, install
`redo` and simply run `redo all`. To clean the project directory, run
`redo clean`.

To build individual targets, run one of the following:

- `redo lib`: build the `libjgfs2` library, `bin/libjgfs2.a`
- `redo test`: build the unit test utility, `bin/test.jgfs2`
- `redo tree`: build the tree viewer utility, `bin/tree.jgfs2`
- `redo fuse`: build the `FUSE` program, `bin/fuse.jgfs2`
- `redo mkfs`: build the `mkfs` utility, `bin/mkfs.jgfs2`
- `redo fsck`: build the `fsck` utility, `bin/fsck.jgfs2`
- `redo defrag`: build the `defrag` utility, `bin/defrag.jgfs2`
- `redo fsctl`: build the filesystem control utility, `bin/fsctl.jgfs2`
- `redo attr`: build the file attribute utility, `bin/attr.jgfs2`

running
-------
Make a new `jgfs2` filesystem on a device or file:

    bin/mkfs.jgfs2 <device>

Mount the filesystem using `FUSE`:

    bin/fuse.jgfs2 <device> <mountpoint>

directories
-----------
- `bin`: contains the `libjgfs2` library and utility binaries after a build
- `doc`: documentation related to the filesystem's development
- `lib`: source code for `libjgfs2`
- `src`: source code for `jgfs2` utilities and the `FUSE` program
- `test`: various scripts used for testing the filesystem

license
-------
This project is licensed under the terms of the simplified (2-clause) BSD
license. For more information, see the `LICENSE` file contained in the project's
root directory.
