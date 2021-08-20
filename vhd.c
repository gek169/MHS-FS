#include "MHS.h"

#include <stdio.h>

/*How many sectors are on the disk?*/
#define MHS_DISK_SIZE 0x10000
/*How many sectors to "skip" for some boot code or MBR*/
#define MHS_SECTOR_OFFSET 0
#define BITMAP_START 0x20


FILE* f; /*the disk.*/
static sector sector_loader;
sector load_sector(MHS_UINT where){
	where += MHS_SECTOR_OFFSET;
	if(where >= MHS_DISK_SIZE){
		printf("<ERROR> Attempted to load sector %lu which is beyond sector bounds.", (unsigned long)where);
	}
	fseek(f, where * MHS_SECTOR_SIZE, SEEK_SET);
	fread(sector_loader.data, 1, MHS_SECTOR_SIZE, f);
	return sector_loader;
}
/*
	Store a sector to the disk.
*/
void store_sector(MHS_UINT where, sector* s){
	where += MHS_SECTOR_OFFSET;
	if(where >= MHS_DISK_SIZE){
		printf("<ERROR> Attempted to load sector %lu which is beyond sector bounds.", (unsigned long)where);
	}
	fseek(f, where * MHS_SECTOR_SIZE, SEEK_SET);
	fwrite(s->data, 1, MHS_SECTOR_SIZE, f);
	fflush(f);
}


void disk_init(){
	MHS_UINT allocation_bitmap_size;

	{ 	unsigned long i;
		f = fopen("disk.dsk", "wb");
		if(!f) exit(1);
		for(i = 0; i < (MHS_DISK_SIZE * MHS_SECTOR_SIZE); i++)
			fputc(0, f);
		fclose(f);
		f = fopen("disk.dsk", "rb+");
	}
	allocation_bitmap_size = (MHS_DISK_SIZE  - MHS_SECTOR_OFFSET)/8; /*NUMBER OF BYTES. Not number of sectors.*/
	{
		sector bruh = {0};
		sector_write_perm_bits(&bruh, MHS_O_R | MHS_P_R);
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
