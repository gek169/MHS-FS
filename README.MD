# MHS Filesystem

The MHS filesystem. Features:

* can be modified to work with any size of disk or sector, even non powers of two!
* Allocation bitmap stored on-disk for improved boot times.
* Recoverability- if the power shuts off during a write, as long as a *sector* write was atomic (I.E. not interrupted *during* a sector write)
then the system will recover just fine. However, the allocation bitmap may have to be re-generated (SLOW)
* Very easy to implement. Much simpler than Fat32.

Any disk which can be abstracted into a linear series of sectors or blocks can be made to work with this filesystem.

A simple driver has been implemented which allows transferring files from the host into the virtual hard disk.

The virtual hard disk is 32 megabytes by default and is initialized to zero if it doesn't exist.


# INTEGRATION

(See MHS.h for most up-to-date information)

Include MHS.h in as many files as you like.

Modify the SECTOR_SIZE define and the MHS_UINT typedef to be those you would like.

use the file_ api calls to interact with the filesystem.

To modify file node attributes such as permission bits and owner UID and whatnot, you should use resolve_path
passing in a non-const char array. It is recommended to use the library-provided pathbuf for this purpose which is 64k in size.

You would then use load_sector, modify the node, and write it back. As long as you don't change the rptr or dptr, this won't require

locking the modify bit of the filesystem.

For faster reads/writes it is recommended not to use file_read_sector and file_write_sector but rather to keep a file descriptor open,
which keeps a cache of the file's size and location on disk for sector load/store ops.

Every time the file is resized, this information will need to be updated.

See the example program main.c for usage.

# Why this over Fat or Fat32, or other Filesystems

Advantages of MHS-FS over other filesystems:

1) Much easier to implement. Hyper-minimal. Approximately 1.1k sloc of C89.

2) Zero intellectual property issues. This filesystem and all the code in this repository are fully public domain.

3) Flexible. The filesystem's sector size or default integer size can be changed with relative ease. Additional file attributes can be added
to the fsnode struct by changing MHS_NATTRIBS and adding getters/setters for it (it should be after the existing attributes).

	Trying to "hack" your own version of fat32 is a nightmare.

4) Portable. No Gnu packed structs are used. It compiles and runs flawlessly with tinycc or GCC.

5) Memory efficient. Very little memory is used for loading and storing sectors. It would be an optimal choice for
a system with very little memory such as a Commodore 64, or the Commander X16, or an AVR microcontroller.


# Disadvantages of this filesystem

1) Brand new. So obviously nothing supports it... maybe that will change.

2) A little slow. While a bitmap is used rather than a FAT, the disk allocation routines are highly unoptimized and perform redundant
	out-of-track reads/writes. For large writes, this is not an issue, but for numerous small (1-2 sector) allocation operations it is
	very inefficient, performing about twice as many sector loads and stores as needed.

	Under ordinary operation this shouldn't be an issue but if you were unzipping a folder with thousands of tiny files
	(Like a retro game ROM dump) your disk throughput would be about half or a third the speed of your hard disk.

	A filesystem or database that is fast but doesn't properly record your data is useless (See: MongoDB vs MySQL)

	So I am focusing on correctness and making the code mature enough for serious usage rather than trying to make it fast.
	Obvious and simple optimizations which I think are "safe" are made, but any that significantly increase code complexity
	are not taken, because bugs will sneak in, lay eggs, and there will be an entire nest before you can blink.

	I heavily avoid recursive algorithms to prevent stack overflows and try to be memory-efficient. Malloc is not used.

# Structure

There are only two relevant data structures in the file system:

* Fsnode. Contains the metadata of a file, including a pointer to the first byte on disk. Size: 1 sector

	The fundamental and most important elements of the fsnode, are the right pointer, down pointer, size, and permission bits.


	The permission bits include a single bit for determining whether the node is a file or a directory, along with UNIX file permission bits.


	The size is only used for file nodes, and it indicates how big the file is in bytes. The number of sectors allocated for the file is always exactly large enough to hold this many bytes-

	if you have a 550 byte file and the sector size is 512 bytes, then 2 sectors are used. a 512 byte file will use 1 sector. A 0 byte file 

	is a special case- if it has a dptr, it is assumed to be a single sector in size.


	The right pointer (rptr) points to the next fsnode in the same directory. The last entry in the directory has a null (0) rptr.


	The down pointer points either to the first entry in a directory, for directory fsnodes, or the data of a file, for file fsnodes.


	The structure of the filesystem, then, is a binary tree. File nodes are simple singly-linked-list entries with data,

	and directory nodes use the same singly linked list, but have a child linked list where a file's data pointer is.


	you can think of "dptr" as either "down" in a directory, or as "data" in a file.

	

* Allocation bitmap. A special file which marks areas on the disk in use and free. Size is determined by the size of the disk.

	The size of the allocation bitmap is used by the filesystem at runtime to determine the size of the disk- The device is never queried.

	It is assumed that during your disk formatting, you will initialize the filesystem in whatever partition you choose to put it in.

	The location of the allocation bitmap is also important- Data for files cannot be allocated before it. Data is only allocated after the allocation bitmap.

	The space before the allocation bitmap is reserved solely for fsnodes. The space after can be used either for fsnodes, or for data.

	(This improves seek times and reduces fragmentation of the disk)


The fsnode at [0] is the root node. the root node has special properties:

* The root node is a file node, but most functions have special code to treat it like a directory. Its permission bits are always `__R__R__ ____ ____`

* Even though it is treated like a directory node, the files in root are pointed to by its rptr. If the rptr is null, the filesystem is empty.

* The dptr of the root node points to the allocation bitmap.

* The size attribute of the root node is exactly one eighth the usable size of the disk (in sectors). It is the size of the allocation bitmap.








