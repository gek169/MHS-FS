/*
	Example program to demonstrate MHSFS

	jump points (ctrl + f to search for these)
	ALLOWED_FILENAME_CHARACTERS - an if statement that checks the allowed filename characters.
	CONFIGURABLES - compiletime toggles/options for the filesystem driver.
	BITMAP_LOCATION - where the bitmap is stored by default. It can be moved later, of course.

*/
/*#define MHS_DEBUG*/
#include "MHS.h"
#include <stdio.h>


extern FILE* f; /*the disk.*/




/*
	Code to initialize a disk.

	This must be executed without any power interruption.
*/

extern void disk_init();
void disk_print_bitmap();

char ubuf[65535];
char ubuf2[65535];

static sector usect;

int main(int argc, char** argv){
	(void)argc;
	(void)argv;
	f = fopen("disk.dsk", "rb+");
	if(!f){
		disk_init();
	}

	check_modify_bit();

	if(argc < 2){
		printf("USAGE: %s command\r\n", argv[0]);
		printf("command can be any of the following:\r\n");
		printf("  ls pathi              : list the files in the directory at path.\r\n");
		printf("  st pathi name patho   : Store an external file into the VHD, into a file named name.\r\n");
		printf("  gt patho pathi        : Dump a file from the VHD to a file\r\n");
		printf("  cat pathi             : Dump a file from the VHD to stdout\r\n");
		printf("  mkdir pathi name      : Create a directory at pathi with name name.\r\n");
		printf("  rm dir name           : Delete file or directory name in dir.\r\n");
		printf("  view                  : View the allocation bitmap.\r\n");
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
	}else if(strcmp(argv[1], "view") == 0){
		disk_print_bitmap();
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
	} else if(strcmp(argv[1], "cat") == 0) {
			char a;
			MHS_UINT len;
			MHS_UINT j = 0;
			MHS_strcpy(ubuf, argv[2]);
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
					fwrite(usect.data, MHS_SECTOR_SIZE, 1, stdout); fflush(stdout);
				}else{
					fwrite(usect.data, len, 1, stdout); fflush(stdout);
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
