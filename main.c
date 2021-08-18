/*
	Example program to demonstrate MHSFS

	jump points (ctrl + f to search for these)
	ALLOWED_FILENAME_CHARACTERS - an if statement that checks the allowed filename characters.
	CONFIGURABLES - compiletime toggles/options for the filesystem driver.
	BITMAP_LOCATION - where the bitmap is stored by default. It can be moved later, of course.

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stringutil.h"


void my_strcpy(char* dest, const char* src){
	while(*src) *dest++ = *src++;
	*dest = 0;
}

/*0x20000000 is 512 megs*/

/*
	~~~~~~~~~~~~~~~~~~~~~~~CONFIGURABLES~~~~~~~~~~~~~~~~~~~
*/
/*How many sectors are on the disk?*/
#define DISK_SIZE 0x10000
/*Sector size?*/
#define SECTOR_SIZE 512
/*How many sectors to "skip" for some boot code or MBR*/
#define SECTOR_OFFSET 0
#define BITMAP_START 0x20

#define IS_DIRECTORY 32768
#define IS_SUID 16384
#define OWNER_R 8192
#define OWNER_W 4096
#define OWNER_X 2048
#define PUBLIC_R 1024
#define PUBLIC_W 512
#define PUBLIC_X 256

#define GROUP_R 128
#define GROUP_W 64
#define GROUP_X 32
#define IS_GUID 16
#define UNUSED_PERM_1 8
#define UNUSED_PERM_2 4
#define UNUSED_PERM_3 2
#define UNUSED_PERM_4 1

typedef unsigned int uint_dsk; /*32 or 64 bit integer.*/
typedef unsigned short ushort; /*16 bit unsigned int.*/
FILE* f; /*the disk.*/
typedef struct{
	unsigned char data[SECTOR_SIZE];
} sector;

typedef struct{
	char d[SECTOR_SIZE - (4 * sizeof(uint_dsk) + 2)];
} fname_string;

static void sector_write_byte(sector* sect, uint_dsk loc, unsigned char byte){
	loc %= (SECTOR_SIZE - 1);sect->data[loc] = byte;
}

static unsigned char sector_read_byte(sector* sect, uint_dsk loc){
	loc %= (SECTOR_SIZE - 1);return sect->data[loc];
}

void sector_write_uint_dsk(sector* sect, uint_dsk loc, uint_dsk val){

	sector_write_byte(sect, loc+0, val >> ((sizeof(uint_dsk) - 1) * 8)    );if(sizeof(uint_dsk) == 1) return;
	sector_write_byte(sect, loc+1, val >> ((sizeof(uint_dsk) - 2) * 8)    );if(sizeof(uint_dsk) == 2) return;	
	sector_write_byte(sect, loc+2, val >> ((sizeof(uint_dsk) - 3) * 8)    );if(sizeof(uint_dsk) == 3) return;
	sector_write_byte(sect, loc+3, val >> ((sizeof(uint_dsk) - 4) * 8)    );if(sizeof(uint_dsk) == 4) return;

	sector_write_byte(sect, loc+4, val >> ((sizeof(uint_dsk) - 5) * 8)    );if(sizeof(uint_dsk) == 5) return;
	sector_write_byte(sect, loc+5, val >> ((sizeof(uint_dsk) - 6) * 8)    );if(sizeof(uint_dsk) == 6) return;
	sector_write_byte(sect, loc+6, val >> ((sizeof(uint_dsk) - 7) * 8)    );if(sizeof(uint_dsk) == 7) return;
	sector_write_byte(sect, loc+7, val >> ((sizeof(uint_dsk) - 8) * 8)    );if(sizeof(uint_dsk) == 8) return;
}

uint_dsk sector_read_uint_dsk(sector* sect, uint_dsk loc){
	uint_dsk val = 0;
	uint_dsk val1 = 0;

	val1 = sector_read_byte(sect, loc + 0); val1 <<= ((sizeof(uint_dsk) - 1) * 8); val |= val1; if(sizeof(uint_dsk) == 1) return val;
	val1 = sector_read_byte(sect, loc + 1); val1 <<= ((sizeof(uint_dsk) - 2) * 8); val |= val1; if(sizeof(uint_dsk) == 2) return val;
	val1 = sector_read_byte(sect, loc + 2); val1 <<= ((sizeof(uint_dsk) - 3) * 8); val |= val1; if(sizeof(uint_dsk) == 3) return val;
	val1 = sector_read_byte(sect, loc + 3); val1 <<= ((sizeof(uint_dsk) - 4) * 8); val |= val1; if(sizeof(uint_dsk) == 4) return val;

	val1 = sector_read_byte(sect, loc + 4); val1 <<= ((sizeof(uint_dsk) - 5) * 8); val |= val1; if(sizeof(uint_dsk) == 5) return val;
	val1 = sector_read_byte(sect, loc + 5); val1 <<= ((sizeof(uint_dsk) - 6) * 8); val |= val1; if(sizeof(uint_dsk) == 6) return val;
	val1 = sector_read_byte(sect, loc + 6); val1 <<= ((sizeof(uint_dsk) - 7) * 8); val |= val1; if(sizeof(uint_dsk) == 7) return val;
	val1 = sector_read_byte(sect, loc + 7); val1 <<= ((sizeof(uint_dsk) - 8) * 8); val |= val1; if(sizeof(uint_dsk) == 8) return val;
}

ushort sector_fetch_perm_bits(sector* sect){
	ushort val = 0;
	val = sector_read_byte(sect, 0);
	val <<= 8;
	val |= sector_read_byte(sect, 1);
	return val;
}

void sector_write_perm_bits(sector* sect, ushort permbits){
	sector_write_byte(sect, 0, permbits / 256);
	sector_write_byte(sect, 1, permbits );
}


unsigned char sector_is_directory(sector* sect){
	return ((sector_fetch_perm_bits(sect) & IS_DIRECTORY) == 0);
}

uint_dsk sector_fetch_ownerid(sector* sect){
	uint_dsk loc = 0;
	loc+= 2; /*permission bits.*/
	return sector_read_uint_dsk(sect, loc);
}

void sector_write_ownerid(sector* sect, uint_dsk val){
	sector_write_uint_dsk(sect, 2, val);
}

uint_dsk sector_fetch_rptr(sector* sect){
	uint_dsk loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(uint_dsk); /*ownerid*/
	return sector_read_uint_dsk(sect, loc);
}

void sector_write_rptr(sector* sect, uint_dsk val){
	uint_dsk loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(uint_dsk); /*ownerid*/
	sector_write_uint_dsk(sect, loc, val);
}

uint_dsk sector_fetch_dptr(sector* sect){
	uint_dsk loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(uint_dsk); /*ownerid*/
	loc+= sizeof(uint_dsk); /*rptr*/
	return sector_read_uint_dsk(sect, loc);
}

void sector_write_dptr(sector* sect, uint_dsk val){
	uint_dsk loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(uint_dsk); /*ownerid*/
	loc+= sizeof(uint_dsk); /*rptr*/
	sector_write_uint_dsk(sect, loc, val);
}


uint_dsk sector_fetch_size(sector* sect){
	uint_dsk loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(uint_dsk); /*ownerid*/
	loc+= sizeof(uint_dsk); /*rptr*/
	loc+= sizeof(uint_dsk); /*dptr*/
	return sector_read_uint_dsk(sect, loc);
}



void sector_write_size(sector* sect, uint_dsk val){
	uint_dsk loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(uint_dsk); /*ownerid*/
	loc+= sizeof(uint_dsk); /*rptr*/
	loc+= sizeof(uint_dsk); /*dptr*/
	sector_write_uint_dsk(sect, loc, val);
}


char* sector_fetch_fname(sector* sect){
	uint_dsk loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(uint_dsk); /*ownerid*/
	loc+= sizeof(uint_dsk); /*rptr*/
	loc+= sizeof(uint_dsk); /*dptr*/
	loc+= sizeof(uint_dsk); /*size*/
	if(sect->data[SECTOR_SIZE - 1]){
		sect->data[SECTOR_SIZE - 1] = '\0'; /*guarantee null termination. This was a malformed file entry.*/
	}
	return (char*)sect->data + loc;
}

void namesan(char* name){
	uint_dsk i = 0;
	if(
		strlen(name) > 
		SECTOR_SIZE - (
			2 + 
			(4 * sizeof(uint_dsk))
		)
	)
		name[SECTOR_SIZE - (
					2 + 
					(4 * sizeof(uint_dsk))
				)] = '\0';
	while(*name){
		if(i == 0 && *name == '.')
			*name = '_';
		/*
			ALLOWED_FILENAME_CHARACTERS
		*/
		if(
			(!isalnum(*name)) &&
			(*name != '_') &&
			(*name != ' ') &&
			(*name != '.') &&
			(*name != '!') &&
			(*name != '\0') /*Obviously don't overwrite the null terminator.*/
		)
			*name = '_';
		name++;
	}
}

void pathsan(char* path){
		/*Remove repeated slashes. Thanks Applejar.*/
	{char* a; char* b;
		a = path; b = path;
		for (;;) {
		    while (a[0] == '/' && a[1] == '/') a++;
		    *b = *a;
		    if(*a == '\0') break;
		    a++; b++;
		}
	}
}


/*
	IMPORTANT- newname must NOT be a const char*

	this means you cannot simply use "my string"
	you must strdup it.
*/

void sector_write_fname(sector* sect, char* newname){
	uint_dsk loc = 0;
	uint_dsk i;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(uint_dsk); /*ownerid*/
	loc+= sizeof(uint_dsk); /*rptr*/
	loc+= sizeof(uint_dsk); /*dptr*/
	loc+= sizeof(uint_dsk); /*size*/
	namesan(newname);
	for(i = loc; i < SECTOR_SIZE-1; i++){
		sect->data[i] = newname[i - loc];
		if(newname[i - loc] == '\0') return;
	}
	sect->data[SECTOR_SIZE-1] = '\0';
}

/*
	Get the size of a file as a number of sectors.
*/

uint_dsk sector_fetch_size_in_sectors(sector* sect){
	uint_dsk fakes = sector_fetch_size(sect);
	fakes += (SECTOR_SIZE - 1); /*the classic trick for integer ceil() to a multiple.*/
	fakes /= SECTOR_SIZE;
	return fakes;
}


/*
	Load a sector from the disk.
*/

static sector sector_loader;
static sector load_sector(uint_dsk where){
	where += SECTOR_OFFSET;
	if(where >= DISK_SIZE){
		printf("<ERROR> Attempted to load sector %lu which is beyond sector bounds.", (unsigned long)where);
	}
	fseek(f, where * SECTOR_SIZE, SEEK_SET);
	fread(sector_loader.data, 1, SECTOR_SIZE, f);
	return sector_loader;
}
/*
	Store a sector to the disk.
*/
static void store_sector(uint_dsk where, sector* s){
	where += SECTOR_OFFSET;
	if(where >= DISK_SIZE){
		printf("<ERROR> Attempted to load sector %lu which is beyond sector bounds.", (unsigned long)where);
	}
	fseek(f, where * SECTOR_SIZE, SEEK_SET);
	fwrite(s->data, 1, SECTOR_SIZE, f);
	fflush(f);
}



/*
	Get the root node.
*/
static sector get_rootnode(){ return load_sector(0); }
/*
	Get the allocation bitmap's information.
	The place on disk where the first block is stored,
	as well as the size of the bitmap in blocks.
*/

static sector s_allocator;


static void get_allocation_bitmap_info(
	uint_dsk* dest_size,
	uint_dsk* dest_where
){
	s_allocator = get_rootnode();
	*dest_size = sector_fetch_size(&s_allocator);
	*dest_where = sector_fetch_dptr(&s_allocator);
	if(*dest_size == 0) {printf("\r\n<ERROR> Allocation bitmap is of zero size!\r\n");			exit(1);}
	if(*dest_where == 0){printf("\r\n<ERROR> Allocation bitmap pointer is NULL!\r\n");			exit(1);}
}
/*
	Find space for a single node. Meant specifically for finding space for fsnodes.

	If the return value is zero, then it didn't find one.

	THIS MUST BE ENTERED UNDER LOCK!!!
*/


static uint_dsk bitmap_find_and_alloc_single_node(

	/*Information attained from a previous call to get_allocation_bitmap_info*/
	const uint_dsk bitmap_size,
	uint_dsk bitmap_where /*HUUUUGE WARNING!!!! WE MODIFY THIS IN THE FUNCTION!!!!*/
){
 	uint_dsk i = 1; /*We do **NOT** start at zero.*/
 	s_allocator = load_sector(bitmap_where);
	for(;i < (bitmap_size * 8);i++){
		unsigned char p; /*before masking.*/
		unsigned char q; /*after masking*/
		/*
			Did we just cross into the next sector?
			If so, load the next sector!
		*/
		if((i/8) % SECTOR_SIZE == 0){
			bitmap_where++;
			s_allocator = load_sector(bitmap_where);
		}
		p = s_allocator.data[ (i%SECTOR_SIZE)/8];
		q = p & (1<< ((i%SECTOR_SIZE)%8));
		if(q == 0) { /*Free slot! Mark it as used.*/
			p = p | (1<< ((i%SECTOR_SIZE)%8));
			store_sector(bitmap_where, &s_allocator);
			return i;
		} 
	}
	return 0; /*Failed allocation.*/
}



/*
	mark a number of nodes as allocated.
*/
static void bitmap_alloc_nodes(
		/*Information attained from a previous call to get_allocation_bitmap_info*/
	const uint_dsk bitmap_size,
	const uint_dsk bitmap_where,
	/*What node do you want to actually allocate?*/
	uint_dsk nodeid_in,
	/*How many nodes do you want to allocate?*/
	uint_dsk nnodes
){
	uint_dsk nodeid = nodeid_in;
	uint_dsk bitmap_offset = 0;

	while(nnodes){
		/*
			Don't attempt to allocate a sector that doesn't exist!
		*/
		if(nodeid > (8 * bitmap_size)) return;
		if(nnodes == 0) return;
		/*
			Generate a block-relative position from nodeid.
		*/
		while(
			nodeid >= (8 * SECTOR_SIZE)
		){
			bitmap_offset++;
			nodeid -= 8 * SECTOR_SIZE;
		}
		s_allocator = load_sector(bitmap_where + bitmap_offset);
		/*
			Optimization- attempt to repeatedly mask elements in the data array.
			We don't want to repeatedly write the disk.
		*/
		node_masker_looptop:
		{
			unsigned char p; /*the mask.*/
			/*calculate the mask for the first nodeid to be marked as used..*/
			p = nodeid % 8;
			p = 1 << p;
			/*nodeid is the id of an actual node (relative to the current sector in the bitmap...) it needs to be divided by 8.
				p refers to the specific bit in the byte.
			*/
			s_allocator.data[nodeid/8] |= p;
			/*
				Can we possibly mask bits for more nodes?
				We want to minimize disk writes.
			*/
			if(
				nnodes > 1 && /*This is not the last node to mark.*/
				(nodeid + 1) < (SECTOR_SIZE*8) /*Its bit resides in the current sector of the bitmap.*/
			){
				nodeid++; 
				nodeid_in++; /*Remember to increment this too!*/
				nnodes--;
				goto node_masker_looptop;
			}
		}
		store_sector(bitmap_where + bitmap_offset, &s_allocator);
		nnodes--; nodeid_in++;
		nodeid = nodeid_in;
	}
	return;	
}

/*
	mark a number of nodes as de-allocated.
*/
static void bitmap_dealloc_nodes(
		/*Information attained from a previous call to get_allocation_bitmap_info*/
	const uint_dsk bitmap_size,
	const uint_dsk bitmap_where,

	/*What node do you want to actually deallocate?*/
	uint_dsk nodeid_in,
	/*How many nodes do you want to deallocate?*/
	uint_dsk nnodes 
){
	uint_dsk nodeid = nodeid_in;
	uint_dsk bitmap_offset = 0;
	
	/*
		Don't attempt to de-allocate a sector that doesn't exist!
	*/
	while(nnodes){
		if(nodeid > (8 * bitmap_size)) return;
		if(nnodes == 0) return;
		/*
			Generate a block-relative position from nodeid.
		*/
		while(
			nodeid >= (8 * SECTOR_SIZE)
		){
			bitmap_offset++;
			nodeid -= 8 * SECTOR_SIZE;
		}
		s_allocator = load_sector(bitmap_where + bitmap_offset);
		/*
			Optimization- attempt to repeatedly mask elements in the data array.
			We don't want to repeatedly write the disk.
		*/
		node_masker_looptop:
		{
			unsigned char p; /*the mask.*/
			/*calculate the mask for the first nodeid to be eliminated.*/
			p = nodeid % 8;
			p = 1 << p;
			p = ~p;
			/*nodeid is the id of an actual node (relative to the current sector in the bitmap...) it needs to be divided by 8.
				p refers to the specific bit in the byte.
			*/
			s_allocator.data[nodeid/8] &= p;
			/*
				Can we possibly mask bits for more nodes?
				We want to minimize disk writes.
			*/
			if(
				nnodes > 1 && /*This is not the last node to mark.*/
				(nodeid + 1) < (SECTOR_SIZE*8) /*Its bit resides in the current sector of the bitmap.*/
			){
				nodeid++; 
				nodeid_in++;
				nnodes--;
				goto node_masker_looptop;
			}
		}
		store_sector(bitmap_where + bitmap_offset, &s_allocator);
		nnodes--; nodeid_in++;
		nodeid = nodeid_in;
	}
	return;	
}



static uint_dsk bitmap_find_and_alloc_multiple_nodes(
	/*Information attained from a previous call to get_allocation_bitmap_info*/
	const uint_dsk bitmap_size,
	const uint_dsk bitmap_where,
	/*How many are actually needed?*/
	uint_dsk needed
){
 	uint_dsk i = bitmap_where + (bitmap_size / SECTOR_SIZE); /*Begin searching the disk beyond the bitmap.*/
 	uint_dsk run = 0;
 	uint_dsk bitmap_offset = i / (8 * SECTOR_SIZE); /*What sector of the bitmap are we searching?*/
 	
 	if(needed == 0) return 0;
 	if(needed == 1) return bitmap_find_and_alloc_single_node(bitmap_size, bitmap_where);
 	s_allocator = load_sector(bitmap_where + bitmap_offset);
	for(;i < (bitmap_size * 8);i++){
		unsigned char p; /*before masking.*/
		unsigned char q; /*after masking*/
		/*
			Did we just cross into the next sector?
			If so, load the next sector!
		*/
		if((i/8) % SECTOR_SIZE == 0){
			bitmap_offset++;
			s_allocator = load_sector(bitmap_offset + bitmap_where);
		}
		p = s_allocator.data[ (i%SECTOR_SIZE)/8];
		q = p & (1<< ((i%SECTOR_SIZE)%8));
		if(q == 0) { /*Free slot! Increment run.*/
			run++;
			if(run >= needed) {
				uint_dsk start = i - (run-1);
				/*
					TODO: Inefficiency, the disk is over-read, and we really should
					allocate nodes from the end backwards, to stay (potentially, on a real hdd) on the same track.
				*/
				bitmap_alloc_nodes(
					bitmap_size,
					bitmap_where, 
					start,
					run
				);
				return start;
			}
		} else run = 0; /*We reached an allocated bit.*/
	}
	return 0; /*Failed allocation.*/
}





/*
	Given a sector, fetch the sector its data pointer is pointing to.
*/
static sector bruh = {0};
static sector get_datasect(sector* sect){
	memset(&bruh, 0, SECTOR_SIZE);
	uint_dsk datapointer = sector_fetch_dptr(sect);
	if(datapointer == 0) return bruh;
	bruh = load_sector(datapointer);
	return bruh;
}
/*
	Now, the right pointer.
*/
static sector get_rightsect(sector* sect){
	memset(&bruh, 0, SECTOR_SIZE);
	uint_dsk datapointer = sector_fetch_rptr(sect);
	if(datapointer == 0) return bruh;
	bruh = load_sector(datapointer);
	return bruh;
}

/*
	Attain lock for modifying the hard disk.

	Whenever we modify the hard disk, we must do it in such a way that a recovery can be done simply
	by re-calculating the bitmap.

	This means that attaching things to the file system is actually the last step-
	the filesystem must remain recoverable simply by recalculating the bitmap.

	This often requires strange orders of events.

	Things listed on the same line can occur in any order.

	To delete a file:
	lock()
	assign next pointer
	deallocate data & deallocate file
	unlock()

	To create a new file:
	lock()
	allocate space in bitmap
	write file node & write file data
	link the new file's right node to the correct right node. 
	link an old file's right node, or a directory's data node.
	unlock()

	To expand an existing file:
	lock()
	allocate space in bitmap if necessary.
	modify size (a crash at this point would cause the file to simply have junk at the end rather than the intended data.)
	write excess bytes
	unlock()

	To shrink an existing file:
	lock()
	modify size
	deallocate in bitmap, if necessary.
	unlock()

	If the shrinking in size will not result in a change in the bitmap, then no lock needs to be attained.

	To re-allocate a file
	lock()
	allocate space in bitmap
	write new file node & write file data (the old one is left alone...)
	link the new file's right node
	link an old file's right node, or a directory's data node.
	deallocate the old file & deallocate the old data.
	unlock()

	To move a file
	
	* create a new file (see creating a new file)
	* delete old file's dptr
	* write new file's dptr to be the old file's dptr
	* 

	To create a directory:
	(Same as creating a file, but data is NULL by default.)

	To delete a directory:
	* Delete all child files first.
	* Perform file deletion.
*/

static sector a_lock, b_lock;
static void lock_modify_bit(){
	a_lock = load_sector(0);
	b_lock = get_datasect(&a_lock);
	b_lock.data[0] |= 1; /*This is the bit usually used for the root node if we think ordinarily*/
	store_sector(sector_fetch_dptr(&a_lock), &b_lock);
}
static void unlock_modify_bit(){
	a_lock = load_sector(0);
	b_lock = get_datasect(&a_lock);
	b_lock.data[0] &= ~1;
	store_sector(sector_fetch_dptr(&a_lock), &b_lock);
}



static sector s_walker;

/*
	Return a pointer to the node in the current directory with target_name name.
*/
static uint_dsk walk_nodes_right(
	uint_dsk start_node_id, 
	const char* target_name
){
	while(1){
		s_walker = load_sector(start_node_id);
		if(start_node_id == 0){ /*We are searching root. Don't match the root node.*/
			start_node_id = sector_fetch_rptr(&s_walker);
			continue;
		}
		if(strcmp(sector_fetch_fname(&s_walker), target_name) == 0) /*Identical!*/
			return start_node_id;
		start_node_id = sector_fetch_rptr(&s_walker);
		if(start_node_id == 0) return 0; /*reached the end of the directory. Bust! Couldn't find it.*/
	}
}

/*
	Follow an absolute path starting at the root.
	The path may never resolve to the root directory node, as it does not exist.
	The path may *only* resolve to a file.
*/
static uint_dsk resolve_path(
	char* path
){
	char* fname;
	while(*path == '/') path++; /*Skip preceding slashes.*/
	if(strlen(path) == 0) return 0; /*Cannot create a directory with no name!*/
	while(
			strlen(path) 
		&& 	path[strlen(path)-1] == '/'
	) path[strlen(path)-1] = '\0';
	if(strlen(path) == 0) return 0; /*Repeat the check. I dont think it's possible for this to ever trigger... but it's here.*/
	pathsan(path);
	if(strlen(path) == 0) return 0; /*Repeat the check.*/
	fname = path;
	while(strfind(fname, "/") != -1) fname += strfind(fname, "/"); /*Skip all slashes.*/
	if(strlen(fname) == 0) return 0; /*Cannot create a directory with no name!*/
	namesan(fname); /*Sanitize the name.*/
	/*Walk the tree. We use the path to do this.*/
	{
		uint_dsk current_node_searching = 0;
		uint_dsk candidate_node = 0;
		while(1){
			long slashloc = strfind(path, "/");
			if(slashloc == 0){
				printf("<INTERNAL ERROR> Failed to sanitize path?");
				exit(1);
			}
			if(slashloc != -1) path[slashloc] = '\0';
			/*From this node, identify nodes to the right.*/
			candidate_node = walk_nodes_right(current_node_searching, path);
			if(slashloc != -1) {
				path[slashloc] = '/';
				path += slashloc + 1; /*Must skip the slash too.*/
				if(candidate_node == 0)	return 0;
				else current_node_searching = candidate_node;
			} else {
				/*Whatever we got, it's it!*/
				return candidate_node;
			}
		}
	}
}

/*
	Does a node exist in the directory?
*/
static char node_exists_in_directory(uint_dsk directory_node_ptr, char* target_name){
	uint_dsk dptr;
	s_allocator = load_sector(directory_node_ptr);
	if(directory_node_ptr) /*SPECIAL CASE- the root node.*/
		dptr = sector_fetch_dptr(&s_allocator);
	else
		dptr = sector_fetch_rptr(&s_allocator);
	if(dptr == 0) return 0; /*Zero entries in the directory.*/
	if(walk_nodes_right(dptr, target_name)) return 1;
	return 0;
}


/*
	Find node in directory, and get it!
*/
static uint_dsk get_node_in_directory(uint_dsk directory_node_ptr, char* target_name){
	uint_dsk dptr;
	uint_dsk r;
	s_allocator = load_sector(directory_node_ptr);
	if(directory_node_ptr) /*SPECIAL CASE- the root node.*/
		dptr = sector_fetch_dptr(&s_allocator);
	else
		dptr = sector_fetch_rptr(&s_allocator);
	if(dptr == 0) return 0; /*Zero entries in the directory.*/
	r = walk_nodes_right(dptr, target_name);
	if(r) return r;
	return 0;
}

static void append_node_right(uint_dsk sibling, uint_dsk newbie){
	s_allocator = load_sector(sibling);
	s_walker = load_sector(newbie);
	/*
		The old rptr of the sibling is the rptr of the new node.
	*/
	sector_write_rptr(
		&s_walker, 
		sector_fetch_rptr(&s_allocator)
	);

	/*
		The guy on the left needs to know!
	*/
	sector_write_rptr(
		&s_allocator, 
		newbie
	);
	store_sector(newbie, &s_walker); /*A crash here will cause an extra file node to exist on the disk which is pointed to by nothing.*/
	store_sector(sibling, &s_allocator); /*A crash here will do nothing*/
}

/*
	Append node to directory.
	MUST BE LOCKED ON ENTRY.
	ALLOCATION BITMAP NOT UPDATED (LOW LEVEL ROUTINE).
*/
static void append_node_to_dir(uint_dsk directory_node_ptr, uint_dsk new_node){
	if(directory_node_ptr == 0){ /*SPECIAL CASE- trying to create a new file in root.*/
		append_node_right(directory_node_ptr, new_node);
		return;
	}
	{
		
		s_allocator = load_sector(directory_node_ptr);
		s_walker = load_sector(new_node);
		/*
			The old dptr of the directory is the rptr of the new node.
		*/
		sector_write_rptr(
			&s_walker, 
			sector_fetch_dptr(&s_allocator)
		);
		/*
			The directory needs a new dptr which points to the new node.
		*/
		sector_write_dptr(
			&s_allocator, 
			new_node
		);
		store_sector(new_node, &s_walker); /*A crash here will cause an extra file node to exist on the disk which is pointed to by nothing.*/
		store_sector(directory_node_ptr, &s_allocator); /*A crash here will do nothing*/
	}
}
/*
	Remove the righthand node.
	MUST BE LOCKED ON ENTRY.
	ALLOCATION BITMAP NOT UPDATED (LOW LEVEL ROUTINE).
*/

static char node_remove_right(uint_dsk sibling){
	uint_dsk deletemeid;
	s_allocator = load_sector(sibling);
	deletemeid = sector_fetch_rptr(&s_allocator);
	if(deletemeid == 0) return 0; /*Don't bother!*/
	s_walker = load_sector(sector_fetch_rptr(&s_allocator));
	/*
		Check if sdeleteme is a directory, with contents.

		A directory with contents cannot be deleted. You must delete all the files in it first.
	*/
	if(
			sector_is_directory(&s_walker)
		&&	sector_fetch_dptr(&s_walker)
	){
		return 0; /*Failure! Cannot delete node- it is a directory with contents.*/
	}
	sector_write_rptr(
		&s_allocator, 
		sector_fetch_rptr(&s_walker)
	);
	store_sector(sibling, &s_allocator);
	return 1;
}

/*
	Remove the first entry in a directory.
	MUST BE LOCKED ON ENTRY.
	ALLOCATION BITMAP NOT UPDATED (LOW LEVEL ROUTINE).
*/
static char node_remove_down(uint_dsk parent){
	uint_dsk deletemeid;
	s_allocator = load_sector(parent);
	if(!sector_is_directory(&s_allocator)) return 0; /*Not a directory... can't do it, sorry.*/
	deletemeid = sector_fetch_dptr(&s_allocator);
	if(deletemeid == 0) return 0; /*We cannot remove a node that does not exist.*/
	s_walker = load_sector(deletemeid);
	/*we need to assign the parent's new dptr.*/
	sector_write_dptr(
		&s_allocator, /*parent node sector*/
		sector_fetch_rptr(&s_walker) /*The entry's righthand pointer.*/
	);
	store_sector(parent, &s_allocator);
	return 1;
}
/*
	Remove a node inside of a directory. Must be entered under lock.
	MUST BE LOCKED ON ENTRY.
	ALLOCATION BITMAP NOT UPDATED (LOW LEVEL ROUTINE).
*/
static char node_remove_from_dir(
	uint_dsk node_dir,
	uint_dsk node_removeme
){
	uint_dsk i, i_prev;
	s_allocator = load_sector(node_dir);
	s_walker = s_allocator;
	if(node_dir == 0){ /*Special case- root node.*/
		i = sector_fetch_rptr(&s_allocator);
	} else {
		i = sector_fetch_dptr(&s_allocator);
	}
	i_prev = 0;
	if(i != 0)
	for(;i != 0;){
		if(i == node_removeme){
			/*REMOVE THIS NODE! it is our target.*/
			if(i_prev == 0 && node_dir == 0){
				/*Remove from rptr of root node.*/
				s_allocator = load_sector(i);
				sector_write_rptr(&s_walker, sector_fetch_rptr(&s_allocator));
				store_sector(node_dir, &s_walker);
			} else if(i_prev == 0){
				/*Reach to our parent- we are the first child.*/
				s_allocator = load_sector(i);
				sector_write_dptr(&s_walker, sector_fetch_rptr(&s_allocator));
				store_sector(node_dir, &s_walker);
			} else {
				/*
					Reach to our right- i_prev is in s_allocator.
					We store our rptr into the previous guy's rptr.
				*/
				s_walker = load_sector(i); /*We don't need */
				sector_write_rptr(&s_allocator, sector_fetch_rptr(&s_walker));
				store_sector(i_prev, &s_allocator);
			}
			return 1;
		}
		i_prev = i;
		s_allocator = load_sector(i);
		i = sector_fetch_rptr(&s_allocator);
	}

	/*fail:*/
	return 0;
}



/*
	API call for creating an empty file or directory.
*/

static char pathbuf[0x10000];
static char namebuf[SECTOR_SIZE - (4 * sizeof(uint_dsk) + 2)];
static sector s_worker;

static char createEmpty(
	const char* path, /*the directory you want to create it in. If you want to create it in root, it MUST be / or a multiple of slashes.*/
	const char* fname,
	ushort permbits /*the permission bits, including whether or not it is a directory*/
){
	uint_dsk directorynode = 0;
	uint_dsk bitmap_size;
	uint_dsk bitmap_where;
	uint_dsk alloced_node = 0;
	pathsan(pathbuf);
	if(strlen(path) == 0) return 0; /*Cannot create a directory with no name!*/
	if(strlen(path) > 65535) return 0; /*Path is too long.*/
	if(strlen(fname) > (SECTOR_SIZE - (4 * sizeof(uint_dsk) + 3)) ) return 0; /*fname is too large. Note the 3 instead of two- it is intentional.*/
	
	my_strcpy(pathbuf, (char*)path);
	my_strcpy(namebuf, (char*)fname);
	namesan(namebuf);
	if(strlen(namebuf) == 0) return 0; /*Cannot create a file with no name!*/

	get_allocation_bitmap_info(&bitmap_size, &bitmap_where);
	if(strcmp(path, "/") != 0)
	{
		directorynode = resolve_path(pathbuf);
		if(directorynode == 0) goto fail;
		/*We need to check if it really is a directory!*/
		s_worker = load_sector(directorynode);
		if(!sector_is_directory(&s_worker)) return 0; /*Not a directory! You can't put files in it!*/
	} else {
		directorynode = 0;
	}
	/*Check if the file already exists.*/
	if(node_exists_in_directory(directorynode, namebuf)) return 0;
	/*Step 1: lock*/
	lock_modify_bit();
		/*Step 2: allocate the node.*/
		alloced_node = bitmap_find_and_alloc_single_node(bitmap_size,bitmap_where);
		if(alloced_node == 0) goto fail; /*Failed allocation.*/
		/*Write the permission bits and whatnot in.*/
		s_worker = load_sector(alloced_node);
		sector_write_fname(&s_worker, namebuf);
		sector_write_dptr(&s_worker, 0);
		sector_write_size(&s_worker, 0);
		sector_write_perm_bits(&s_worker, permbits);
		store_sector(alloced_node, &s_worker);
		/*Step 3: append node to directory! (TODO: REDUNDANT READ AND WRITE HERE...)*/
		append_node_to_dir(directorynode, alloced_node); /*This actually has a special case for the root node.*/
	
	unlock_modify_bit();
	
	return 1;

	fail:
		unlock_modify_bit();
		return 0;
}
/*
	Re-Allocate space for a file.

	(API call)
*/
static char file_realloc(
	const char* path,
	uint_dsk newsize
){
	uint_dsk fsnode;
	uint_dsk bitmap_size;
	uint_dsk bitmap_where;
	uint_dsk old_size;
	uint_dsk new_location;
	uint_dsk size_to_copy; 
	char need_to_copy = 0;

	
	if(strlen(path) == 0) return 0; /*Cannot create a directory with no name!*/
	if(strlen(path) > 65535) return 0; /*Path is too long.*/
	my_strcpy(pathbuf, path);
	fsnode = resolve_path(pathbuf);
	if(fsnode == 0) return 0; /*FAILURE! Does not exist.*/
	s_worker = load_sector(fsnode);
	if(sector_is_directory(&s_worker)) return 0; /*FAILURE! it's a directory, not a file!*/
	if(
		sector_fetch_dptr(&s_worker) && /*Not a NULL file...*/
		(sector_fetch_size(&s_worker) == newsize)
	) return 1; /*Technically, it's already that size.*/

	/*If the new size is zero... We simply free()*/
	if(newsize == 0)
	{
		if(sector_fetch_dptr(&s_worker)){
			uint_dsk nsectors_to_dealloc;
			get_allocation_bitmap_info(&bitmap_size, &bitmap_where);
			lock_modify_bit();
				nsectors_to_dealloc = (sector_fetch_size(&s_worker) + SECTOR_SIZE - 1) / SECTOR_SIZE;
				if(nsectors_to_dealloc == 0) nsectors_to_dealloc++;
				bitmap_dealloc_nodes(
					bitmap_size,
					bitmap_where,
					sector_fetch_dptr(&s_worker),
					nsectors_to_dealloc
				);
				sector_write_size(&s_worker, 0);
				sector_write_dptr(&s_worker, 0);
				store_sector(fsnode, &s_worker);
			unlock_modify_bit();
			return 1;
		} else { /*Newsize is zero, but no dptr?*/
			sector_write_size(&s_worker, 0);
			store_sector(fsnode, &s_worker);
		}
	}
	/*If the new size would require exactly as many sectors as the old size... do nothing!
	*/
	if(sector_fetch_dptr(&s_worker))
	if( 
		((newsize+SECTOR_SIZE-1) / SECTOR_SIZE)
		== 
		((sector_fetch_size(&s_worker)+SECTOR_SIZE-1) / SECTOR_SIZE)
	){
		sector_write_size(&s_worker, newsize);
		store_sector(fsnode, &s_worker);
		return 1;
	}

	old_size = sector_fetch_size(&s_worker);

	if(sector_fetch_dptr(&s_worker) && old_size) {
		need_to_copy = 1; /*Gotta copy over some stuff!*/
		if(newsize > old_size)
			size_to_copy = old_size;
		else
			size_to_copy = newsize;
	}
	get_allocation_bitmap_info(&bitmap_size, &bitmap_where);
	lock_modify_bit();
	/*Step 1: allocate new space.*/
	new_location = bitmap_find_and_alloc_multiple_nodes(
		bitmap_size, 
		bitmap_where,
		(newsize + SECTOR_SIZE - 1) / SECTOR_SIZE
	);
	if(new_location == 0) goto fail;
	/*Step 2: Copy over the data to the new location.*/
	{uint_dsk i = 0;
		for(; i < ( (size_to_copy + SECTOR_SIZE - 1) / SECTOR_SIZE );){
			s_walker = load_sector(sector_fetch_dptr(&s_worker) + i);
			store_sector(new_location + i, &s_walker);
		}
	}
	/*Step 3: Release.*/
	{
		uint_dsk nsectors_to_dealloc;
		lock_modify_bit();
			nsectors_to_dealloc = (sector_fetch_size(&s_worker) + SECTOR_SIZE - 1) / SECTOR_SIZE;
			if(nsectors_to_dealloc == 0) nsectors_to_dealloc++; /*size was 0.*/
			bitmap_dealloc_nodes(
				bitmap_size,
				bitmap_where,
				sector_fetch_dptr(&s_worker),
				nsectors_to_dealloc
			);
		unlock_modify_bit();
	}
	/*Step 4: Relink*/
	sector_write_dptr(&s_worker, new_location);
	store_sector(fsnode, &s_worker);
	unlock_modify_bit();
	return 1;
	fail:
		unlock_modify_bit();
		return 0;
}


static sector s_deleter;

/*
	Delete a file.
	API Call.
	
*/
static char file_delete(
	const char* path_to_directory,
	const char* fname
){
	uint_dsk node = 0;
	uint_dsk node_parentdirectory = 0;
	uint_dsk bitmap_size, bitmap_where;
	if(strlen(path_to_directory) == 0) return 0; /*no name!*/
	if(strlen(path_to_directory) > 65535) return 0; /*Path is too long.*/
	if(strlen(fname) > (SECTOR_SIZE - (4 * sizeof(uint_dsk) + 3)) ) return 0; /*fname is too large. Note the 3 instead of two- it is intentional.*/

	my_strcpy(pathbuf, path_to_directory);pathsan(pathbuf);
	if(strcmp(pathbuf, "/") == 0){ /*root directory.*/
		node_parentdirectory = 0;
	} else {
		node_parentdirectory = resolve_path(pathbuf);
		if(node_parentdirectory == 0) return 0;
	}
	/*Pathbuf is now invalid.*/
	my_strcpy(pathbuf, path_to_directory);		pathsan(pathbuf);
	my_strcpy(namebuf, fname); 					namesan(namebuf);
	/*We now have the parent directory! We must fetch the file from the directory.*/
	node = get_node_in_directory(node_parentdirectory, namebuf);
	if(node_parentdirectory != 0) s_deleter = load_sector(node_parentdirectory);

	node = get_node_in_directory(
		node_parentdirectory, 
		namebuf);
		s_worker = load_sector(node);

	/*For non-directory nodes, delete their contents.*/
	if(
		(!sector_is_directory(&s_worker)) &&
		sector_fetch_dptr(&s_worker)
	){
		my_strcpy(pathbuf, path_to_directory);		pathsan(pathbuf);
		my_strcpy(namebuf, fname); 					namesan(namebuf);
		if(strlen(pathbuf) + strlen(namebuf) > 65534) return 0;
		strcat(pathbuf, "/");
		strcat(pathbuf, namebuf);
		char a = file_realloc(pathbuf, 0);
		if(a == 0) return 0; /*Failure! Couldn't reallocate.*/
	}
	node = get_node_in_directory(
	node_parentdirectory, 
	namebuf);
	s_worker = load_sector(node);
	/*If it has contents now, it cannot be saved.*/
	if( sector_fetch_dptr(&s_worker) )return 0;

	/**/
	get_allocation_bitmap_info(&bitmap_size, &bitmap_where);
	
	lock_modify_bit();
	node_remove_from_dir(
		node_parentdirectory,
		node
	);
	bitmap_dealloc_nodes(
		bitmap_size,
		bitmap_where,
		node,
		1
	);
	unlock_modify_bit();

	return 1;
}



/*
	Code to initialize a disk.

	This must be executed without any power interruption.
*/

static void disk_init(){
	uint_dsk allocation_bitmap_size;
	allocation_bitmap_size = (DISK_SIZE  - SECTOR_OFFSET)/8; /*NUMBER OF BYTES. Not number of sectors.*/
	{
		sector bruh = {0};
		sector_write_perm_bits(&bruh, OWNER_R | PUBLIC_R);
		sector_write_fname(&bruh, "some very long text i am testing");
		sector_write_dptr(&bruh, BITMAP_START);
		sector_write_size(&bruh, allocation_bitmap_size);
		store_sector(0, &bruh);
		lock_modify_bit(); /*After this point, a power failure is safe.*/
			bitmap_alloc_nodes(
					allocation_bitmap_size,
					BITMAP_START,
					BITMAP_START,
					sector_fetch_size_in_sectors(&bruh) /*how many contiguous sectors do we need?*/
				);
		unlock_modify_bit();
	}
	return;
}


int main(int argc, char** argv){
	f = fopen("disk.dsk", "rb+");
	if(!f){
		unsigned long i;
		f = fopen("disk.dsk", "wb");
		if(!f) return 1;
		for(i = 0; i < (DISK_SIZE * SECTOR_SIZE); i++)
			fputc(0, f);
		fclose(f);
		f = fopen("disk.dsk", "rb+");
		disk_init();
	} 
	/*
		A valid filesystem has been loaded. TODO: attempt to parse a command.
	*/
}
