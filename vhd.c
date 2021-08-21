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


/*
	Rebuild the bitmap from a power failure.
*/

MHS_UINT disk_traversal_stack[0x10000];
void disk_rebuild_bitmap(){
	printf("<Rebuilding bitmap with max recurse depth of 0x10000, or 64k...\r\n");
	if(bitmap_recover(disk_traversal_stack, 0x10000)){
		printf("<ERROR> Disk recovery failed.\r\n");
		if(MHS_recovering_err_flag)
			printf("<FATAL ERROR> Disk recovery failed. Cannot recover disk.\r\n");
	}
	exit(1);
}


void disk_print_bitmap(){
	MHS_UINT bitmap_size, bitmap_where, i;

	get_allocation_bitmap_info(&bitmap_size, &bitmap_where);

	i = 0;
	printf("\r\n<BITMAP>\r\n");
	for(; i < bitmap_size; i++){ /*For all the bytes in the bitmap...*/
		char a = 0;
		char b = 0;
		if( i % (MHS_SECTOR_SIZE) == 0)
			load_sector(bitmap_where + i / (MHS_SECTOR_SIZE));
		/*Print the byte.*/
		switch(sector_loader.data[i % MHS_SECTOR_SIZE] >> 4){
			case 0: a = '0';break;
			case 1: a = '1';break;
			case 2: a = '2';break;
			case 3: a = '3';break;
			case 4: a = '4';break;
			case 5: a = '5';break;
			case 6: a = '6';break;
			case 7: a = '7';break;
			case 8: a = '8';break;
			case 9: a = '9';break;
			case 10: a = 'a';break;
			case 11: a = 'b';break;
			case 12: a = 'c';break;
			case 13: a = 'd';break;
			case 14: a = 'e';break;
			case 15: a = 'f';break;
		}
		b = a;
		switch(sector_loader.data[i % MHS_SECTOR_SIZE] & 15){
			case 0: a = '0';break;
			case 1: a = '1';break;
			case 2: a = '2';break;
			case 3: a = '3';break;
			case 4: a = '4';break;
			case 5: a = '5';break;
			case 6: a = '6';break;
			case 7: a = '7';break;
			case 8: a = '8';break;
			case 9: a = '9';break;
			case 10: a = 'a';break;
			case 11: a = 'b';break;
			case 12: a = 'c';break;
			case 13: a = 'd';break;
			case 14: a = 'e';break;
			case 15: a = 'f';break;
		}
		printf("%c%c", b, a);
		if( (i & 63) == 63) printf("\r\n");
	}
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
