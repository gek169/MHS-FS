/*
	THE MHS FILESYSTEM

	Developed and written by David M.H.S. Webster for the Public Domain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Let all that you do be done with love
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	USAGE

	Include MHS.h into as many files as you like. All the functions are static.

	Interactions with the filesystem should be restricted by your kernel.

	You should modify MHS_UINT to be the unsigned integer of your choice (you probably want a 64 bit uint for a modern OS...)

	

	Implement these functions:

	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		If the disk was being modified during a power shutoff, this should rebuild the bitmap.
	
		Simply walking the entire tree and writing their allocations to the bitmap should work.
	
		the bitmap must be cleared to zero (except for the modify bit) before you do that.

		You should invoke check_modify_bit() before you do any file re-allocations (Any WRITES)

		The filesystem is guaranteed to always be left in a valid state as long as sector writes are atomic,

		however the bitmap may be messed up.
	
	void disk_rebuild_bitmap();
	
	
		Load a sector from the disk.
	
	sector load_sector(MHS_UINT where);
	
	
		Store a sector to the disk.
	
	void store_sector(MHS_UINT where, sector* s);



	Legal Jargon
	~~~~~~~~~~~~~~~~~~~~

	Statement of Purpose
	
	The laws of most jurisdictions throughout the world automatically confer exclusive Copyright and Related Rights (defined below) upon the creator and subsequent owner(s) (each and all, an "owner") of an original work of authorship and/or a database (each, a "Work").
	
	Certain owners wish to permanently relinquish those rights to a Work for the purpose of contributing to a commons of creative, cultural and scientific works ("Commons") that the public can reliably and without fear of later claims of infringement build upon, modify, incorporate in other works, reuse and redistribute as freely as possible in any form whatsoever and for any purposes, including without limitation commercial purposes. These owners may contribute to the Commons to promote the ideal of a free culture and the further production of creative, cultural and scientific works, or to gain reputation or greater distribution for their Work in part through the use and efforts of others.
	
	For these and/or other purposes and motivations, and without any expectation of additional consideration or compensation, the person associating CC0 with a Work (the "Affirmer"), to the extent that he or she is an owner of Copyright and Related Rights in the Work, voluntarily elects to apply CC0 to the Work and publicly distribute the Work under its terms, with knowledge of his or her Copyright and Related Rights in the Work and the meaning and intended legal effect of CC0 on those rights.
	
	1. Copyright and Related Rights. A Work made available under CC0 may be protected by copyright and related or neighboring rights ("Copyright and Related Rights"). Copyright and Related Rights include, but are not limited to, the following:
	
	the right to reproduce, adapt, distribute, perform, display, communicate, and translate a Work;
	moral rights retained by the original author(s) and/or performer(s);
	publicity and privacy rights pertaining to a person's image or likeness depicted in a Work;
	rights protecting against unfair competition in regards to a Work, subject to the limitations in paragraph 4(a), below;
	rights protecting the extraction, dissemination, use and reuse of data in a Work;
	database rights (such as those arising under Directive 96/9/EC of the European Parliament and of the Council of 11 March 1996 on the legal protection of databases, and under any national implementation thereof, including any amended or successor version of such directive); and
	other similar, equivalent or corresponding rights throughout the world based on applicable law or treaty, and any national implementations thereof.
	2. Waiver. To the greatest extent permitted by, but not in contravention of, applicable law, Affirmer hereby overtly, fully, permanently, irrevocably and unconditionally waives, abandons, and surrenders all of Affirmer's Copyright and Related Rights and associated claims and causes of action, whether now known or unknown (including existing as well as future claims and causes of action), in the Work (i) in all territories worldwide, (ii) for the maximum duration provided by applicable law or treaty (including future time extensions), (iii) in any current or future medium and for any number of copies, and (iv) for any purpose whatsoever, including without limitation commercial, advertising or promotional purposes (the "Waiver"). Affirmer makes the Waiver for the benefit of each member of the public at large and to the detriment of Affirmer's heirs and successors, fully intending that such Waiver shall not be subject to revocation, rescission, cancellation, termination, or any other legal or equitable action to disrupt the quiet enjoyment of the Work by the public as contemplated by Affirmer's express Statement of Purpose.
	
	3. Public License Fallback. Should any part of the Waiver for any reason be judged legally invalid or ineffective under applicable law, then the Waiver shall be preserved to the maximum extent permitted taking into account Affirmer's express Statement of Purpose. In addition, to the extent the Waiver is so judged Affirmer hereby grants to each affected person a royalty-free, non transferable, non sublicensable, non exclusive, irrevocable and unconditional license to exercise Affirmer's Copyright and Related Rights in the Work (i) in all territories worldwide, (ii) for the maximum duration provided by applicable law or treaty (including future time extensions), (iii) in any current or future medium and for any number of copies, and (iv) for any purpose whatsoever, including without limitation commercial, advertising or promotional purposes (the "License"). The License shall be deemed effective as of the date CC0 was applied by Affirmer to the Work. Should any part of the License for any reason be judged legally invalid or ineffective under applicable law, such partial invalidity or ineffectiveness shall not invalidate the remainder of the License, and in such case Affirmer hereby affirms that he or she will not (i) exercise any of his or her remaining Copyright and Related Rights in the Work or (ii) assert any associated claims and causes of action with respect to the Work, in either case contrary to Affirmer's express Statement of Purpose.
	
	4. Limitations and Disclaimers.
	
	No trademark or patent rights held by Affirmer are waived, abandoned, surrendered, licensed or otherwise affected by this document.
	Affirmer offers the Work as-is and makes no representations or warranties of any kind concerning the Work, express, implied, statutory or otherwise, including without limitation warranties of title, merchantability, fitness for a particular purpose, non infringement, or the absence of latent or other defects, accuracy, or the present or absence of errors, whether or not discoverable, all to the greatest extent permissible under applicable law.
	Affirmer disclaims responsibility for clearing rights of other persons that may apply to the Work or any use thereof, including without limitation any person's Copyright and Related Rights in the Work. Further, Affirmer disclaims responsibility for obtaining any necessary consents, permissions or other rights required for any use of the Work.
	Affirmer understands and acknowledges that Creative Commons is not a party to this document and has no duty or obligation with respect to this CC0 or use of the Work.

*/



/*
	maximum size of the path.
*/
#define MHS_MAX_PATH_LENGTH 1024




#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void MHS_strcpy(char* dest, const char* src){
	while(*src) *dest++ = *src++;
	*dest = 0;
}
static long MHS_strfind(const char* text, const char* subtext){
	long ti = 0;
	long si = 0;
	long st = strlen(subtext);
	for(;text[ti] != '\0';ti++){
		if(text[ti] == subtext[si]) {
			si++;
			if(subtext[si] == '\0') return (ti - st)+1;
		}else{
			if(subtext[si] == '\0') return (ti - st)+1;
			ti-=si;si = 0;
		}
	}
	return -1;
}

/*
	~~~~~~~~~~~~~~~~~~~~~~~CONFIGURABLES~~~~~~~~~~~~~~~~~~~
*/

#define MHS_NATTRIBS 4

/*Sector size?*/
#define MHS_SECTOR_SIZE 512



#define MHS_IS_DIR 32768
#define MHS_SUID 16384
#define MHS_O_R 8192
#define MHS_O_W 4096
#define MHS_O_X 2048
#define MHS_P_R 1024
#define MHS_P_W 512
#define MHS_P_X 256

#define MHS_G_R 128
#define MHS_G_W 64
#define MHS_G_X 32
#define MHS_GUID 16
/*
#define UNUSED_PERM_1 8
#define UNUSED_PERM_2 4
#define UNUSED_PERM_3 2
#define UNUSED_PERM_4 1
*/

#ifndef MHS_LOG

#ifdef MHS_DEBUG 
#include <stdio.h>
#define MHS_LOG printf
#else
#define MHS_LOG MHS_dummy
static void MHS_dummy(const char* fmt, ...){(void)fmt;return;}
#endif

#endif


typedef unsigned int MHS_UINT; /*32 or 64 bit unsigned integer.*/
typedef unsigned short MHS_USHRT; /*16 bit unsigned int.*/

typedef struct{
	unsigned char data[MHS_SECTOR_SIZE];
} sector;

typedef struct{
	char d[MHS_SECTOR_SIZE - (MHS_NATTRIBS * sizeof(MHS_UINT) + 2)];
} fname_string;


/*
	If the disk was being modified during a power shutoff, this should rebuild the bitmap:

	Simply walking the entire tree and writing their allocations to the bitmap should work.

	the bitmap must be cleared to zero (except for the modify bit) before you do that.
*/
void disk_rebuild_bitmap();

/*
	Load a sector from the disk.
*/
sector load_sector(MHS_UINT where);

/*
	Store a sector to the disk.
*/
void store_sector(MHS_UINT where, sector* s);


static void sector_write_byte(sector* sect, MHS_UINT loc, unsigned char byte){
	loc %= (MHS_SECTOR_SIZE - 1);sect->data[loc] = byte;
}

static unsigned char sector_read_byte(sector* sect, MHS_UINT loc){
	loc %= (MHS_SECTOR_SIZE - 1);return sect->data[loc];
}

static void sector_write_MHS_UINT(sector* sect, MHS_UINT loc, MHS_UINT val){

	sector_write_byte(sect, loc+0, val >> ((sizeof(MHS_UINT) - 1) * 8)    );if(sizeof(MHS_UINT) == 1) return;
	sector_write_byte(sect, loc+1, val >> ((sizeof(MHS_UINT) - 2) * 8)    );if(sizeof(MHS_UINT) == 2) return;	
	sector_write_byte(sect, loc+2, val >> ((sizeof(MHS_UINT) - 3) * 8)    );if(sizeof(MHS_UINT) == 3) return;
	sector_write_byte(sect, loc+3, val >> ((sizeof(MHS_UINT) - 4) * 8)    );if(sizeof(MHS_UINT) == 4) return;

	sector_write_byte(sect, loc+4, val >> ((sizeof(MHS_UINT) - 5) * 8)    );if(sizeof(MHS_UINT) == 5) return;
	sector_write_byte(sect, loc+5, val >> ((sizeof(MHS_UINT) - 6) * 8)    );if(sizeof(MHS_UINT) == 6) return;
	sector_write_byte(sect, loc+6, val >> ((sizeof(MHS_UINT) - 7) * 8)    );if(sizeof(MHS_UINT) == 7) return;
	sector_write_byte(sect, loc+7, val >> ((sizeof(MHS_UINT) - 8) * 8)    );if(sizeof(MHS_UINT) == 8) return;
}

static MHS_UINT sector_read_MHS_UINT(sector* sect, MHS_UINT loc){
	MHS_UINT val = 0;
	MHS_UINT val1 = 0;

	val1 = sector_read_byte(sect, loc + 0); val1 <<= ((sizeof(MHS_UINT) - 1) * 8); val |= val1; if(sizeof(MHS_UINT) == 1) return val;
	val1 = sector_read_byte(sect, loc + 1); val1 <<= ((sizeof(MHS_UINT) - 2) * 8); val |= val1; if(sizeof(MHS_UINT) == 2) return val;
	val1 = sector_read_byte(sect, loc + 2); val1 <<= ((sizeof(MHS_UINT) - 3) * 8); val |= val1; if(sizeof(MHS_UINT) == 3) return val;
	val1 = sector_read_byte(sect, loc + 3); val1 <<= ((sizeof(MHS_UINT) - 4) * 8); val |= val1; if(sizeof(MHS_UINT) == 4) return val;

	val1 = sector_read_byte(sect, loc + 4); val1 <<= ((sizeof(MHS_UINT) - 5) * 8); val |= val1; if(sizeof(MHS_UINT) == 5) return val;
	val1 = sector_read_byte(sect, loc + 5); val1 <<= ((sizeof(MHS_UINT) - 6) * 8); val |= val1; if(sizeof(MHS_UINT) == 6) return val;
	val1 = sector_read_byte(sect, loc + 6); val1 <<= ((sizeof(MHS_UINT) - 7) * 8); val |= val1; if(sizeof(MHS_UINT) == 7) return val;
	val1 = sector_read_byte(sect, loc + 7); val1 <<= ((sizeof(MHS_UINT) - 8) * 8); val |= val1; if(sizeof(MHS_UINT) == 8) return val;
}

static MHS_USHRT sector_fetch_perm_bits(sector* sect){
	MHS_USHRT val = 0;
	val = sector_read_byte(sect, 0);
	val <<= 8;
	val |= sector_read_byte(sect, 1);
	return val;
}

static void sector_write_perm_bits(sector* sect, MHS_USHRT permbits){
	sector_write_byte(sect, 0, permbits / 256);
	sector_write_byte(sect, 1, permbits );
}


static unsigned char sector_is_dir(sector* sect){
	return ((sector_fetch_perm_bits(sect) & MHS_IS_DIR) != 0);
}

static MHS_UINT sector_fetch_ownerid(sector* sect){
	MHS_UINT loc = 0;
	loc+= 2; /*permission bits.*/
	return sector_read_MHS_UINT(sect, loc);
}

static void sector_write_ownerid(sector* sect, MHS_UINT val){
	sector_write_MHS_UINT(sect, 2, val);
}

static MHS_UINT sector_fetch_rptr(sector* sect){
	MHS_UINT loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(MHS_UINT); /*ownerid*/
	return sector_read_MHS_UINT(sect, loc);
}

static void sector_write_rptr(sector* sect, MHS_UINT val){
	MHS_UINT loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(MHS_UINT); /*ownerid*/
	sector_write_MHS_UINT(sect, loc, val);
}

static MHS_UINT sector_fetch_dptr(sector* sect){
	MHS_UINT loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(MHS_UINT); /*ownerid*/
	loc+= sizeof(MHS_UINT); /*rptr*/
	return sector_read_MHS_UINT(sect, loc);
}

static void sector_write_dptr(sector* sect, MHS_UINT val){
	MHS_UINT loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(MHS_UINT); /*ownerid*/
	loc+= sizeof(MHS_UINT); /*rptr*/
	sector_write_MHS_UINT(sect, loc, val);
}


static MHS_UINT sector_fetch_size(sector* sect){
	MHS_UINT loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(MHS_UINT); /*ownerid*/
	loc+= sizeof(MHS_UINT); /*rptr*/
	loc+= sizeof(MHS_UINT); /*dptr*/
	return sector_read_MHS_UINT(sect, loc);
}



static void sector_write_size(sector* sect, MHS_UINT val){
	MHS_UINT loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= sizeof(MHS_UINT); /*ownerid*/
	loc+= sizeof(MHS_UINT); /*rptr*/
	loc+= sizeof(MHS_UINT); /*dptr*/
	sector_write_MHS_UINT(sect, loc, val);
}


static char* sector_fetch_fname(sector* sect){
	MHS_UINT loc = 0;
	loc+= 2; /*permission bits.*/
	loc+= MHS_NATTRIBS * sizeof(MHS_UINT); /*ownerid*/
	if(sect->data[MHS_SECTOR_SIZE - 1]){
		sect->data[MHS_SECTOR_SIZE - 1] = '\0'; /*guarantee null termination. This was a malformed file entry.*/
	}
	return (char*)sect->data + loc;
}

static void namesan(char* name){
	MHS_UINT i = 0; /*Have we started iterating?*/
	if(
		strlen(name) > 
		MHS_SECTOR_SIZE - (
			3 + 
			(MHS_NATTRIBS * sizeof(MHS_UINT))
		)
	)
		name[MHS_SECTOR_SIZE - (
					3 + 
					(MHS_NATTRIBS * sizeof(MHS_UINT))
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

static void pathsan(char* path){
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

static void sector_write_fname(sector* sect, char* newname){
	MHS_UINT loc = 0;
	MHS_UINT i;
	loc+= 2; /*permission bits.*/
	loc+= MHS_NATTRIBS * sizeof(MHS_UINT); /*ownerid*/
	namesan(newname);
	for(i = loc; i < MHS_SECTOR_SIZE-1; i++){
		sect->data[i] = newname[i - loc];
		if(newname[i - loc] == '\0') return;
	}
	sect->data[MHS_SECTOR_SIZE-1] = '\0';
}

/*
	Get the size of a file as a number of sectors.
*/
static MHS_UINT sector_fetch_size_in_sectors(sector* sect){
	MHS_UINT fakes = sector_fetch_size(sect);
	fakes += (MHS_SECTOR_SIZE - 1); /*the classic trick for integer ceil() to a multiple.*/
	fakes /= MHS_SECTOR_SIZE;
	return fakes;
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
	MHS_UINT* dest_size,
	MHS_UINT* dest_where
){
	s_allocator = get_rootnode();
	*dest_size = sector_fetch_size(&s_allocator);
	*dest_where = sector_fetch_dptr(&s_allocator);
	if(*dest_size == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("\r\n<ERROR> Allocation bitmap is of zero size!\r\n");
#endif
		exit(1);
	}
	if(*dest_where == 0){
#ifdef MHS_DEBUG
		MHS_LOG("\r\n<ERROR> Allocation bitmap pointer is NULL!\r\n");
#endif
		exit(1);
	}
}
/*
	Find space for a single node. Meant specifically for finding space for fsnodes.

	If the return value is zero, then it didn't find one.

	THIS MUST BE ENTERED UNDER LOCK!!!
*/


static MHS_UINT bitmap_find_and_alloc_single_node(
	/*Information attained from a previous call to get_allocation_bitmap_info*/
	const MHS_UINT bitmap_size,
	MHS_UINT bitmap_where
){
 	MHS_UINT i = 1; /*We do **NOT** start at zero.*/
 	MHS_UINT bitmap_offset = 0; /*Represents the currently loaded sector.*/
 	s_allocator = load_sector(bitmap_where);
	for(;i < (bitmap_size * 8);i++){
		unsigned char p; /*before masking.*/
		unsigned char q; /*after masking*/
		/*
			Did we just cross into the next sector?
			If so, load the next sector!
		*/
		if(bitmap_offset != i / (8 * MHS_SECTOR_SIZE)){
			bitmap_offset = i / (8 * MHS_SECTOR_SIZE);
			s_allocator = load_sector(bitmap_offset + bitmap_where);
		}
		p = s_allocator.data[ (i%MHS_SECTOR_SIZE)/8];
		q = p & (1<< ((i%MHS_SECTOR_SIZE)%8));
		if(q == 0) { /*Free slot! Mark it as used.*/
			s_allocator.data[ (i%MHS_SECTOR_SIZE)/8] = p | (1<< ((i%MHS_SECTOR_SIZE)%8));
			store_sector(bitmap_where + bitmap_offset, &s_allocator);
			return i;
		}
	}
	return 0; /*Failed allocation.*/
}



static char MHS_is_recovering = 0;
static char MHS_recovering_err_flag = 0;
/*
	mark a number of nodes as allocated.
*/
static void bitmap_alloc_nodes(
	/*Information attained from a previous call to get_allocation_bitmap_info*/
	const MHS_UINT bitmap_size,
	const MHS_UINT bitmap_where,
	/*What node do you want to actually allocate?*/
	MHS_UINT nodeid_in,
	/*How many nodes do you want to allocate?*/
	MHS_UINT nnodes
){
	MHS_UINT nodeid = nodeid_in;
	MHS_UINT bitmap_offset = 0;

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
			nodeid >= (8 * MHS_SECTOR_SIZE)
		){
			bitmap_offset++;
			nodeid -= 8 * MHS_SECTOR_SIZE;
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
			/*
				If we are recovering... we must verify that this marked spot is not already used.
			*/
			if(MHS_is_recovering){
				if(s_allocator.data[nodeid/8] & p){
					MHS_recovering_err_flag = 1;
					return;
				}
			}
			s_allocator.data[nodeid/8] |= p;
			/*
				Can we possibly mask bits for more nodes?
				We want to minimize disk writes.
			*/
			if(
				nnodes > 1 && /*This is not the last node to mark.*/
				(nodeid + 1) < (MHS_SECTOR_SIZE*8) /*Its bit resides in the current sector of the bitmap.*/
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
	const MHS_UINT bitmap_size,
	const MHS_UINT bitmap_where,

	/*What node do you want to actually deallocate?*/
	MHS_UINT nodeid_in,
	/*How many nodes do you want to deallocate?*/
	MHS_UINT nnodes 
){
	MHS_UINT nodeid = nodeid_in;
	MHS_UINT bitmap_offset = 0;
	
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
			nodeid >= (8 * MHS_SECTOR_SIZE)
		){
			bitmap_offset++;
			nodeid -= 8 * MHS_SECTOR_SIZE;
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
				(nodeid + 1) < (MHS_SECTOR_SIZE*8) /*Its bit resides in the current sector of the bitmap.*/
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





static MHS_UINT bitmap_find_and_alloc_multiple_nodes(
	/*Information attained from a previous call to get_allocation_bitmap_info*/
	const MHS_UINT bitmap_size,
	const MHS_UINT bitmap_where,
	/*How many are actually needed?*/
	MHS_UINT needed
){
 	MHS_UINT i = bitmap_where + ((bitmap_size + MHS_SECTOR_SIZE - 1) / MHS_SECTOR_SIZE); /*Begin searching the disk beyond the bitmap.*/
 	MHS_UINT run = 0;
 	MHS_UINT bitmap_offset = i / (8 * MHS_SECTOR_SIZE); /*What sector of the bitmap are we searching?*/
#ifdef MHS_DEBUG
	MHS_LOG("bitmap_find_and_alloc_multiple_nodes: attempting to find %lu nodes starting at %lu\r\n", 
		(unsigned long)needed, (unsigned long)i
	);
#endif
 	if(needed == 0) return 0;
#ifdef MHS_DEBUG
 	if(needed == 1) MHS_LOG("DEBUG: WARNING: size 1???\r\n");
#endif
 	s_allocator = load_sector(bitmap_where + bitmap_offset);
	for(
		i = bitmap_where + ((bitmap_size + MHS_SECTOR_SIZE - 1) / MHS_SECTOR_SIZE);
		i < (bitmap_size * 8);
		i++
	){
		unsigned char p; /*before masking.*/
		unsigned char q; /*after masking*/
		/*
			Did we just cross into the next sector?
			If so, load the next sector!
		*/
		if(bitmap_offset != i / (8 * MHS_SECTOR_SIZE)){
			bitmap_offset = i / (8 * MHS_SECTOR_SIZE);
			s_allocator = load_sector(bitmap_offset + bitmap_where);
		}
		p = s_allocator.data[ (i%(MHS_SECTOR_SIZE * 8))/8];
		q = p & (1<< ((i%(MHS_SECTOR_SIZE * 8))%8)); /*We cannot optimize this line due to the modifiable MHS_SECTOR_SIZE */
		if(q == 0) { /*Free slot! Increment run.*/
			run++;
			if(run >= needed) {
				MHS_UINT start = i - (run-1);
#ifdef MHS_DEBUG
				MHS_LOG("<FOUND SUITABLE LOCATION @ %lu OF LENGTH %lu>\r\n", (unsigned long)i, (unsigned long)run); 
#endif
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
	}
	return 0; /*Failed allocation.*/
}










/*
	Given a sector, fetch the sector its data pointer is pointing to.
*/
static sector bruh_msect = {0};
static sector get_datasect(sector* sect){
	memset(&bruh_msect, 0, MHS_SECTOR_SIZE);
	MHS_UINT datapointer = sector_fetch_dptr(sect);
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
static void check_modify_bit(){
	a_lock = load_sector(0);
	b_lock = get_datasect(&a_lock);
	if(b_lock.data[0] & 1) disk_rebuild_bitmap();
}



static void unlock_modify_bit(){
	a_lock = load_sector(0);
	b_lock = get_datasect(&a_lock);
	b_lock.data[0] &= ~1;
	store_sector(sector_fetch_dptr(&a_lock), &b_lock);
}




static char fsnode_marker(
	MHS_UINT* stacky,
	MHS_UINT stacksize, 
	MHS_UINT bitmap_size,
	MHS_UINT bitmap_where,
	MHS_UINT depth, /*Current stack depth.*/
	MHS_UINT active_node /*This is the node we are responsible for traversing.*/
){
	top:;
	if(depth >= stacksize) return 1; /*Failure!*/
	{
		bitmap_alloc_nodes(
			bitmap_size,
			bitmap_where,
			active_node,
			1
		);
		if(MHS_recovering_err_flag) return 1;

		a_lock = load_sector(active_node);



		if(sector_is_dir(&a_lock)){ /*Directory with contents.*/
			/*Send someone out to walk our children*/
			if(sector_fetch_dptr(&a_lock))
			{
				stacky[depth++] = active_node;
				active_node = sector_fetch_dptr(&a_lock);
				goto top;
				continuation:;
				/*Alternative: use hardware stack.*/
				/*
				if(fsnode_marker(
					stacky, stacksize, 
					bitmap_size, bitmap_where,
					depth + 1,
					sector_fetch_dptr(&a_lock)
				)) return 1;
				*/
			}
			
		} else if(sector_fetch_dptr(&a_lock)) {/*File with contents.*/
			/**/
			MHS_UINT size = sector_fetch_size(&a_lock);
			size += MHS_SECTOR_SIZE - 1;
			size /= MHS_SECTOR_SIZE;
			if(size == 0) size = 1;
			
			bitmap_alloc_nodes(
				bitmap_size,
				bitmap_where,
				sector_fetch_dptr(&a_lock),
				size
			);
			if(MHS_recovering_err_flag) return 1;
		}
		/*go right!*/
		if(sector_fetch_rptr(&a_lock)) {
			active_node = sector_fetch_rptr(&a_lock);
			goto top;
		}
		/*Attempt to pop off the stack.*/
		if(depth){
			depth--;
			active_node = stacky[depth];
			goto continuation;
		}
	}
	return 0;
}

static char bitmap_recover(
	MHS_UINT* stacky,
	MHS_UINT stacksize
){
	MHS_UINT bitmap_where;
	MHS_UINT bitmap_size;
	get_allocation_bitmap_info(&bitmap_size, &bitmap_where);

	/*It should already be...*/
	lock_modify_bit();
	MHS_is_recovering = 1;
	/*Clear the entire bitmap to zero.*/
	bitmap_dealloc_nodes(
		bitmap_size,
		bitmap_where,
		1,
		(bitmap_size * 8 - 1)
	);
	if(fsnode_marker(
		stacky, stacksize, 
		bitmap_size,
		bitmap_where,
		0,  /*Depth of zero.*/
		0 /*Root node.*/
	)){
		if(MHS_recovering_err_flag) return 2;
		return 1;
	}
	MHS_is_recovering = 0;
	return 0;
}


static sector s_walker;
static sector s_seeker;

/*
	Return a pointer to the node in the current directory with target_name name.
*/
static MHS_UINT walk_nodes_right(
	MHS_UINT start_node_id, 
	const char* target_name
){
	
	if(start_node_id == 0){ /*We are searching root. Don't match the root node.*/
		s_walker = load_sector(start_node_id);
		start_node_id = sector_fetch_rptr(&s_walker);
	}
	s_walker = load_sector(start_node_id);
	
	while(1){
#ifdef MHS_DEBUG
		MHS_LOG("<Walking right for %s, id = %lu>\r\n", target_name, (unsigned long)start_node_id);
#endif
		if(start_node_id == 0)
			{
#ifdef MHS_DEBUG
				MHS_LOG("walk_nodes_right: Failed to find entry %s, reached EOD\r\n", target_name);
#endif
				return 0;
			} /*reached the end of the directory. Bust! Couldn't find it.*/
		if(strcmp(sector_fetch_fname(&s_walker), target_name) == 0) /*Identical!*/
			return start_node_id;
		start_node_id = sector_fetch_rptr(&s_walker);
		if(start_node_id == 0) 
			{
#ifdef MHS_DEBUG
				MHS_LOG("walk_nodes_right: Failed to find entry %s\r\n", target_name);
#endif
				return 0;
			} /*reached the end of the directory. Bust! Couldn't find it.*/
		s_walker = load_sector(start_node_id);
	}
}

/*
	Does a node exist in the directory?
*/
static char node_exists_in_directory(MHS_UINT directory_node_ptr, char* target_name){
	MHS_UINT dptr;
	s_walker = load_sector(directory_node_ptr);
	if(directory_node_ptr) /*SPECIAL CASE- the root node.*/
		dptr = sector_fetch_dptr(&s_walker);
	else
		dptr = sector_fetch_rptr(&s_walker);
	if(dptr == 0) return 0; /*Zero entries in the directory.*/
	if(walk_nodes_right(dptr, target_name)) return 1;
#ifdef MHS_DEBUG
	MHS_LOG("node_exists_in_directory: Cannot find entry %s\r\n", target_name);
#endif
	return 0;
}


/*
	Find node in directory, and get it!
*/
static MHS_UINT get_node_in_directory(MHS_UINT directory_node_ptr, char* target_name){
	MHS_UINT dptr;
	MHS_UINT r;
	s_walker = load_sector(directory_node_ptr);
	if(directory_node_ptr)
		dptr = sector_fetch_dptr(&s_walker);
	else /*SPECIAL CASE- the root node.*/
		dptr = sector_fetch_rptr(&s_walker);
	if(dptr == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("get_node_in_directory: Directory has no entries. Cannot find %s\r\n", target_name);
#endif
		return 0;
	} /*Zero entries in the directory.*/
	r = walk_nodes_right(dptr, target_name); /*Walk right for the target name.*/
	if(r == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("get_node_in_directory: Cannot find entry %s\r\n", target_name);
#endif
		return 0;
	}
#ifdef MHS_DEBUG
	MHS_LOG("get_node_in_directory: resolved %s as %lu\r\n", target_name, (unsigned long)r);
#endif
	return r;
}


/*
	Follow an absolute path starting at the root.
	The path may never resolve to the root directory node, as it does not exist.
	The path may *only* resolve to a file.

	API call, requires path be modifiable.
*/
static MHS_UINT resolve_path(
	char* path
){
	char* fname;

	pathsan(path);
	
	while(path[0] == '/') {++path;} /*Skip preceding slashes.*/
	
	if(strlen(path) == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("resolve_path: Empty path (1).\r\n");
#endif
		return 0;
	} /*Cannot create a directory with no name!*/
	
	if(strlen(path) == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("resolve_path: Empty path (2).\r\n");
#endif
		return 0;
	} /*Repeat the check.*/
	fname = path;
	{
		long loc;
		while( (loc = MHS_strfind(fname, "/")) != -1)
		{
			if(loc == 0){
#ifdef MHS_DEBUG
				MHS_LOG("resolve_path: Malformed.\r\n");
#endif
				return 0;
			} /*Repeat the check.*/
			fname += loc+1; /*Skip all slashes.*/

		}
	}	
	if(strlen(fname) == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("resolve_path: Empty fname (1).\r\n");
#endif
		return 0;
	}  /*Cannot create a directory with no name!*/
	namesan(fname); /*Sanitize the name.*/
	/*Walk the tree. We use the path to do this.*/
	{
		MHS_UINT current_node_searching = 0;
		MHS_UINT candidate_node = 0;
		while(1){
			long slashloc = MHS_strfind(path, "/");
			if(slashloc == 0){
#ifdef MHS_DEBUG
				MHS_LOG("<INTERNAL ERROR> Failed to sanitize path?");
#endif
				exit(1);
			}
			if(slashloc != -1) path[slashloc] = '\0';
			/*From this node, identify nodes to the right.*/
#ifdef MHS_DEBUG
			MHS_LOG("DEBUG: resolve_path: resolving %s from node %lu\r\n", path, (unsigned long)current_node_searching);
#endif
			candidate_node = get_node_in_directory(current_node_searching, path);
			if(slashloc != -1) {
				path[slashloc] = '/';
								
				if(candidate_node == 0)	{
#ifdef MHS_DEBUG
					MHS_LOG("resolve_path: Discovered NULL.\r\n");
#endif
					return 0;
				}
				else {
					s_seeker = load_sector(candidate_node);
					if(!sector_is_dir(&s_seeker)){
#ifdef MHS_DEBUG
						MHS_LOG("resolve_path: Expected to go into a file?\r\n");
#endif
						return 0;
					}
					current_node_searching = candidate_node;
#ifdef MHS_DEBUG
					MHS_LOG("DEBUG: resolve_path: resolved %s as %lu\r\n", path, (unsigned long)current_node_searching);
#endif
					path += slashloc + 1; /*Must skip the slash too.*/
					if(current_node_searching == 0) {
#ifdef MHS_DEBUG
					MHS_LOG("resolve_path: Cannot go deeper into node, lacks dptr.\r\n");
#endif
					return 0;}
				}
			} else {
				/*Whatever we got, it's it!*/
				if(candidate_node == 0)
					{
#ifdef MHS_DEBUG
						MHS_LOG("resolve_path: Discovered NULL at end.\r\n");
#endif
						return 0;
					}
				return candidate_node;
			}
		}
	}
}

static sector s_appender;
static sector s_appender2;

static void append_node_right(MHS_UINT sibling, MHS_UINT newbie){
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
static void append_node_to_dir(MHS_UINT directory_node_ptr, MHS_UINT new_node){
	if(directory_node_ptr == 0){ /*SPECIAL CASE- trying to create a new file in root.*/
#ifdef MHS_DEBUG
		MHS_LOG("W: append_node_to_dir: Special case- root node.\r\n");
#endif
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

static char node_remove_right(MHS_UINT sibling){
	MHS_UINT deletemeid;
	s_appender = load_sector(sibling);
	deletemeid = sector_fetch_rptr(&s_appender);
	if(deletemeid == 0) {
#ifdef MHS_DEBUG
	MHS_LOG("node_remove_right: Null rptr.\r\n");
#endif
	return 0;}  /*Don't bother!*/
	s_appender2 = load_sector(deletemeid);
	/*
		Check if sdeleteme is a directory, with contents.

		A directory with contents cannot be deleted. You must delete all the files in it first.
	*/
	if(
			sector_is_dir(&s_appender2)
		&&	sector_fetch_dptr(&s_appender2)
	){
		{
#ifdef MHS_DEBUG
			MHS_LOG("node_remove_right: Cannot remove directory with contents.\r\n");
#endif
			return 0;
		} /*Failure! Cannot delete node- it is a directory with contents.*/
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
static char node_remove_down(MHS_UINT parent){
	MHS_UINT deletemeid;
	s_appender = load_sector(parent);
	if(!sector_is_dir(&s_appender)) {
#ifdef MHS_DEBUG
	MHS_LOG("node_remove_down: Node is not a directory.\r\n");
#endif
	return 0;} /*Not a directory... can't do it, sorry.*/
	deletemeid = sector_fetch_dptr(&s_appender);
	if(deletemeid == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("node_remove_down: Node has null dptr.\r\n");
#endif
		return 0;
	} /*We cannot remove a node that does not exist.*/
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
	MHS_UINT node_dir,
	MHS_UINT node_removeme
){
	MHS_UINT i, i_prev;
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
#ifdef MHS_DEBUG
	MHS_LOG("node_remove_from_dir: Node does not exist in the directory.\r\n");
#endif
	return 0;
}



/*
	API call for creating an empty file or directory.
*/
static char pathbuf[MHS_MAX_PATH_LENGTH];
static char namebuf[MHS_SECTOR_SIZE - (MHS_NATTRIBS * sizeof(MHS_UINT) + 2)];
static sector s_worker;
static sector s_worker2;

static char file_createempty(
	const char* path, /*the directory you want to create it in. If you want to create it in root, it MUST be / or a multiple of slashes.*/
	const char* fname,
	MHS_UINT owner,
	MHS_USHRT permbits /*the permission bits, including whether or not it is a directory*/
){
	MHS_UINT directorynode = 0;
	MHS_UINT bitmap_size;
	MHS_UINT bitmap_where;
	MHS_UINT alloced_node = 0;
	
	if(strlen(path) == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("file_createempty: path empty.\r\n");
#endif
		return 0;
	} /*Cannot create a directory with no name!*/
	if(strlen(path) >(MHS_MAX_PATH_LENGTH - 1)) {
#ifdef MHS_DEBUG
		MHS_LOG("file_createempty: path too long.\r\n");
#endif
		return 0;
	} /*Path is too long.*/
	if(strlen(fname) == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("file_createempty: fname is empty.\r\n");
#endif
		return 0;
	}
	if(strlen(fname) > (MHS_SECTOR_SIZE - (MHS_NATTRIBS * sizeof(MHS_UINT) + 3)) ) {
#ifdef MHS_DEBUG
		MHS_LOG("file_createempty: fname too long.\r\n");
#endif
		return 0;
	} /*fname is too large. Note the 3 instead of two- it is intentional.*/
	
	MHS_strcpy(pathbuf, (char*)path);
	pathsan(pathbuf);
	MHS_strcpy(namebuf, (char*)fname);
	namesan(namebuf);
	if(strlen(namebuf) == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("file_createempty: no name.\r\n");
#endif
		return 0;
	} /*Cannot create a file with no name!*/

	get_allocation_bitmap_info(&bitmap_size, &bitmap_where);
	if(strcmp(path, "/") != 0)
	{
#ifdef MHS_DEBUG
		MHS_LOG("DEBUG: file_createempty: about to resolve path %s\r\n", pathbuf);
#endif
		directorynode = resolve_path(pathbuf);
		if(directorynode == 0) {
#ifdef MHS_DEBUG
			MHS_LOG("file_createempty: directory node failed to resolve.\r\n");
#endif
			return 0;
		}
		/*We need to check if it really is a directory!*/
		s_worker = load_sector(directorynode);
		if(!sector_is_dir(&s_worker)) {
#ifdef MHS_DEBUG
			MHS_LOG("file_createempty: directory node isn't a directory.\r\n");
#endif
			return 0;
		} /*Not a directory! You can't put files in it!*/
	} else {
		directorynode = 0;
	}
#ifdef MHS_DEBUG
	MHS_LOG("file_createempty: about to search if exists\r\n");
#endif
	/*Check if the file already exists.*/
	if(node_exists_in_directory(directorynode, namebuf)) {
#ifdef MHS_DEBUG
		MHS_LOG("file_createempty: file exists in directory.\r\n");
#endif
		return 0;
	}
#ifdef MHS_DEBUG
	MHS_LOG("file_createempty: about to lock\r\n");
#endif
	/*Step 1: lock*/
	lock_modify_bit();
		/*Step 2: allocate the node.*/
		alloced_node = bitmap_find_and_alloc_single_node(bitmap_size,bitmap_where);
		if(alloced_node == 0) {
#ifdef MHS_DEBUG
			MHS_LOG("file_createempty: could not allocate space.\r\n");
#endif
			goto fail;
		} /*Failed allocation.*/
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
	MHS_UINT offset, /*How far into the file? In bytes.*/
	sector* dest
){
	MHS_UINT res;

	if(strlen(path) == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("file_read_sector: path empty.\r\n");
#endif
		return 0;
	} /*Cannot create a directory with no name!*/
	if(strlen(path) > (MHS_MAX_PATH_LENGTH - 1)) {
#ifdef MHS_DEBUG
	MHS_LOG("file_read_sector: path too long.\r\n");
#endif
	return 0;} /*Path is too long.*/
	MHS_strcpy(pathbuf, path);

	res = resolve_path(pathbuf);
	if(res == 0) {
#ifdef MHS_DEBUG
	MHS_LOG("file_read_sector: path fails to resolve.\r\n");
#endif
	return 0;}

	s_worker2 = load_sector(res);

	if(sector_is_dir(&s_worker2)) {
#ifdef MHS_DEBUG
	MHS_LOG("file_read_sector: path points to directory.\r\n");
#endif
	return 0;}/*Cannot read from a directory.*/
	if(sector_fetch_dptr(&s_worker2) == 0) {
#ifdef MHS_DEBUG
	MHS_LOG("file_read_sector: file has NULL dptr.\r\n");
#endif
	return 0;}
	if(sector_fetch_size(&s_worker2) <= offset) {return 0;} /*Too big!*/
	
	*dest = load_sector((offset / MHS_SECTOR_SIZE) + sector_fetch_dptr(&s_worker2));
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
	MHS_UINT res;

	if(strlen(path) == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("file_read_node: path empty.\r\n");
#endif
		return 0;
	} /*Cannot create a directory with no name!*/
	if(strlen(path) > (MHS_MAX_PATH_LENGTH - 1)) {
#ifdef MHS_DEBUG
	MHS_LOG("file_read_node: path too long.\r\n");
#endif
		return 0;
	} /*Path is too long.*/
	MHS_strcpy(pathbuf, path);
	pathsan(pathbuf);
	if(strcmp(pathbuf, "/") != 0)
	{
		res = resolve_path(pathbuf);
		if(res == 0) {
#ifdef MHS_DEBUG
			MHS_LOG("file_read_node: path fails to resolve.\r\n");
#endif
			return 0;
		}
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
	MHS_UINT offset, /*How far into the file? In bytes.*/
	sector* newcontents
){
	MHS_UINT res;

	if(strlen(path) == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("file_write_sector: path empty.\r\n");
#endif
		return 0;
	} /*Cannot create a directory with no name!*/
	if(strlen(path) > (MHS_MAX_PATH_LENGTH - 1)) {
#ifdef MHS_DEBUG
		MHS_LOG("file_write_sector: path too long.\r\n");
#endif
		return 0;
	} /*Path is too long.*/
	MHS_strcpy(pathbuf, path);

	res = resolve_path(pathbuf);
	if(res == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("file_write_sector: path fails to resolve.\r\n");
#endif
		return 0;
	}

	s_worker2 = load_sector(res);

	if(sector_is_dir(&s_worker2)) {
#ifdef MHS_DEBUG
		MHS_LOG("file_write_sector: path points to directory.\r\n");
#endif
		return 0;
	}/*Cannot read from a directory.*/
	if(sector_fetch_dptr(&s_worker2) == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("file_write_sector: file has NULL dptr.\r\n");
#endif
		return 0;
	}
	if(sector_fetch_size(&s_worker2) <= offset) {
#ifdef MHS_DEBUG
		MHS_LOG("file_write_sector: file is too small.\r\n");
#endif
		return 0;
	} /*Too big!*/

#ifdef MHS_DEBUG
	MHS_LOG(
		"DEBUG: offset calculation is %lu, was %lu\r\n", 
		(unsigned long)(offset / MHS_SECTOR_SIZE), 
		(unsigned long)offset
	);
#endif
	store_sector(
		(offset / MHS_SECTOR_SIZE) + sector_fetch_dptr(&s_worker2), newcontents
	); /*Perform storage.*/
	return 1;
}


/*
	Get the Nth directory entry in a directory.

	(API CALL)
*/

static char file_get_dir_entry_by_index(
	const char* path,
	MHS_UINT n,
	char* buf
){
	MHS_UINT res;
	MHS_UINT i;
	if(strlen(path) == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("file_get_dir_entry_by_index: path empty.\r\n");
#endif
		return 0;
	} /*Cannot create a directory with no name!*/
	if(strlen(path) > (MHS_MAX_PATH_LENGTH - 1)) {
#ifdef MHS_DEBUG
		MHS_LOG("file_get_dir_entry_by_index: path too long.\r\n");
#endif
		return 0;
	} /*Path is too long.*/
	MHS_strcpy(pathbuf, path);
	pathsan(pathbuf);
	if(strcmp("/", pathbuf) == 0){
		s_worker2 = load_sector(0);
		res = sector_fetch_rptr(&s_worker2);
	} else {
		res = resolve_path(pathbuf);
		if (res == 0) {return 0;}
		s_worker2 = load_sector(res);
		if(!sector_is_dir(&s_worker2)) {return 0;} /*Trying to list a file like a directory???*/
		res = sector_fetch_dptr(&s_worker2);
	}
	for(i = 0; i < n; i++){
		if(res == 0) {return 0;}
		s_worker2 = load_sector(res);
		res = sector_fetch_rptr(&s_worker2);
	}
	if(res == 0) {return 0;}
	s_worker2 = load_sector(res);
	MHS_strcpy(buf, sector_fetch_fname(&s_worker2));
	return 1;
}

/*
	Re-Allocate space for a file.

	(API call)
*/
static char file_realloc(
	const char* path,
	MHS_UINT newsize
){
	MHS_UINT fsnode;
	MHS_UINT bitmap_size;
	MHS_UINT bitmap_where;
	MHS_UINT old_size;
	MHS_UINT new_location;
	MHS_UINT size_to_copy; 
	char need_to_copy = 0;

	
	if(strlen(path) == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("file_realloc: path_to_directory is empty.\r\n");
#endif
		return 0;
	} /*Cannot create a directory with no name!*/
	if(strlen(path) > (MHS_MAX_PATH_LENGTH - 1)) {
#ifdef MHS_DEBUG
		MHS_LOG("file_realloc: path_to_directory is too long..\r\n");
#endif
		return 0;
	} /*Path is too long.*/
	MHS_strcpy(pathbuf, path);
	fsnode = resolve_path(pathbuf); /*pathbuf is ruined after this.*/
	if(fsnode == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("file_realloc: file failed to resolve.\r\n");
#endif
		return 0;
	} /*FAILURE! Does not exist.*/
	s_worker = load_sector(fsnode);
	if(sector_is_dir(&s_worker)) return 0; /*FAILURE! it's a directory, not a file!*/
	if(
		sector_fetch_dptr(&s_worker) && /*Not a NULL file...*/
		(sector_fetch_size(&s_worker) == newsize)
	) return 1; /*Technically, it's already that size.*/

	/*If the new size is zero... We simply free()*/
	if(newsize == 0)
	{
		if(sector_fetch_dptr(&s_worker)){
			MHS_UINT nsectors_to_dealloc;
			get_allocation_bitmap_info(&bitmap_size, &bitmap_where);
			lock_modify_bit();
				nsectors_to_dealloc = (sector_fetch_size(&s_worker) + MHS_SECTOR_SIZE - 1) / MHS_SECTOR_SIZE;
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
		((newsize+MHS_SECTOR_SIZE-1) / MHS_SECTOR_SIZE)
		== 
		((sector_fetch_size(&s_worker)+MHS_SECTOR_SIZE-1) / MHS_SECTOR_SIZE)
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
			(newsize + MHS_SECTOR_SIZE - 1) / MHS_SECTOR_SIZE
		);
	if(new_location == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("file_realloc: failed to allocate space for file.\r\n");
#endif
		goto fail;
	}
	if(new_location < bitmap_where){
#ifdef MHS_DEBUG
		MHS_LOG("file_realloc: internal error, space allocated in invalid region: %lu.\r\n", (unsigned long)new_location);
#endif
		goto fail;
	}
	/*Step 2: Copy over the data to the new location.*/
	
	if(need_to_copy)
		{MHS_UINT i = 0;
#ifdef MHS_DEBUG
			MHS_LOG("DEBUG: <FILE REALLOC> Copying data...\r\n");
#endif
			for(; i < ( (size_to_copy + MHS_SECTOR_SIZE - 1) / MHS_SECTOR_SIZE ); i++){
				s_walker = load_sector(sector_fetch_dptr(&s_worker) + i);
				store_sector(new_location + i, &s_walker);
			}
#ifdef MHS_DEBUG
			MHS_LOG("DEBUG: <FILE REALLOC> Done Copying data...\r\n");
#endif
		}
	/*Step 3: Release.*/
	
	if(sector_fetch_dptr(&s_worker))
	{
		MHS_UINT nsectors_to_dealloc;
			nsectors_to_dealloc = (sector_fetch_size(&s_worker) + MHS_SECTOR_SIZE - 1) / MHS_SECTOR_SIZE;
			if(nsectors_to_dealloc == 0) {
#ifdef MHS_DEBUG
				MHS_LOG("W: file_realloc: zero size file reached release.\r\n");
#endif
				nsectors_to_dealloc++;
			} /*size was 0.*/
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
	MHS_UINT node = 0;
	MHS_UINT node_parentdirectory = 0;
	MHS_UINT bitmap_size, bitmap_where;
	if(strlen(path_to_directory) == 0) {
#ifdef MHS_DEBUG
		MHS_LOG("file_delete: path_to_directory is empty.\r\n");
#endif
		return 0;
	} /*no name!*/
	if(strlen(path_to_directory) > (MHS_MAX_PATH_LENGTH - 1)) {
#ifdef MHS_DEBUG
		MHS_LOG("file_delete: path_to_directory is too long.\r\n");
#endif
		return 0;
	} /*Path is too long.*/
	if(strlen(fname) > (MHS_SECTOR_SIZE - (MHS_NATTRIBS * sizeof(MHS_UINT) + 3)) ) return 0; /*fname is too large. Note the 3 instead of two- it is intentional.*/

	MHS_strcpy(pathbuf, path_to_directory);pathsan(pathbuf);
	if(strcmp(pathbuf, "/") == 0){ /*root directory.*/
		node_parentdirectory = 0;
	} else {
		node_parentdirectory = resolve_path(pathbuf);
		if(node_parentdirectory == 0) { 
#ifdef MHS_DEBUG
			MHS_LOG("<ERROR> file_delete: Cannot resolve parent directory.\r\n");
#endif
			return 0;
		}
	}
	/*Pathbuf is now invalid.*/
	MHS_strcpy(pathbuf, path_to_directory);		pathsan(pathbuf);
	MHS_strcpy(namebuf, fname); 					namesan(namebuf);
	/*We now have the parent directory! We must fetch the file from the directory.*/
	node = get_node_in_directory(node_parentdirectory, namebuf);
	if(node_parentdirectory != 0) s_deleter = load_sector(node_parentdirectory);

	node = get_node_in_directory(
		node_parentdirectory, 
		namebuf);
		s_deleter2 = load_sector(node);

	/*For non-directory nodes, delete their contents.*/
	if(
		(!sector_is_dir(&s_deleter2)) &&
		sector_fetch_dptr(&s_deleter2)
	){
		MHS_strcpy(pathbuf, path_to_directory);		pathsan(pathbuf);
		MHS_strcpy(namebuf, fname); 					namesan(namebuf);
		if(strlen(pathbuf) + strlen(namebuf) > (MHS_MAX_PATH_LENGTH - 2)) {
#ifdef MHS_DEBUG
			MHS_LOG("<ERROR> file_delete: pathbuf + namebuf too large.\r\n");
#endif
			return 0;
		}
		strcat(pathbuf, "/");
		strcat(pathbuf, namebuf);
		char a = file_realloc(pathbuf, 0);
		if(a == 0) {
#ifdef MHS_DEBUG
			MHS_LOG("<ERROR> file_delete failed while deleting file contents!\r\n");  
#endif
			return 0;
		} /*Failure! Couldn't reallocate.*/
		s_deleter2 = load_sector(node);
	}
	/*If it has contents now, it cannot be freed.*/
	if( sector_fetch_dptr(&s_deleter2) ) {
#ifdef MHS_DEBUG
		MHS_LOG("<ERROR> file_delete: file still has contents.\r\n");
#endif
		return 0;
	}

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
