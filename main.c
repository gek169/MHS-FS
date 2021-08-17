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


void my_strcpy(char* dest, char* src){
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

static sector load_sector(uint_dsk where){
	sector bruh;
	where += SECTOR_OFFSET;
	if(where >= DISK_SIZE){
		printf("<ERROR> Attempted to load sector %lu which is beyond sector bounds.", (unsigned long)where);
	}
	fseek(f, where * SECTOR_SIZE, SEEK_SET);
	fread(bruh.data, 1, SECTOR_SIZE, f);
	return bruh;
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
static void get_allocation_bitmap_info(
	uint_dsk* dest_size,
	uint_dsk* dest_where
){
	sector s = get_rootnode();
	*dest_size = sector_fetch_size(&s);
	*dest_where = sector_fetch_dptr(&s);
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
 	sector s = load_sector(bitmap_where);
	for(;i < (bitmap_size * 8);i++){
		unsigned char p; /*before masking.*/
		unsigned char q; /*after masking*/
		/*
			Did we just cross into the next sector?
			If so, load the next sector!
		*/
		if((i/8) % SECTOR_SIZE == 0){
			bitmap_where++;
			s = load_sector(bitmap_where);
		}
		p = s.data[ (i%SECTOR_SIZE)/8];
		q = p & (1<< ((i%SECTOR_SIZE)%8));
		if(q == 0) { /*Free slot! Mark it as used.*/
			p = p | (1<< ((i%SECTOR_SIZE)%8));
			store_sector(bitmap_where, &s);
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
	sector s;
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
		s = load_sector(bitmap_where + bitmap_offset);
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
			s.data[nodeid/8] |= p;
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
		store_sector(bitmap_where + bitmap_offset, &s);
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
	sector s;
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
		s = load_sector(bitmap_where + bitmap_offset);
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
			s.data[nodeid/8] &= p;
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
		store_sector(bitmap_where + bitmap_offset, &s);
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
 	sector s;
 	if(needed == 0) return 0;
 	if(needed == 1) return bitmap_find_and_alloc_single_node(bitmap_size, bitmap_where);
 	s = load_sector(bitmap_where + bitmap_offset);
	for(;i < (bitmap_size * 8);i++){
		unsigned char p; /*before masking.*/
		unsigned char q; /*after masking*/
		/*
			Did we just cross into the next sector?
			If so, load the next sector!
		*/
		if((i/8) % SECTOR_SIZE == 0){
			bitmap_offset++;
			s = load_sector(bitmap_offset + bitmap_where);
		}
		p = s.data[ (i%SECTOR_SIZE)/8];
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
static sector get_datasect(sector* sect){
	sector bruh = {0};
	uint_dsk datapointer = sector_fetch_dptr(sect);
	if(datapointer == 0) return bruh;
	bruh = load_sector(datapointer);
	return bruh;
}
/*
	Now, the right pointer.
*/
static sector get_rightsect(sector* sect){
	sector bruh = {0};
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
static void lock_modify_bit(){
	sector a,b;
	a = load_sector(0);
	b = get_datasect(&a);
	b.data[0] |= 1; /*This is the bit usually used for the root node if we think ordinarily*/
	store_sector(sector_fetch_dptr(&a), &b);
}

static void unlock_modify_bit(){
	sector a,b;
	a = load_sector(0);
	b = get_datasect(&a);
	b.data[0] &= ~1;
	store_sector(sector_fetch_dptr(&a), &b);
}

/*
	Return a pointer to the node in the current directory with target_name name.
*/
static uint_dsk walk_nodes_right(
	uint_dsk start_node_id, 
	char* target_name
){
	sector s;
	while(1){
		s = load_sector(start_node_id);
		if(start_node_id == 0){ /*We are searching root. Don't match the root node.*/
			start_node_id = sector_fetch_rptr(&s);
			continue;
		}
		if(strcmp(sector_fetch_fname(&s), target_name) == 0) /*Identical!*/
			return start_node_id;
		start_node_id = sector_fetch_rptr(&s);
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
			
			candidate_node = walk_nodes_right(current_node_searching, path);
			if(slashloc != -1) {
				path[slashloc] = '/';
				path += slashloc;
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
	sector s;
	uint_dsk dptr;
	s = load_sector(directory_node_ptr);
	dptr = sector_fetch_dptr(&s);
	if(dptr == 0) return 0; /*Zero entries in the directory.*/
	if(walk_nodes_right(dptr, target_name)) return 1;
	return 0;
}
/*
	Append node to directory.
	MUST BE LOCKED ON ENTRY.
	ALLOCATION BITMAP NOT UPDATED (LOW LEVEL ROUTINE).
*/
static void append_node_to_dir(uint_dsk directory_node_ptr, uint_dsk new_node){
	sector s, s2;
	s = load_sector(directory_node_ptr);
	s2 = load_sector(new_node);
	/*
		The old dptr of the directory is the rptr of the new node.
	*/
	sector_write_rptr(
		&s2, 
		sector_fetch_dptr(&s)
	);
	/*
		The directory needs a new dptr which points to the new node.
	*/
	sector_write_dptr(
		&s, 
		new_node
	);
	store_sector(new_node, &s2); /*A crash here will cause an extra file node to exist on the disk which is pointed to by nothing.*/
	store_sector(directory_node_ptr, &s); /*A crash here will do nothing*/
}
/*
	Remove the righthand node.
	MUST BE LOCKED ON ENTRY.
	ALLOCATION BITMAP NOT UPDATED (LOW LEVEL ROUTINE).
*/

static char node_remove_right(uint_dsk sibling){
	sector s, sdeleteme;
	uint_dsk deletemeid;
	s = load_sector(sibling);
	deletemeid = sector_fetch_rptr(&s);
	if(deletemeid == 0) return 0; /*Don't bother!*/
	sdeleteme = load_sector(sector_fetch_rptr(&s));
	/*
		Check if sdeleteme is a directory, with contents.

		A directory with contents cannot be deleted. You must delete all the files in it first.
	*/
	if(
			sector_is_directory(&sdeleteme)
		&&	sector_fetch_dptr(&sdeleteme)
	){
		return 0; /*Failure! Cannot delete node- it is a directory with contents.*/
	}
	sector_write_rptr(
		&s, 
		sector_fetch_rptr(&sdeleteme)
	);
	store_sector(sibling, &s);
	return 1;
}

/*
	Remove the first entry in a directory.
	MUST BE LOCKED ON ENTRY.
	ALLOCATION BITMAP NOT UPDATED (LOW LEVEL ROUTINE).
*/
static char node_remove_down(uint_dsk parent){
	sector s, sdeleteme;
	uint_dsk deletemeid;
	s = load_sector(parent);
	if(!sector_is_directory(&s)) return 0; /*Not a directory... can't do it, sorry.*/
	deletemeid = sector_fetch_dptr(&s);
	if(deletemeid == 0) return 0; /*We cannot remove a node that does not exist.*/
	sdeleteme = load_sector(deletemeid);
	/*we need to assign the parent's new dptr.*/
	sector_write_dptr(
		&s, /*parent node sector*/
		sector_fetch_rptr(&sdeleteme) /*The entry's righthand pointer.*/
	);
	store_sector(parent, &s);
	return 1;
}
/*
	API call for creating an empty file or directory.

	the path must *NOT* end in a slash.
*/
static char createEmpty(
	char* path,
	ushort permbits /*the permission bits, including whether or not it is a directory*/
){
	if(strlen(path) == 0) return 0; /*Cannot create a directory with no name!*/
	char* fname = path;
	while(strfind(fname, "/") != -1) fname += strfind(fname, "/"); /*Skip all slashes.*/
	if(strlen(fname) == 0) return 0; /*Cannot create a directory with no name!*/
	/*TODO*/
	return 1;
}


/*
	Code to initialize a disk.
*/

static void disk_init(){
	/*BITMAP_LOCATION*/
	uint_dsk allocation_bitmap_size = BITMAP_START;
	allocation_bitmap_size = (DISK_SIZE  - SECTOR_OFFSET)/8; /*NUMBER OF BYTES. Not number of sectors.*/
	{
		sector bruh = {0};
		sector_write_perm_bits(&bruh, OWNER_R | PUBLIC_R);
		sector_write_fname(&bruh, "some very long text i am testing");
		sector_write_dptr(&bruh, BITMAP_START);
		sector_write_size(&bruh, allocation_bitmap_size);
		store_sector(0, &bruh);
	}
	/*BITMAP INITIALIZATION*/
	{
		uint_dsk i = 0;
		uint_dsk j = BITMAP_START;
		sector bruh = {0};
		for(i = 0; i < allocation_bitmap_size; i++){
			unsigned char val = 0;
			/*are we inside of area of the bitmap, which marks the area for the bitmap???*/
			{
				uint_dsk work = i * 8; /*Which bits are we working on, I.E. what sectors are we currently generating a bitmap for?*/
				
				/*
				If any of these are within the allocation bitmap, we must mark them as allocated! The bitmap 
				must reflect its own existence so that the allocator won't allocate overtop of it!
				*/
				/*
				the first sector is used by the root node! Obviously it is taken.
				So it's implicit. We use its bit to store the whether or not the filesystem is in use or not.
				*/
				if(((work + 0) >= BITMAP_START) && ((work + 0) < (BITMAP_START + allocation_bitmap_size)))val |= 1;
				if(((work + 1) >= BITMAP_START) && ((work + 1) < (BITMAP_START + allocation_bitmap_size)))val |= 2;
				if(((work + 2) >= BITMAP_START) && ((work + 2) < (BITMAP_START + allocation_bitmap_size)))val |= 4;
				if(((work + 3) >= BITMAP_START) && ((work + 3) < (BITMAP_START + allocation_bitmap_size)))val |= 8;
				if(((work + 4) >= BITMAP_START) && ((work + 4) < (BITMAP_START + allocation_bitmap_size)))val |= 16;
				if(((work + 5) >= BITMAP_START) && ((work + 5) < (BITMAP_START + allocation_bitmap_size)))val |= 32;
				if(((work + 6) >= BITMAP_START) && ((work + 6) < (BITMAP_START + allocation_bitmap_size)))val |= 64;
				if(((work + 7) >= BITMAP_START) && ((work + 7) < (BITMAP_START + allocation_bitmap_size)))val |= 128;
				bruh.data[i % SECTOR_SIZE] = val;
			}
			if((i % SECTOR_SIZE) == (SECTOR_SIZE - 1)){
				store_sector(j, &bruh); ++j;
				memset(bruh.data, 0, SECTOR_SIZE);
			}
		}
		i--;
		if((i % SECTOR_SIZE) != (SECTOR_SIZE - 1)){ /*The sector wasn't stored! Store the rest of it.*/
			store_sector(j, &bruh); 	  j++;
			memset(bruh.data, 0, SECTOR_SIZE);
		}
	}
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
