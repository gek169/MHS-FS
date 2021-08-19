/*
	Example program to demonstrate MHSFS

	jump points (ctrl + f to search for these)
	ALLOWED_FILENAME_CHARACTERS - an if statement that checks the allowed filename characters.
	CONFIGURABLES - compiletime toggles/options for the filesystem driver.
	BITMAP_LOCATION - where the bitmap is stored by default. It can be moved later, of course.

*/
#include "MHS.h"

/*How many sectors are on the disk?*/
#define MHS_DISK_SIZE 0x10000
/*How many sectors to "skip" for some boot code or MBR*/
#define MHS_SECTOR_OFFSET 0

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
	Code to initialize a disk.

	This must be executed without any power interruption.
*/

static void disk_init(){
	MHS_UINT allocation_bitmap_size;
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


char ubuf[65535];
char ubuf2[65535];

static sector usect;

void disk_rebuild_bitmap(){
	printf("<ERROR> Not implemented.");
	exit(1);
}


int main(int argc, char** argv){
	(void)argc;
	(void)argv;
	f = fopen("disk.dsk", "rb+");
	if(!f){
		unsigned long i;
		f = fopen("disk.dsk", "wb");
		if(!f) return 1;
		for(i = 0; i < (MHS_DISK_SIZE * MHS_SECTOR_SIZE); i++)
			fputc(0, f);
		fclose(f);
		f = fopen("disk.dsk", "rb+");
		disk_init();
	}

	check_modify_bit();

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
		MHS_UINT i = 0;
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
					sector_fetch_perm_bits(&usect) & MHS_IS_DIR
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
		MHS_UINT i, len;
		char a;
		FILE* q;
		/*
			Attempt to create it- if it already exists, then it won't do anything.
		*/
		file_createempty(
			argv[2],
			argv[3],
			0,
			MHS_O_R | MHS_O_W | MHS_O_X
		);
		MHS_strcpy(ubuf, argv[2]);
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
		for(i = 0; i < (len + MHS_SECTOR_SIZE - 1) / MHS_SECTOR_SIZE; i++){
			fread(usect.data, 1, MHS_SECTOR_SIZE, q);
			file_write_sector(ubuf, (i * MHS_SECTOR_SIZE), &usect);
		}
		return 0;
	} else if(strcmp(argv[1], "mkdir") == 0){
		file_createempty(
			argv[2],
			argv[3],
			0,
			MHS_O_R | MHS_O_W | MHS_O_X | MHS_IS_DIR
		);
		return 0;
	} else if(strcmp(argv[1], "gt") == 0) {
		FILE* q;
		char a;
		MHS_UINT len;
		MHS_UINT j = 0;
		MHS_strcpy(ubuf, argv[3]);
		q = fopen(argv[2], "wb");
		a = file_read_node(ubuf, &usect);
		if(a == 0) return 1;
		len = sector_fetch_size(&usect);
		if(len == 0) return 1;
		j =sector_fetch_dptr(&usect); 
		if(j == 0) return 1;
		if(sector_fetch_perm_bits(&usect) & MHS_IS_DIR ) return 1;
		for(;len > 0; len -= MHS_SECTOR_SIZE){
			usect = load_sector(j++);
			if(len > MHS_SECTOR_SIZE){
				fwrite(usect.data, MHS_SECTOR_SIZE, 1, q); fflush(q);
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
