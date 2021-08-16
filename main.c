/*
	Example program to demonstrate MHSFS

	jump points (ctrl + f to search for these)
	ALLOWED_FILENAME_CHARACTERS - an if statement that checks the allowed filename characters.
	CONFIGURABLES - compiletime toggles/options for the filesystem driver.
	BITMAP_LOCATION - where the bitmap is stored by default. It can be moved later, of course.

*/




#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>




#include "stringutil.h"

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

void sector_write_fname(sector* sect, char* newname){
	uint_dsk loc = 0;
	uint_dsk i;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(uint_dsk); /*ownerid*/
	loc+= sizeof(uint_dsk); /*rptr*/
	loc+= sizeof(uint_dsk); /*dptr*/
	loc+= sizeof(uint_dsk); /*size*/
	for(i = loc; i < SECTOR_SIZE-1; i++){
	/*Handle the case of a preceding period.*/
		if(i == loc && newname[i - loc] == '.')
			sect->data[loc] = '_';
		else
			sect->data[i] = newname[i - loc];

		/*
			ALLOWED_FILENAME_CHARACTERS
		*/
		if(
			(!isalnum(newname[i - loc])) &&
			(newname[i - loc] != '_') &&
			(newname[i - loc] != ' ') &&
			(newname[i - loc] != '.') &&
			(newname[i - loc] != '!') &&
			(newname[i - loc] != '\0') /*Obviously don't overwrite the null terminator.*/
		)
			sect->data[i] = '_';
		if(newname[i - loc] == '\0') return;
	}
	sect->data[SECTOR_SIZE-1] = '\0';
}


/*
	Load a sector from the disk.
*/

static sector load_sector(uint_dsk where){
	sector bruh;
	where += SECTOR_OFFSET;
	fseek(f, where * SECTOR_SIZE, SEEK_SET);
	fread(bruh.data, 1, SECTOR_SIZE, f);
	return bruh;
}
/*
	Store a sector to the disk.
*/
static void store_sector(uint_dsk where, sector* s){
	where += SECTOR_OFFSET;
	fseek(f, where * SECTOR_SIZE, SEEK_SET);
	fwrite(s->data, 1, SECTOR_SIZE, f);
}



/*
	Get the root node.
*/
static sector get_rootnode(){ return load_sector(0); }
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
	b.data[0] |= 1;
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
	Code to initialize a disk.
*/

static void disk_init(){
	/*BITMAP_LOCATION*/
	uint_dsk allocation_bitmap_size = BITMAP_START;
	allocation_bitmap_size = (DISK_SIZE  - SECTOR_OFFSET)/8;
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
		if((i % SECTOR_SIZE) != (SECTOR_SIZE - 1)){ /*The sector wasn't stored properly.*/
			store_sector(j, &bruh); ++j;
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
