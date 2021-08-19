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



/*
	~~~~~~~~~~~~~~~~~~~~~~~CONFIGURABLES~~~~~~~~~~~~~~~~~~~
*/

#define NATTRIBS 4
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
	char d[SECTOR_SIZE - (NATTRIBS * sizeof(uint_dsk) + 2)];
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
	return ((sector_fetch_perm_bits(sect) & IS_DIRECTORY) != 0);
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
	loc+= NATTRIBS * sizeof(uint_dsk); /*ownerid*/
	if(sect->data[SECTOR_SIZE - 1]){
		sect->data[SECTOR_SIZE - 1] = '\0'; /*guarantee null termination. This was a malformed file entry.*/
	}
	return (char*)sect->data + loc;
}

void namesan(char* name){
	uint_dsk i = 0; /*Have we started iterating?*/
	if(
		strlen(name) > 
		SECTOR_SIZE - (
			3 + 
			(NATTRIBS * sizeof(uint_dsk))
		)
	)
		name[SECTOR_SIZE - (
					3 + 
					(NATTRIBS * sizeof(uint_dsk))
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
		name++; i = 1;
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
	while(
			strlen(path) 
		&& 	path[strlen(path)-1] == '/'
	) {
		if(strcmp(path, "/") == 0) return; /*At every step!*/
		path[strlen(path)-1] = '\0'; 
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
	loc+= NATTRIBS * sizeof(uint_dsk); /*ownerid*/
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
		/**/
		if(i > (8 * SECTOR_SIZE))
			if((i/8) % SECTOR_SIZE == 0){
				bitmap_where++;
				s_allocator = load_sector(bitmap_where);
			}
		p = s_allocator.data[ (i%SECTOR_SIZE)/8];
		q = p & (1<< ((i%SECTOR_SIZE)%8));
		if(q == 0) { /*Free slot! Mark it as used.*/
			s_allocator.data[ (i%SECTOR_SIZE)/8] = p | (1<< ((i%SECTOR_SIZE)%8));
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
		bitmap_offset = 0; /*Important.*/
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
		node_masker_looptop:;
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
		bitmap_offset = 0;
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
 	uint_dsk i = bitmap_where + ((bitmap_size + SECTOR_SIZE - 1) / SECTOR_SIZE); /*Begin searching the disk beyond the bitmap.*/
 	uint_dsk run = 0;
 	char have_iterated = 0;
 	uint_dsk bitmap_offset = i / (8 * SECTOR_SIZE); /*What sector of the bitmap are we searching?*/

	/*printf("bitmap_find_and_alloc_multiple_nodes: attempting to find %lu nodes starting at %lu\r\n", (unsigned long)needed, (unsigned long)i);*/
 	
 	if(needed == 0) return 0;
 	/*if(needed == 1) printf("DEBUG: WARNING: size 1???");*/
 	s_allocator = load_sector(bitmap_where + bitmap_offset);
	for(
		i = bitmap_where + ((bitmap_size + SECTOR_SIZE - 1) / SECTOR_SIZE);
		i < (bitmap_size * 8);
		i++
	){
		unsigned char p; /*before masking.*/
		unsigned char q; /*after masking*/
		/*
			Did we just cross into the next sector?
			If so, load the next sector!
		*/
		if(have_iterated && (i/8) % SECTOR_SIZE == 0){
			bitmap_offset++;
			s_allocator = load_sector(bitmap_offset + bitmap_where);
		}
		p = s_allocator.data[ (i%SECTOR_SIZE)/8];
		q = p & (1<< ((i%SECTOR_SIZE)%8));
		if(q == 0) { /*Free slot! Increment run.*/
			run++;
			
			if(run >= needed) {
				uint_dsk start = i - (run-1);
				/*printf("<FOUND SUITABLE LOCATION @ %lu OF LENGTH %lu>\r\n", (unsigned long)i, (unsigned long)run); fflush(stdout);*/
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
		} else run = 0; /*We reached an allocated bit. This is not valid.*/
		have_iterated = 1;
	}
	return 0; /*Failed allocation.*/
}





/*
	Given a sector, fetch the sector its data pointer is pointing to.
*/
static sector bruh_msect = {0};
static sector get_datasect(sector* sect){
	memset(&bruh_msect, 0, SECTOR_SIZE);
	uint_dsk datapointer = sector_fetch_dptr(sect);
	if(datapointer == 0) return bruh_msect;
	bruh_msect = load_sector(datapointer);
	return bruh_msect;
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

/*
	Check the modify bit- was the hard disk being modified?
*/
static char check_modify_bit(){
	a_lock = load_sector(0);
	b_lock = get_datasect(&a_lock);
	return b_lock.data[0] & 1;
}
static void unlock_modify_bit(){
	a_lock = load_sector(0);
	b_lock = get_datasect(&a_lock);
	b_lock.data[0] &= ~1;
	store_sector(sector_fetch_dptr(&a_lock), &b_lock);
}



static sector s_walker;
static sector s_seeker;

/*
	Return a pointer to the node in the current directory with target_name name.
*/
static uint_dsk walk_nodes_right(
	uint_dsk start_node_id, 
	const char* target_name
){
	
	if(start_node_id == 0){ /*We are searching root. Don't match the root node.*/
		s_walker = load_sector(start_node_id);
		start_node_id = sector_fetch_rptr(&s_walker);
	}
	s_walker = load_sector(start_node_id);
	
	while(1){
		/*printf("<Walking right for %s, id = %lu>\r\n", target_name, (unsigned long)start_node_id);fflush(stdout);*/
		if(start_node_id == 0)
			{
				/*printf("walk_nodes_right: Failed to find entry %s, reached EOD\r\n", target_name);*/
				return 0;
			} /*reached the end of the directory. Bust! Couldn't find it.*/
		if(strcmp(sector_fetch_fname(&s_walker), target_name) == 0) /*Identical!*/
			return start_node_id;
		start_node_id = sector_fetch_rptr(&s_walker);
		if(start_node_id == 0) 
			{
				/*printf("walk_nodes_right: Failed to find entry %s\r\n", target_name);*/
				return 0;
			} /*reached the end of the directory. Bust! Couldn't find it.*/
		s_walker = load_sector(start_node_id);
	}
}

/*
	Does a node exist in the directory?
*/
static char node_exists_in_directory(uint_dsk directory_node_ptr, char* target_name){
	uint_dsk dptr;
	s_walker = load_sector(directory_node_ptr);
	if(directory_node_ptr) /*SPECIAL CASE- the root node.*/
		dptr = sector_fetch_dptr(&s_walker);
	else
		dptr = sector_fetch_rptr(&s_walker);
	if(dptr == 0) return 0; /*Zero entries in the directory.*/
	if(walk_nodes_right(dptr, target_name)) return 1;
	/*printf("node_exists_in_directory: Cannot find entry %s\r\n", target_name);*/
	return 0;
}


/*
	Find node in directory, and get it!
*/
static uint_dsk get_node_in_directory(uint_dsk directory_node_ptr, char* target_name){
	uint_dsk dptr;
	uint_dsk r;
	s_walker = load_sector(directory_node_ptr);
	if(directory_node_ptr)
		dptr = sector_fetch_dptr(&s_walker);
	else /*SPECIAL CASE- the root node.*/
		dptr = sector_fetch_rptr(&s_walker);
	if(dptr == 0) {
	/*printf("get_node_in_directory: Directory has no entries. Cannot find %s\r\n", target_name);*/
	return 0;} /*Zero entries in the directory.*/
	r = walk_nodes_right(dptr, target_name); /*Walk right for the target name.*/
	if(r == 0) {
	/*printf("get_node_in_directory: Cannot find entry %s\r\n", target_name);*/
	return 0;}
	/*printf("get_node_in_directory: resolved %s as %lu\r\n", target_name, (unsigned long)r);*/
	return r;
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

	pathsan(path);
	
	while(path[0] == '/') {++path;} /*Skip preceding slashes.*/
	
	if(strlen(path) == 0) {printf("resolve_path: Empty path (1).\r\n");return 0;} /*Cannot create a directory with no name!*/
	
	if(strlen(path) == 0) {printf("resolve_path: Empty path (2).\r\n");return 0;} /*Repeat the check.*/
	fname = path;
	{
		long loc;
		while( (loc = strfind(fname, "/")) != -1)
		{
			if(loc == 0){printf("resolve_path: Malformed.\r\n");return 0;} /*Repeat the check.*/
			fname += loc+1; /*Skip all slashes.*/

		}
	}	
	if(strlen(fname) == 0) {printf("resolve_path: Empty fname (1).\r\n");return 0;}  /*Cannot create a directory with no name!*/
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
			/*printf("DEBUG: resolve_path: resolving %s from node %lu\r\n", path, (unsigned long)current_node_searching);*/
			candidate_node = get_node_in_directory(current_node_searching, path);
			if(slashloc != -1) {
				path[slashloc] = '/';
								
				if(candidate_node == 0)	{
					
					/*printf("resolve_path: Discovered NULL.\r\n");*/
					return 0;
				}
				else {
					s_seeker = load_sector(candidate_node);
					if(!sector_is_directory(&s_seeker)){
					/*printf("resolve_path: Expected to go into a file?\r\n");return 0;*/
					}
					current_node_searching = candidate_node;
					/*printf("DEBUG: resolve_path: resolved %s as %lu\r\n", path, (unsigned long)current_node_searching);*/
					path += slashloc + 1; /*Must skip the slash too.*/
					if(current_node_searching == 0) {printf("resolve_path: Cannot go deeper into node, lacks dptr.\r\n");return 0;}
				}
			} else {
				/*Whatever we got, it's it!*/
				if(candidate_node == 0)
					{
					/*printf("resolve_path: Discovered NULL at end.\r\n");*/
					return 0;}
				return candidate_node;
			}
		}
	}
}

static sector s_appender;
static sector s_appender2;

static void append_node_right(uint_dsk sibling, uint_dsk newbie){
	s_appender = load_sector(sibling);
	s_appender2 = load_sector(newbie);
	/*
		The old rptr of the sibling is the rptr of the new node.
	*/
	sector_write_rptr(
		&s_appender2, 
		sector_fetch_rptr(&s_appender)
	);

	/*
		The guy on the left needs to know!
	*/
	sector_write_rptr(
		&s_appender, 
		newbie
	);
	store_sector(newbie, &s_appender2); /*A crash here will cause an extra file node to exist on the disk which is pointed to by nothing.*/
	store_sector(sibling, &s_appender); /*A crash here will do nothing*/
}

/*
	Append node to directory.
	MUST BE LOCKED ON ENTRY.
	ALLOCATION BITMAP NOT UPDATED (LOW LEVEL ROUTINE).
*/
static void append_node_to_dir(uint_dsk directory_node_ptr, uint_dsk new_node){
	if(directory_node_ptr == 0){ /*SPECIAL CASE- trying to create a new file in root.*/
		/*printf("W: append_node_to_dir: Special case- root node.\r\n");*/
		append_node_right(directory_node_ptr, new_node);
		return;
	}
	{
		s_appender = load_sector(directory_node_ptr);
		s_appender2 = load_sector(new_node);
		/*
			The old dptr of the directory is the rptr of the new node.
		*/
		sector_write_rptr(
			&s_appender2, 
			sector_fetch_dptr(&s_appender)
		);
		/*
			The directory needs a new dptr which points to the new node.
		*/
		sector_write_dptr(
			&s_appender, 
			new_node
		);
		store_sector(new_node, &s_appender2); /*A crash here will cause an extra file node to exist on the disk which is pointed to by nothing.*/
		store_sector(directory_node_ptr, &s_appender); /*A crash here will do nothing*/
	}
}
/*
	Remove the righthand node.
	MUST BE LOCKED ON ENTRY.
	ALLOCATION BITMAP NOT UPDATED (LOW LEVEL ROUTINE).
*/

static char node_remove_right(uint_dsk sibling){
	uint_dsk deletemeid;
	s_appender = load_sector(sibling);
	deletemeid = sector_fetch_rptr(&s_appender);
	if(deletemeid == 0) {printf("node_remove_right: Null rptr.\r\n");return 0;}  /*Don't bother!*/
	s_appender2 = load_sector(deletemeid);
	/*
		Check if sdeleteme is a directory, with contents.

		A directory with contents cannot be deleted. You must delete all the files in it first.
	*/
	if(
			sector_is_directory(&s_appender2)
		&&	sector_fetch_dptr(&s_appender2)
	){
		{printf("node_remove_right: Cannot remove directory with contents.\r\n");return 0;} /*Failure! Cannot delete node- it is a directory with contents.*/
	}
	sector_write_rptr(
		&s_appender, 
		sector_fetch_rptr(&s_appender2)
	);
	store_sector(sibling, &s_appender);
	return 1;
}

/*
	Remove the first entry in a directory.
	MUST BE LOCKED ON ENTRY.
	ALLOCATION BITMAP NOT UPDATED (LOW LEVEL ROUTINE).
*/
static char node_remove_down(uint_dsk parent){
	uint_dsk deletemeid;
	s_appender = load_sector(parent);
	if(!sector_is_directory(&s_appender)) {printf("node_remove_down: Node is not a directory.\r\n");return 0;} /*Not a directory... can't do it, sorry.*/
	deletemeid = sector_fetch_dptr(&s_appender);
	if(deletemeid == 0) {printf("node_remove_down: Node has null dptr.\r\n");return 0;} /*We cannot remove a node that does not exist.*/
	s_appender2 = load_sector(deletemeid);
	/*we need to assign the parent's new dptr.*/
	sector_write_dptr(
		&s_appender, /*parent node sector*/
		sector_fetch_rptr(&s_appender2) /*The entry's righthand pointer.*/
	);
	store_sector(parent, &s_appender);
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
	s_appender = load_sector(node_dir);
	s_appender2 = s_appender;
	if(node_dir == 0){ /*Special case- root node.*/
		i = sector_fetch_rptr(&s_appender);
	} else {
		i = sector_fetch_dptr(&s_appender);
	}
	i_prev = 0;
	if(i != 0)
	for(;i != 0;){
		if(i == node_removeme){
			/*REMOVE THIS NODE! it is our target.*/
			if(i_prev == 0 && node_dir == 0){
				/*Remove from rptr of root node.*/
				s_appender = load_sector(i);
				sector_write_rptr(&s_appender2, sector_fetch_rptr(&s_appender));
				store_sector(node_dir, &s_appender2);
			} else if(i_prev == 0){
				/*Reach to our parent- we are the first child.*/
				s_appender = load_sector(i);
				sector_write_dptr(&s_appender2, sector_fetch_rptr(&s_appender));
				store_sector(node_dir, &s_appender2);
			} else {
				/*
					Reach to our right- i_prev is in s_appender.
					We store our rptr into the previous guy's rptr.
				*/
				s_appender2 = load_sector(i); /*We don't need s_appender2 anymore.*/
				sector_write_rptr(&s_appender, sector_fetch_rptr(&s_appender2));
				store_sector(i_prev, &s_appender);
			}
			return 1;
		}
		i_prev = i;
		s_appender = load_sector(i);
		i = sector_fetch_rptr(&s_appender);
	}

	/*fail:*/
	printf("node_remove_from_dir: Node does not exist in the directory.\r\n");
	return 0;
}



/*
	API call for creating an empty file or directory.
*/

static char pathbuf[0x10000];
static char namebuf[SECTOR_SIZE - (4 * sizeof(uint_dsk) + 2)];
static sector s_worker;
static sector s_worker2;

static char file_createempty(
	const char* path, /*the directory you want to create it in. If you want to create it in root, it MUST be / or a multiple of slashes.*/
	const char* fname,
	uint_dsk owner,
	ushort permbits /*the permission bits, including whether or not it is a directory*/
){
	uint_dsk directorynode = 0;
	uint_dsk bitmap_size;
	uint_dsk bitmap_where;
	uint_dsk alloced_node = 0;
	
	if(strlen(path) == 0) {printf("file_createempty: path empty.\r\n");return 0;} /*Cannot create a directory with no name!*/
	if(strlen(path) > 65535) {printf("file_createempty: path too long.\r\n");return 0;} /*Path is too long.*/
	if(strlen(fname) == 0) {printf("file_createempty: fname is empty.\r\n");return 0;}
	if(strlen(fname) > (SECTOR_SIZE - (NATTRIBS * sizeof(uint_dsk) + 3)) ) {printf("file_createempty: fname too long.\r\n");return 0;} /*fname is too large. Note the 3 instead of two- it is intentional.*/
	
	my_strcpy(pathbuf, (char*)path);
	pathsan(pathbuf);
	my_strcpy(namebuf, (char*)fname);
	namesan(namebuf);
	if(strlen(namebuf) == 0) {printf("file_createempty: no name.\r\n");return 0;} /*Cannot create a file with no name!*/

	get_allocation_bitmap_info(&bitmap_size, &bitmap_where);
	if(strcmp(path, "/") != 0)
	{
		/*printf("DEBUG: file_createempty: about to resolve path %s\r\n", pathbuf);fflush(stdout);*/
		directorynode = resolve_path(pathbuf);
		/*printf("DEBUG: file_createempty: resolved path %s\r\n", pathbuf);fflush(stdout);*/
		if(directorynode == 0) {printf("file_createempty: directory node failed to resolve.\r\n");return 0;}
		/*We need to check if it really is a directory!*/
		s_worker = load_sector(directorynode);
		if(!sector_is_directory(&s_worker)) {printf("file_createempty: directory node isn't a directory.\r\n");return 0;} /*Not a directory! You can't put files in it!*/
	} else {
		directorynode = 0;
	}
	/*printf("file_createempty: about to search if exists\r\n");*/
	/*Check if the file already exists.*/
	if(node_exists_in_directory(directorynode, namebuf)) {printf("file_createempty: file exists in directory.\r\n");return 0;}
	/*printf("file_createempty: about to lock\r\n");*/
	/*Step 1: lock*/
	lock_modify_bit();
		/*Step 2: allocate the node.*/
		alloced_node = bitmap_find_and_alloc_single_node(bitmap_size,bitmap_where);
		if(alloced_node == 0) {printf("file_createempty: could not allocate space.\r\n");goto fail;} /*Failed allocation.*/
		/*Write the permission bits and whatnot in.*/
		s_worker = load_sector(alloced_node);
			sector_write_fname(&s_worker, namebuf);
			sector_write_dptr(&s_worker, 0);
			sector_write_size(&s_worker, 0);
			sector_write_perm_bits(&s_worker, permbits);
			sector_write_ownerid(&s_worker, owner);
		store_sector(alloced_node, &s_worker);
		/*Step 3: append node to directory! (TODO: REDUNDANT READ AND WRITE HERE...)*/
		append_node_to_dir(directorynode, alloced_node); /*This actually has a special case for the root node.*/
	/*Step 4: unlock*/
	unlock_modify_bit();
	
	return 1;

	fail:
		unlock_modify_bit();
		return 0;
}

/*
	Read sector from file

	(API CALL)
*/

static char file_read_sector(
	const char* path,
	uint_dsk offset, /*How far into the file? In bytes.*/
	sector* dest
){
	uint_dsk res;

	if(strlen(path) == 0) {printf("file_read_sector: path empty.\r\n");return 0;} /*Cannot create a directory with no name!*/
	if(strlen(path) > 65535) {printf("file_read_sector: path too long.\r\n");return 0;} /*Path is too long.*/
	my_strcpy(pathbuf, path);

	res = resolve_path(pathbuf);
	if(res == 0) {printf("file_read_sector: path fails to resolve.\r\n");return 0;}

	s_worker2 = load_sector(res);

	if(sector_is_directory(&s_worker2)) {printf("file_read_sector: path points to directory.\r\n");return 0;}/*Cannot read from a directory.*/
	if(sector_fetch_dptr(&s_worker2) == 0) {printf("file_read_sector: file has NULL dptr.\r\n");return 0;}
	if(sector_fetch_size(&s_worker2) <= offset) {return 0;} /*Too big!*/
	
	*dest = load_sector((offset / SECTOR_SIZE) + sector_fetch_dptr(&s_worker2));
	return 1;
}

/*
	Read a file's fsnode.
	API call.
*/

static char file_read_node(
	const char* path,
	sector* dest
){
	uint_dsk res;

	if(strlen(path) == 0) {printf("file_read_node: path empty.\r\n");return 0;} /*Cannot create a directory with no name!*/
	if(strlen(path) > 65535) {printf("file_read_node: path too long.\r\n");return 0;} /*Path is too long.*/
	my_strcpy(pathbuf, path);
	pathsan(pathbuf);
	if(strcmp(pathbuf, "/") != 0)
	{
		res = resolve_path(pathbuf);
		if(res == 0) {printf("file_read_node: path fails to resolve.\r\n");return 0;}
	} else {
		res = 0;
	}
	*dest = load_sector(res);
	return 1;
}

/*
	Write sector of file.

	(API call)
*/

static char file_write_sector(
	const char* path,
	uint_dsk offset, /*How far into the file? In bytes.*/
	sector* newcontents
){
	uint_dsk res;

	if(strlen(path) == 0) {printf("file_write_sector: path empty.\r\n");return 0;} /*Cannot create a directory with no name!*/
	if(strlen(path) > 65535) {printf("file_write_sector: path too long.\r\n");return 0;} /*Path is too long.*/
	my_strcpy(pathbuf, path);

	res = resolve_path(pathbuf);
	if(res == 0) {printf("file_write_sector: path fails to resolve.\r\n");return 0;}

	s_worker2 = load_sector(res);

	if(sector_is_directory(&s_worker2)) {printf("file_write_sector: path points to directory.\r\n");return 0;}/*Cannot read from a directory.*/
	if(sector_fetch_dptr(&s_worker2) == 0) {printf("file_write_sector: file has NULL dptr.\r\n");return 0;}
	if(sector_fetch_size(&s_worker2) <= offset) {printf("file_write_sector: file is too small.\r\n");return 0;} /*Too big!*/

/*
	printf(
		"DEBUG: offset calculation is %lu, was %lu\r\n", 
		(unsigned long)(offset / SECTOR_SIZE), 
		(unsigned long)offset
	);*/
	store_sector(
		(offset / SECTOR_SIZE) + sector_fetch_dptr(&s_worker2), newcontents
	); /*Perform storage.*/
	return 1;
}


/*
	Get the Nth directory entry in a directory.

	(API CALL)
*/

static char file_get_dir_entry_by_index(
	const char* path,
	uint_dsk n,
	char* buf
){
	uint_dsk res;
	uint_dsk i;
	if(strlen(path) == 0) {printf("file_get_dir_entry_by_index: path empty.\r\n");return 0;} /*Cannot create a directory with no name!*/
	if(strlen(path) > 65535) {printf("file_get_dir_entry_by_index: path too long.\r\n");return 0;} /*Path is too long.*/
	my_strcpy(pathbuf, path);
	pathsan(pathbuf);
	if(strcmp("/", pathbuf) == 0){
		s_worker2 = load_sector(0);
		res = sector_fetch_rptr(&s_worker2);
	} else {
		res = resolve_path(pathbuf);
		if (res == 0) {return 0;}
		s_worker2 = load_sector(res);
		if(!sector_is_directory(&s_worker2)) {return 0;} /*Trying to list a file like a directory???*/
		res = sector_fetch_dptr(&s_worker2);
	}
	for(i = 0; i < n; i++){
		if(res == 0) {return 0;}
		s_worker2 = load_sector(res);
		res = sector_fetch_rptr(&s_worker2);
	}
	if(res == 0) {return 0;}
	s_worker2 = load_sector(res);
	my_strcpy(buf, sector_fetch_fname(&s_worker2));
	return 1;
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

	
	if(strlen(path) == 0) {printf("file_realloc: path_to_directory is empty.\r\n");return 0;} /*Cannot create a directory with no name!*/
	if(strlen(path) > 65535) {printf("file_realloc: path_to_directory is too long..\r\n");return 0;} /*Path is too long.*/
	my_strcpy(pathbuf, path);
	fsnode = resolve_path(pathbuf); /*pathbuf is ruined after this.*/
	if(fsnode == 0) {printf("file_realloc: file failed to resolve.\r\n");return 0;} /*FAILURE! Does not exist.*/
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
			sector_write_size(&s_worker, 0); /*It probably was this size?*/
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
		store_sector(fsnode, &s_worker); /*The write is atomic. No need to lock and unlock.*/
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
	new_location = 
		bitmap_find_and_alloc_multiple_nodes(
			bitmap_size, 
			bitmap_where,
			(newsize + SECTOR_SIZE - 1) / SECTOR_SIZE
		);
	if(new_location == 0) {printf("file_realloc: failed to allocate space for file.\r\n");goto fail;}
	if(new_location < bitmap_where){
		printf("file_realloc: internal error, space allocated in invalid region: %lu.\r\n", (unsigned long)new_location);goto fail;
	}
	/*Step 2: Copy over the data to the new location.*/
	
	if(need_to_copy)
		{uint_dsk i = 0;
			/*printf("DEBUG: <FILE REALLOC> Copying data...\r\n");*/
			for(; i < ( (size_to_copy + SECTOR_SIZE - 1) / SECTOR_SIZE ); i++){
				s_walker = load_sector(sector_fetch_dptr(&s_worker) + i);
				store_sector(new_location + i, &s_walker);
			}
			/*printf("DEBUG: <FILE REALLOC> Done Copying data...\r\n");*/
		}
	/*Step 3: Release.*/
	
	if(sector_fetch_dptr(&s_worker))
	{
		uint_dsk nsectors_to_dealloc;
			nsectors_to_dealloc = (sector_fetch_size(&s_worker) + SECTOR_SIZE - 1) / SECTOR_SIZE;
			if(nsectors_to_dealloc == 0) {printf("W: file_realloc: zero size file reached release.\r\n");nsectors_to_dealloc++;} /*size was 0.*/
			bitmap_dealloc_nodes(
				bitmap_size,
				bitmap_where,
				sector_fetch_dptr(&s_worker),
				nsectors_to_dealloc
			);
	}
	/*Step 4: Relink*/
	sector_write_dptr(&s_worker, new_location);
	sector_write_size(&s_worker, newsize);
	store_sector(fsnode, &s_worker);
	unlock_modify_bit();
	return 1;
	fail:
		unlock_modify_bit();
		return 0;
}


static sector s_deleter;
static sector s_deleter2;

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
	if(strlen(path_to_directory) == 0) {printf("file_delete: path_to_directory is empty.\r\n");return 0;} /*no name!*/
	if(strlen(path_to_directory) > 65535) {printf("file_delete: path_to_directory is too long.\r\n");return 0;} /*Path is too long.*/
	if(strlen(fname) > (SECTOR_SIZE - (NATTRIBS * sizeof(uint_dsk) + 3)) ) return 0; /*fname is too large. Note the 3 instead of two- it is intentional.*/

	my_strcpy(pathbuf, path_to_directory);pathsan(pathbuf);
	if(strcmp(pathbuf, "/") == 0){ /*root directory.*/
		node_parentdirectory = 0;
	} else {
		node_parentdirectory = resolve_path(pathbuf);
		if(node_parentdirectory == 0) { printf("<ERROR> file_delete: Cannot resolve parent directory.\r\n");return 0;}
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
		s_deleter2 = load_sector(node);

	/*For non-directory nodes, delete their contents.*/
	if(
		(!sector_is_directory(&s_deleter2)) &&
		sector_fetch_dptr(&s_deleter2)
	){
		my_strcpy(pathbuf, path_to_directory);		pathsan(pathbuf);
		my_strcpy(namebuf, fname); 					namesan(namebuf);
		if(strlen(pathbuf) + strlen(namebuf) > 65534) { printf("<ERROR> file_delete: pathbuf + namebuf too large.\r\n");return 0;}
		strcat(pathbuf, "/");
		strcat(pathbuf, namebuf);
		char a = file_realloc(pathbuf, 0);
		if(a == 0) {printf("<ERROR> file_delete failed while deleting file contents!\r\n");  return 0;} /*Failure! Couldn't reallocate.*/
		s_deleter2 = load_sector(node);
	}
	/*If it has contents now, it cannot be freed.*/
	if( sector_fetch_dptr(&s_deleter2) ) {printf("<ERROR> file_delete: file still has contents.\r\n");return 0;}

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


char ubuf[65535];
char ubuf2[65535];

static sector usect;

int main(int argc, char** argv){
	(void)argc;
	(void)argv;
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

	if(argc < 2){
		printf("USAGE: %s command\r\n", argv[0]);
		printf("command can be any of the following:\r\n");
		printf("  ls pathi              : list the files in the directory at path.\r\n");
		printf("  st pathi name patho   : Store an external file into the VHD, into a file named name.\r\n");
		printf("  gt patho pathi        : Dump a file from the VHD to a file\r\n");
		printf("  mkdir pathi name      : Create a directory at pathi with name name.\r\n");
		printf("  rm dir name           : Delete file or directory name in dir.\r\n");
		return 0;
	}

	if(strcmp(argv[1], "ls") == 0){
		char a;
		uint_dsk i = 0;
		printf("<DIRECTORY LISTING>\r\n");
		for(;;){
			a = file_get_dir_entry_by_index(
				argv[2],
				i,
				ubuf
			);
			if(a) { 
			
				printf(" ENTRY %lu: %s ", (unsigned long)i, ubuf);
				ubuf2[0] = '\0';
				strcat(ubuf2, argv[2]);
				strcat(ubuf2, "/");
				strcat(ubuf2, ubuf);
				
				file_read_node(ubuf2, &usect);
				if(
					sector_fetch_perm_bits(&usect) & IS_DIRECTORY
				)
					printf("DIR\r\n");
				else{
					printf("FILE: %lu\r\n", (unsigned long) sector_fetch_size(&usect));
				}

			}
			else return 0;

			i++;
		}
		return 0;
	}else if(strcmp(argv[1], "rm") == 0){
		file_delete(
			argv[2],
			argv[3]
		);
		return 0;
	} else if(strcmp(argv[1], "st") == 0){
		uint_dsk i, len;
		char a;
		FILE* q;
		/*
			Attempt to create it- if it already exists, then it won't do anything.
		*/
		file_createempty(
			argv[2],
			argv[3],
			0,
			OWNER_R | OWNER_W | OWNER_X
		);
		my_strcpy(ubuf, argv[2]);
		strcat(ubuf, "/");
		strcat(ubuf, argv[3]);

		q = fopen(argv[4], "rb"); if(!q) return 1;

		fseek(q, 0, SEEK_END);
		len = ftell(q);
		fseek(q, 0, SEEK_SET);
		printf("\r\n~~~~~~~~~~~~~~~~Real file %s is size %lu\r\n", argv[4], (unsigned long)len);
		a = file_realloc(
			ubuf,
			len
		);
		if(a == 0) {
			printf("<ERROR> could not do file_realloc.");
			return 1;
		}
		for(i = 0; i < (len + SECTOR_SIZE - 1) / SECTOR_SIZE; i++){
			fread(usect.data, 1, SECTOR_SIZE, q);
			file_write_sector(ubuf, (i * SECTOR_SIZE), &usect);
		}
		return 0;
	} else if(strcmp(argv[1], "mkdir") == 0){
		file_createempty(
			argv[2],
			argv[3],
			0,
			OWNER_R | OWNER_W | OWNER_X | IS_DIRECTORY
		);
		return 0;
	} else if(strcmp(argv[1], "gt") == 0) {
		FILE* q;
		char a;
		uint_dsk len;
		uint_dsk j = 0;
		my_strcpy(ubuf, argv[3]);
		q = fopen(argv[2], "wb");
		a = file_read_node(ubuf, &usect);
		if(a == 0) return 1;
		len = sector_fetch_size(&usect);
		if(len == 0) return 1;
		j =sector_fetch_dptr(&usect); 
		if(j == 0) return 1;
		if(sector_fetch_perm_bits(&usect) & IS_DIRECTORY ) return 1;
		for(;len > 0; len -= SECTOR_SIZE){
			usect = load_sector(j++);
			if(len > SECTOR_SIZE){
				fwrite(usect.data, SECTOR_SIZE, 1, q); fflush(q);
			}else{
				fwrite(usect.data, len, 1, q); fflush(q);
				return 0;
			}
		}
		return 0;
	} else {
		printf("\r\nBad Command\r\n");
		return 1;
	}
	return 1;
}
/*
BBBBB
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAQAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAFAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAZAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAARAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAANAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAA
*/
