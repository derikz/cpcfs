
/*					Time-stamp: <98/01/10 14:08:41 derik>
------------------------------------------------------------------------------

	=====
	CPCFS  --  f s . c  --	Manageing the Filesystem
	=====

	Version 0.85		  	(c) February '96 by Derik van Zuetphen
------------------------------------------------------------------------------
*/

#include <sys/stat.h>

#include "cpcfs.h"


/****** Variables ******/

int	cur_trk = -1;		/* flag for no track loaded */
int	cur_hd = 0;
int	cur_blk = -1;


bool tag_ok () {
/*   ^^^^^^ */
	return	(strncmp("MV - CPC",(signed char*)disk_header.tag,8)==0);
}


void alloc_block(int blk,int file) {
/*   ^^^^^^^^^^^
<file> is only informational, real CP/M uses bitmaps */
	if (file >= 0xFF) file = 0xFE;	/* FF marks a free block */
	if (blk > dpb->DSM)
		printm(1,"Warning: Directory entry %d currupt!\n",file);
	else
		blk_alloc[blk]=file;
}


void free_block(int blk) {
/*   ^^^^^^^^^^ */
	if (blk <= dpb->DSM)
		blk_alloc[blk]=0xFF;
}


bool is_free_block(int blk) {
/*   ^^^^^^^^^^^^^ */
	return blk_alloc[blk]==0xFF;
}


void calc_allocation () {
/*   ^^^^^^^^^^^^^^^ */
int	i;
	allocated_blks = 0;
	free_blks = 0;
	for (i=dpb->DBL; i<=dpb->DSM; i++)
		if (is_free_block(i)) free_blks++;
		else allocated_blks++;
	percentage=100*allocated_blks/(float)(allocated_blks+free_blks);
	total_blks = allocated_blks + free_blks;
}


bool inactive () {
/*   ^^^^^^^^ */
	if (*disk_header.tag==0) {
		printm(1,"No image loaded!\n");
		return TRUE;
	} else
		return FALSE;
}


/* Calculate track, sector, and head number of the first sector of block
<blk>.

A quick picture:

		secs
	   .------^-------.
	   _______________
	  / -2- - - - - >/|  } heads
	 /______________/ | }
      / | -1- - - - - > | |
     :	|		| |
     :	|		| |
     :	|		| |
trks =	|  -4- - - - - -| |
     :	|		| |
     :	| -3- - - - - > | |
     :	|		| |
      \ |_______________|/


numbering scheme:
first secs, then heads, then tracks
    %s	       /s%h	  /s/h%t	has to be applied on running sec number

s = sec/trk, h = heads, t = tracks (here infinite)

*/


int trk_calc(int blk) {
/*  ^^^^^^^^ */
	return ((long)blk*dpb->BLS/dpb->BPS + dpb->OFS*dpb->SECS)
		/ (dpb->SECS*dpb->HDS);
}

int sec_calc(int blk) {
/*  ^^^^^^^^
The returned number is not shifted by <sec_offset>! */
	return ((long)blk*dpb->BLS/dpb->BPS + dpb->OFS*dpb->SECS) % dpb->SECS;
}

int hd_calc(int blk) {
/*  ^^^^^^^ */
	return ((long)blk*dpb->BLS/dpb->BPS + dpb->OFS*dpb->SECS)
		/ dpb->SECS % dpb->HDS;
}


int blk_calc(int hd, int trk, int sec) {
/*  ^^^^^^^^
Return the blocknumber of position <hd>, <trk>, <sec> or -1, if it is a
reserved position
*/
	if (trk*dpb->HDS+hd < dpb->OFS) return -1;
	return ((long)sec + hd*dpb->SECS
		+ trk*dpb->SECS*dpb->HDS
		- dpb->OFS*dpb->SECS)
		/ (dpb->BLS/dpb->BPS);
}



void abandonimage() {
/*   ^^^^^^^^^^^^ */
	*disk_header.tag = 0;
	cur_trk = -1;
	if (track)	{free(track); track=NULL;}
	if (blk_alloc)	{free(blk_alloc); blk_alloc=NULL;}
	if (directory)	{free(directory); directory=(DirEntry*)NULL;}
	if (block_buffer){free(block_buffer); block_buffer=(uchar*)NULL;}       
	errorf(FALSE,"Image \"%s\" abandoned!",imagename);
}



/********
  Tracks
 ********/

int read_track (int hd, int trk) {
/*  ^^^^^^^^^^
WARNING: the DPB does not nessecarily exist here! */
long	n, pos;
	if (trk == cur_trk && hd == cur_hd) {
		return 0;
	}

	printm(11,"[rt(%d,%d)] ",hd,trk);

	pos = (long)(trk*disk_header.nbof_heads + hd)
		* (long)disk_header.tracksize;
	n = lseek(imagefile,0x100L+pos,SEEK_SET);
	if (n == -1L) {
		errorf(TRUE,"Image currupt! I cannot position on track %d!",
									trk);
		abandonimage();
		return -1;
	}

	n = read(imagefile,track,disk_header.tracksize);
	if (n != disk_header.tracksize) {
		errorf(TRUE,"Image currupt! I can only read %ld bytes "
					"of track %d (instead of %d bytes)!",
					n,trk,disk_header.tracksize);
		abandonimage();
		return -1;
	}
	cur_trk = trk;
	cur_hd	= hd;

	return 0;
}


int write_track() {
/*  ^^^^^^^^^^^ */
long	n,pos;
	if (cur_trk == -1) return 0;

	printm(11,"[wt(%d,%d)] ",cur_hd,cur_trk);
	pos = (long)(cur_trk*dpb->HDS + cur_hd)	* (long)disk_header.tracksize;
	n = lseek(imagefile,0x100L+pos,SEEK_SET);
	if (n == -1L) {
		errorf(TRUE,"Image currupt! I cannot position on track %d!",
								cur_trk);
		abandonimage();
		return -1;
	}

	n = write(imagefile,track,disk_header.tracksize);
	if (n != disk_header.tracksize) {
		errorf(TRUE,"Something wrong! I cannot write %d bytes "
					"to track %d (only %d bytes written)!",
					disk_header.tracksize,cur_trk,n);
		abandonimage();
		return -1;
	}
	return 0;
}


bool next_sector (int *hd, int *trk, int *sec) {
/*   ^^^^^^^^^^^
Assumes <sec> without offset. Answer TRUE if the <trk> or <hd> is changed. */
	(*sec)++;
	if (*sec >= dpb->SECS) {
		*sec -= dpb->SECS;
		(*hd)++;
		if (*hd >= dpb->HDS) {
			*hd = 0;
			(*trk)++;
		}
		return TRUE;
	}
	return FALSE;
}


int interleave(uchar *track,int sec) {
/* Searches the sector numbers of <track> for the sectorid <sec> and answers
its potition */
int	i;
	for (i=0;i<dpb->SECS;i++) {
		if (((struct t_header*)track)->sector[i].sector
							== sec+dpb->SEC1) {
			return i;
		}
	}
	errorf(FALSE,"Image uses invalid interleave! Sector 0x%X in Track %d "
					"and Head %d",sec,cur_trk,cur_hd);
	abandonimage();
	return 0;
}


uchar *read_block (int blk) {
/*    ^^^^^^^^^^
Read block <blk> into a buffer and return its address, or NULL on error */
int	trk, sec, hd;
int	filled = 0;

	if (blk==cur_blk) return block_buffer;

	printm(11,"[rb(%d)] ",blk);

	trk = trk_calc (blk);
	sec = sec_calc (blk);
	hd  = hd_calc (blk);

	while (filled < dpb->BLS) {
		if (read_track(hd,trk)) return NULL;
		memcpy(block_buffer+filled,
			track+0x100+interleave(track,sec)*dpb->BPS,dpb->BPS);
		filled += dpb->BPS;
		next_sector (&hd,&trk,&sec);
	}
	cur_blk = blk;
	return block_buffer;
}


uchar *write_block (int blk, char *buf) {
/*    ^^^^^^^^^^^
Return the written buffer or NULL on error */
int	trk, sec, hd;
int	filled = 0;

	printm(11,"[wb(%d)] ",blk);

	trk = trk_calc (blk);
	sec = sec_calc (blk);
	hd  = hd_calc (blk);

	while (filled < dpb->BLS) {
		if (read_track(hd,trk)) return NULL;
		memcpy(track+0x100+interleave(track,sec)*dpb->BPS,
							buf+filled,dpb->BPS);
		filled += dpb->BPS;
		if (next_sector (&hd,&trk,&sec)) write_track();
	}
	write_track();
	return buf;
}



/***********
  Directory
 ***********/


/* local definitions for glob_cpm_* */

#define GLOB_ENV_MAX	3
int	glob_env = 0;
	/* determines which global variable-set should <glob_cpm_*> use, */
	/* necessary for nested globs */

uchar	pattern[GLOB_ENV_MAX][INPUTLEN];
int	last_entry[GLOB_ENV_MAX];

int glob_cpm_next() {
/*  ^^^^^^^^^^^^^
Answer the next entry (or -1) with the same pattern as before */
int	i;
uchar	name[20];

	for (i=last_entry[glob_env]+1;i<=dpb->DRM;i++) {
		if (!directory[i].first) continue;
		build_cpm_name_32((signed char*)name, directory[i].user,
				  (signed char*)directory[i].root,
				  (signed char*)directory[i].ext);
		if (match((signed char*)pattern[glob_env],(signed char*)name)) {
			last_entry[glob_env] = i;
			return i;
		}
	}
	return -1;
}


int glob_cpm_file(char *pat) {
/*  ^^^^^^^^^^^^^
Scan the entries for the first file that matches <pattern> and answer its
first (according to <first> flag, see DirEntry) entry number,
If the pattern contains no filename part, *.* is assumed.
Errorcode is -1, if pattern not found.
The work is mostly deferred to <glob_cpm_next>. */

int	user;
uchar	root[INPUTLEN], ext[INPUTLEN];
const char errmsg[] = "Illegal filename \"%s\"";

	if (parse_cpm_filename(pat,&user,root,ext))
		return errorf(FALSE,errmsg,pat);
	upper(root);
	upper(ext);
	if (*root==0) {
		if (user >= 0) {
			strcpy(root,"*");
			strcpy(ext,"*");
		} else {
		return errorf(FALSE,errmsg,pat);

		}
	}
	if (user==-1) user = cur_user;
	build_cpm_name((signed char*)pattern[glob_env], user,
		(signed char*)root, (signed char*)ext);
	last_entry[glob_env] = -1;	/* thus start with 0 */
	return glob_cpm_next();
}


/* local definitions for update_directory */

struct pair {uchar en; uchar ex;};

int cmp_pair(struct pair *x, struct pair *y) {
	if (x->ex < y->ex) return -1;
	if (x->ex > y->ex) return 1;
	return 0;
}

void update_directory() {
/*   ^^^^^^^^^^^^^^^^
(Re-)Organizes the directory structure (Packing the name, making a linked
list) */

int	i, j;

/* a dynamic array of (entry,extent) pairs */
struct pair	*map;


	printm(10,"[ud] ");
	map = (struct pair*)Malloc(sizeof(struct pair)*(dpb->DRM+1));

/****** packing a name of kind "FOO	BAR" to "FOO.BAR\0" ******/
	for (i=0;i<=dpb->DRM;i++) {
		if (directory[i].user == 0xE5) continue;
		build_cpm_name_32((signed char*)directory[i].name, -1,
				  (signed char*)directory[i].root,
				  (signed char*)directory[i].ext);
	}


/****** organizing the directory structure as linked list ******/

/* set entries in the directory to "not visited"
<size> = 0 : never visit; <size> = -1 : not yet visited */
	for (i=0;i<=dpb->DRM;i++) {
		if (directory[i].user == 0xE5) /*NOT <filler>, E5 is required*/
			directory[i].size = 0;	/* never visit empty entries */
		else
			directory[i].size = -1; /* not visited */
		directory[i].first = FALSE;
		directory[i].next  = -1;
	};

/* scan the entries */
	for (i=0;i<=dpb->DRM;i++) {
		if (directory[i].size > -1) continue;

/* reset the map */
		for (j=0;j<=dpb->DRM;j++) {map[j].en=j;map[j].ex=0xFF;}

/* fill the map with <extent> from the directory */
		map[i].ex = directory[i].extent;
		for (j=0;j<=dpb->DRM;j++) {
			if ((directory[j].size == -1) &&
			    (directory[j].user == directory[i].user) &&
			    (i!=j) &&
			    (strcmp((signed char*)directory[i].name,
				    (signed char*)directory[j].name)==0)) {
					map[j].ex = directory[j].extent;
					directory[j].size = 0;
			}
		}
/* sort the map according to map[].ex, not necessary in most cases */
		qsort(map,dpb->DRM+1,sizeof(struct pair),
			(int(*)(const void*,const void*))cmp_pair);

/* fill <first>, <size> and <next> from the map */
		directory[map[0].en].first = TRUE;
		j=1;
		while (map[j].ex < 0xFF) {
			directory[map[j-1].en].next = map[j].en;
			j++;
		}
		directory[map[j-1].en].next = -1;

/* the filesize located in the first fileentry can be calculated from the
<extent> and <rec> fields of the last fileentry */

		directory[map[0].en].size =
			(long)directory[map[j-1].en].extent * EXTENTSIZE
			+ directory[map[j-1].en].rec * RECORDSIZE;

		
	} /* for i */

	free(map); map=NULL;
}


void get_directory() {
/*   ^^^^^^^^^^^^^ */
int	i,j,off;
uchar	*buf;
int	mask;

	printm(10,"[rd] ");
/* reading the directory data */
	for (i=0;i<=dpb->DRM;i++) {
		buf = read_block((signed)i*32/dpb->BLS);
		off = i*32 % dpb->BLS;
/* user, name, ... */
		directory[i].user	= buf[off+0];
		for (j=0;j<8;j++) directory[i].root[j] = buf[off+j+1] & 0x7F;
		for (j=0;j<3;j++) directory[i].ext[j]  = buf[off+j+9] & 0x7F;   
		directory[i].name[0]	= 0;
	        
		directory[i].extent	= buf[off+12];
		directory[i].unused[0]	= buf[off+13];
		directory[i].unused[1]	= buf[off+14];
		directory[i].rec	= buf[off+15];

/* attributes */
		mask=0x1;
		directory[i].attr = 0;
		for (j=11;j>0;j--) {
			if (buf[off+j]&0x80) directory[i].attr |= mask;
			mask <<= 1;
		}

/* block pointer */
		for (j=0;j<16;j++) directory[i].blk[j] = 0;
		if (BLKNR_SIZE==1) {
			for (j=0;j<16;j++) {
				directory[i].blk[j] = buf[off+16+j];
			}
		} else if (BLKNR_SIZE==2) {
			for (j=0;j<8;j++) {
				directory[i].blk[j] = buf[off+16+2*j]
							+ 256 * buf[off+17+2*j];
			}
		}
	}

	update_directory();
	for (i=0;i<dpb->DBL;i++) alloc_block(i,0); /* 0 is not correct! */
        
/* marking the blocks as allocated */
	for (j=0;j<=dpb->DSM;j++) free_block(j);
	for (i=0;i<=dpb->DRM;i++) {
		for (j=0;j<BLKNR;j++) {
			if ((directory[i].user!=0xE5)&&(directory[i].blk[j]))
				alloc_block(directory[i].blk[j],i);
		}
	}
}


void put_directory() {
/*   ^^^^^^^^^^^^^ */
int	i, j, off;
uchar	*buf;
int	mask;
int	block;

	printm(10,"[wd] ");
	buf = block_buffer;	/* simply a shortcut */
        
	block = 0;
	for (i=0;i<=dpb->DRM;i++) {
		off=i*32 % dpb->BLS;

		buf[off] = directory[i].user;
		for (j=0;j<8;j++) buf[off+j+1] = directory[i].root[j];
		for (j=0;j<3;j++) buf[off+j+9] = directory[i].ext[j];   

		buf[off+12] = directory[i].extent;
		buf[off+13] = directory[i].unused[0];
		buf[off+14] = directory[i].unused[1];
		buf[off+15] = directory[i].rec;

		mask=0x1;
		for (j=11;j>0;j--) {
			if (directory[i].attr & mask)
				buf[off+j] |= 0x80;
			mask <<= 1;
		};

		if (BLKNR_SIZE==1) {
			for (j=0;j<16;j++) {
				buf[off+16+j] = directory[i].blk[j];
			}
		} else if (BLKNR_SIZE==2) {
			for (j=0;j<8;j++) {
				buf[off+16+2*j] = directory[i].blk[j] % 256;
				buf[off+17+2*j] = directory[i].blk[j] / 256;
			}
		}

/* if next entry is in the next block, then write the current block */
		if ((i+1)*32/(signed)dpb->BLS > block) {
			write_block(block,buf);
			block++;
		}
	}
}



/*******
  Image
 *******/

void update_dpb(DPB_type *dpb, uchar *track) {
/*   ^^^^^^^^^^
Determine the extended DPB data out of <dpb> and the sample track <track>.
Complete the extension parts of <dpb>. <track> must be read in first!
*/
	dpb->BLS  = 1 << (dpb->BSH + 7); /* or 2^BSH*128 */

/* an image must exist, do not call form <format>! */
	/* dpb->SEC1 = keep from DPB template */
	dpb->SECS = ((struct t_header*)track)->SPT;
/* the next two elements should already be set */
	dpb->TRKS = disk_header.nbof_tracks;
	dpb->HDS  = disk_header.nbof_heads;

	dpb->SYS  = (dpb->OFS>0) && (*(track+0x100)) != filler;
	dpb->DBL  = 32 * (dpb->DRM+1) / dpb->BLS; /* or often CKS/8 */

	dpb->DSM = (dpb->TRKS*dpb->HDS*dpb->SECS) / (dpb->BLS/dpb->BPS) - 1;
/* subtract reserved tracks */
	dpb->DSM -= dpb->OFS * dpb->SECS / (dpb->BLS/dpb->BPS); 
/* subtract one for a slack block, because neither 18/8 nor 9/2 is integral: */
	dpb->DSM--;	
	
	if (dpb->DSM>=255) {
/* 2 byte pointer and 8 pointer per entry */
		BLKNR_SIZE = 2;
		BLKNR = 8;
	} else {
/* 1 byte pointer and 16 pointer per entry */
		BLKNR_SIZE = 1;
		BLKNR = 16;
	}
}       


void close_image() {
/*   ^^^^^^^^^^^ */
	if (*disk_header.tag) {
		printm(10,"[ci] ");
		if (cur_trk > -1) write_track();
		put_directory();
		free(blk_alloc);	blk_alloc=NULL;
		free(track);		track=NULL;
		free(directory);	directory=(DirEntry*)NULL;
		free(block_buffer);	block_buffer=(uchar*)NULL;	        
		*disk_header.tag = 0;
		dpb = NULL;
		close(imagefile);
	}
	cur_trk = -1;
	cur_blk = -1;
}


int open_image(char *name) {
/*  ^^^^^^^^^^
alloc track buffer, blk_alloc, buffer, read directory */

int	n;
char	dirsep[2] = {DIRSEPARATOR, 0};
char	*p;

	if (*disk_header.tag) close_image();
/* open file */
	printm(10,"[oi] ");
	imagefile = open(name,O_RDWR|O_BINARY,0);
	imagename = name;	/* temporary, for <abandonimage> */
	if (imagefile < 0) {
		return errorf(TRUE,"Cannot open \"%s\"",name);
	}
	n = read(imagefile,&disk_header,0x100);
	if (n!=0x100) {
		errorf(FALSE,"Image corrupt! I cannot read image header "
							"(only %d bytes)!",n);
		abandonimage();
		return -1;
	}
	if (!tag_ok()) {
		errorf(FALSE,"\"%s\" is not a DSK image!",name);
		abandonimage();
		return -1;
	}
	if ((disk_header.nbof_heads<1) || (disk_header.nbof_tracks<1)) {
		errorf(FALSE,"--==>>> open_image: \"%s\"",name);
		abandonimage();
		return -1;
	}
	        
/* allocate memory */
	track = Malloc(disk_header.tracksize);
        
/* set up varaibles */
	filler = 0xE5;
	cur_user=0;

	p = getwd(full_imagename);
	if (p) strcpy(full_imagename,p);
	if (full_imagename[strlen(full_imagename)-1]==DIRSEPARATOR)
		full_imagename[strlen(full_imagename)-1]=0;
	strcat(full_imagename,dirsep);
	strcat(full_imagename,name);
#if DOS
	lower(full_imagename);
#endif
	if((imagename=strrchr(full_imagename,DIRSEPARATOR)))
		imagename++;
	else	imagename=full_imagename;
        
/* determine system/data-disk */
	read_track(0,0);	
	cur_format = ((struct t_header*)track)->sector[0].sector; 

	if (cur_format>=SYSTEMFORMAT && cur_format<SYSTEMFORMAT+FORMATDIFF)
		cur_format=SYSTEMFORMAT;
	if (cur_format>=IBMFORMAT && cur_format<IBMFORMAT+FORMATDIFF)
		cur_format=IBMFORMAT;
	if (cur_format>=DATAFORMAT && cur_format<DATAFORMAT+FORMATDIFF)
		cur_format=DATAFORMAT;

	switch (cur_format) {
	case SYSTEMFORMAT:
		dpb=&DPB_store[SYSTEM_DPB]; update_dpb(dpb,track); break;
	case IBMFORMAT: /* or Vortex format */
		if (disk_header.nbof_heads==1) { /*sensible condition???*/
			dpb=&DPB_store[IBM_DPB];
			update_dpb(dpb,track);
		} else {
			dpb=&DPB_store[VORTEX_DPB];
			update_dpb(dpb,track);
			cur_format = VORTEXFORMAT;
		}
		break;
	case DATAFORMAT:
		dpb=&DPB_store[DATA_DPB]; update_dpb(dpb,track); break;
	default:
		errorf(FALSE,"Disk format not recognised!");
		abandonimage();
		return -1;
		break;
	}


/* calculate number of blocks and allocate memory */
	blk_alloc = Malloc(dpb->DSM+1);
	directory = (DirEntry*)Malloc(sizeof(DirEntry)*(dpb->DRM+1));

/* allocate block buffer */
	block_buffer = (uchar*)Malloc(dpb->BLS);
        
/* get directory information */
	get_directory();
	calc_allocation();

	return 0;
}


int comment_image(const char *text) {
/*  ^^^^^^^^^^^^^
Place <text> in the comment field of the image and save the image
48 bytes tag = 8 bytes required + 40 bytes free */

int     i;
	memset(disk_header.tag+8,0,40);
	i=0;
	while (text[i] && i<40) {
		*(disk_header.tag+8+i) = text[i];
		i++;
	}

	lseek(imagefile,0L,SEEK_SET);
	if (write(imagefile,&disk_header,sizeof(disk_header)) < 0) {
		return errorf(TRUE,"--==>>> comment_image");
	}
	return 0;
}



/********
  Blocks
 ********/

int get_free_block() {
/*  ^^^^^^^^^^^^^^ */
static int next = 0;
int	i;

	if (next > dpb->DSM) next = 0;
/* try to allocate next block, if there was a previos one (next!=0) */
	if ((next != 0) && (is_free_block(next))) return next++;
/* try to find the first free block */
	for (i=dpb->DBL;i<=dpb->DSM;i++) {
		if (is_free_block(i)) return i;
	}
	return -1;
}


/*****************
  FS Maintenance
 *****************/


struct {
	int	flag;
	uchar	type;
	ushort	load;
	ushort	jump;
	ushort	size;
	ushort	checksum;

} amsdos_header;


void get_amshead(int ent) {
/*   ^^^^^^^^^^^
Read the first 128 bytes from the file and store them in the structure
<amsdos_header>. Set the <flag> to 2, if it's a valid header; to 1, if it's
invalid; and to 0 if the files is empty.
*/

int	i;
ushort	sum = 0;
uchar	*buf;

	if (directory[ent].blk[0] == 0) {
		amsdos_header.flag = 0;
		return;
	}
	buf = read_block((signed)directory[ent].blk[0]);
	amsdos_header.type	= buf[18];
	amsdos_header.load	= buf[21] + 256*buf[22];
	amsdos_header.jump	= buf[26] + 256*buf[27];
	amsdos_header.size	= buf[64] + 256*buf[65];
	amsdos_header.checksum	= buf[67] + 256*buf[68];

	for (i=0;i<=66;i++) {
		sum += buf[i];
	}
	if (sum==amsdos_header.checksum)
		amsdos_header.flag = 2;
	else
		amsdos_header.flag = 1;

	return;
}

/****** local function for dir() ******/

int cmp_array(int *x, int *y) {
/*  ^^^^^^^^^
Compares two filenames, whose entry numbers are <x> and <y> */
int	res;

	if (directory[*x].user < directory[*y].user)		res = -1;
	else if (directory[*x].user > directory[*y].user)	res = 1;
	else {
		res = strncmp((signed char*)directory[*x].root,
			      (signed char*)directory[*y].root, 8);
		if (res==0)
			res = strncmp((signed char*)directory[*x].ext,
				      (signed char*)directory[*y].ext, 3);
	}
	return res;
}


int dir(char *pat, int mask) {
/*  ^^^
<mask> is a "bit-or" of the following bits:
	DIR_DOUBLE	default 					--00
	DIR_WIDE	only names and sizes				--01
	DIR_AMSHEAD	incl. AMSDOS Header				--10
	DIR_LONG	incl. entries, records, all attributes		--11

	DIR_SORT	sorted						-1--
*/


long	total_bytes = 0;
int	used_entries = 0;
int	i,j;
int	ent;
int	files;
int	mode;
int	*array; 		/* temporary dynamic array */
char	upbuffer[INPUTLEN];	/* buffer for upper case conversion*/
char	*buf;
int	user;
char	root[INPUTLEN];
char	ext[INPUTLEN];

	array = (int*)Malloc(sizeof(int)*max(256,dpb->DRM+1));

	parse_cpm_filename(pat,&user,root,ext);
	if (user==-1) user = cur_user;

/* calculate active users, entries */
	files = 0;
	for (i=0;i<256;i++) array[i]=0;
	for (i=0;i<=dpb->DRM;i++) {
		if (directory[i].user != 0xE5) used_entries++;
		if (directory[i].first) {
			array[directory[i].user]++;
			files++;
		}
	}

	newpage("c");

/*** header ***/
	strcpy(upbuffer,imagename);
	upper(upbuffer);
	printm(0,"Directory of Image: %s, User ", upbuffer);
	if (user==-2)	printm(0,"ALL\n\n");
	else		printm(0,"%-d\n\n",user);
	nextline(); nextline();

/*** used users ***/
	if (files>0) {
		printm(0,"Used Users: ");
		for (i=0;i<256;i++) {
			if (array[i]!=0) {
				printm(0,"%d with %d file%s  \t",i,array[i],
							plural(array[i]));
			}
		}
		printm(0,"\n\n");	nextline(); nextline();
	}
/*** files ***/

/* fetch all needed files */
 /* fill the array with 64+1 large, but nonnegative values */
	for (i=0;i<=dpb->DRM+1;i++) array[i]=0xFFF;

	ent=glob_cpm_file(pat);
	files=0;
	while (ent>=0) {
		array[files] = ent;
		total_bytes += directory[ent].size;
		ent=glob_cpm_next();
		files++;

	}

	if (mask & DIR_SORT) qsort(array,files,sizeof(int),
		(int(*)(const void*,const void*))cmp_array);

/* <array> contains now the entry numbers of the requested files in
the requested order */


/* output of the filenames */

	if (files==0) {
		printm(0,"No files\n");
		goto footer;
	}

	i=0;
	switch (mask&0x3) {
	case DIR_DOUBLE:
		printm(0," U Name            Size  Attr    Ent   "
		       "%c U Name            Size  Attr    Ent\n",vert);
		nextline();
		printm(0,"%s%c",repstr(hori,39),cross);
		printm(0,"%s\n",repstr(hori,37));
		nextline();
		while (array[i]<0xFFF) {
			j=array[i]; ent=1;
			while(directory[j].next>-1) {
				ent++;
				j=directory[j].next;
			}

			printm(0," %u %-12s  %6lu  %3s %c  %3d   ",
				directory[array[i]].user,
				directory[array[i]].name,
				directory[array[i]].size,
				show_attr(directory[array[i]].attr,ATTR_R,FALSE),
				( directory[array[i]].attr&~ATTR_R? '+' : ' '),
				ent);
			if (i%2==0)	printm(0," %c",vert);
			else		{putcharm(0,10); nextline();}
			i++;
		};
		break;
	case DIR_WIDE:
		printm(0,"Name          Size %c"
			 "Name          Size %c"
			 "Name          Size %c"			 
			 "Name          Size\n",vert,vert,vert);
		nextline();
		printm(0,"%s%c",repstr(hori,19),cross);
		printm(0,"%s%c",repstr(hori,19),cross);
		printm(0,"%s%c",repstr(hori,19),cross);		
		printm(0,"%s\n",repstr(hori,19));
		nextline();
		while (array[i]<0xFFF) {
			printm(0,"%-12s%6lu ",
				directory[array[i]].name,
				directory[array[i]].size);
			if (i%4!=3)	printm(0,"%c",vert);
			else		{putcharm(0,10); nextline();}
			i++;
		};
		break;
	case DIR_AMSHEAD:
		printm(0," U Name           Size Attr     Amsdos-Header\n");
		nextline();
		printm(0,"%s\n",repstr(hori,74));
		nextline();
		while (array[i]<0xFFF) {
			printm(0,"%2u %-12s %6lu %3s %c    ",
				directory[array[i]].user,
				directory[array[i]].name,
				directory[array[i]].size,
				show_attr(directory[array[i]].attr,ATTR_R,FALSE),
				( directory[array[i]].attr&~ATTR_R? '+' : ' '));

			get_amshead(array[i]);
			if (amsdos_header.flag==0) {
				printm(0,"Empty");
			} else if (amsdos_header.flag==1) {
				if (strncmp((signed char*)
				  directory[array[i]].ext,"COM",3)==0)
					printm(0,"CP/M Program");
				else
					printm(0,"---");
			} else {
				if (amsdos_header.type & 0x1)
					printm(0,"protected ");
				switch ((amsdos_header.type>>1) & 0x7) {
				case 0: printm(0,"BASIC "); break;
				case 1: printm(0,"Binary"); break;
				case 2: printm(0,"Screen"); break;
				case 3: printm(0,"ASCII "); break;
				default:printm(0,"Type=&%2X",
							amsdos_header.type);
				}

				printm(0,"  Load=&%-4X, Jump=&%-4X, Size=&%-4X",
					amsdos_header.load,
					amsdos_header.jump,
					amsdos_header.size);
			}

			putcharm(0,10); 	nextline();

			i++;
		};
		break;
	case DIR_LONG:
		printm(0,"User  Name            Size  Attr    Ext. Attr. "
			 "Detect Entries Records Blocks\n");
		nextline();
		printm(0,"%s\n",repstr(hori,77));
		nextline();
		while (array[i]<0xFFF) {
			strcpy(upbuffer,
				(signed char*)directory[array[i]].name);
			j=array[i]; ent=1;
/* detect mode */
			if (directory[array[i]].blk[0]==0)
				mode = -1;
			else {
/* the only other function using <block_buffer> here is <get_amshead>,
but they do not interfere! */
				buf = read_block(directory[array[i]].blk[0]);
				mode = detectmode(buf,dpb->BLS);
			}
/* count entries */
			while(directory[j].next>-1) {
				ent++;
				j=directory[j].next;
			}

			printm(0,"%2u    %-12s  %6lu  %s %-6s%5u%8lu%7lu",
				directory[array[i]].user,
				upbuffer,
				directory[array[i]].size,
				show_all_attr(directory[array[i]].attr,TRUE),
				mode==-1? "Empty"
					: (mode==M_TEXT? "Text" : "Bin"),
				ent,
				(directory[array[i]].size+RECORDSIZE-1)/RECORDSIZE,
				(directory[array[i]].size+(dpb->BLS-1))/dpb->BLS);
			putcharm(0,10); 	nextline();
			i++;
		};
		break;
	} /* switch */
	printm(0,"\n"); 	nextline();


/*** footer ***/
footer:

	printm(0,"\n%d file%s in %lu Bytes\n",
		files, plural(files),
		total_bytes);
	nextline(); nextline();
	printm(0,"(%lu Bytes free, %lu Bytes allocated, %d entr%s of %d)\n",
		(long)free_blks*dpb->BLS,
		(long)allocated_blks*dpb->BLS,
		used_entries, plural_y(used_entries),
		dpb->DRM+1);
	nextline();

	free(array);
	return 0;
}


long delete(bool silent, char *pat) {
/*   ^^^^^^
Delete all files that match <pat>.
Answer the amount of deleted bytes (at least 0) */
long	freed = 0;
long	total_freed = 0;
int	ent, i;


/* warn, if <pat> contains *.* or is only usernumber */
	if (match("*\\*.\\**",pat) || match("*:",pat)) {
		if (!silent && Verb > 0) {
			printm(1,"Delete all in \"%s\"? ",pat);
			if (!confirmed()) return 0;
		}
	}

	ent = glob_cpm_file(pat);
	if (ent<0) {
		if (!silent) errorf(FALSE,"\"%s\" not found",pat);
		return 0;
	}


	while (ent>=0) {
		freed = 0;
		if (directory[ent].attr & ATTR_R) {
			if (!silent && Verb > 0) {
				printm(1,"\"%u:%s\" readonly. Delete? ",
				    directory[ent].user,directory[ent].name);
				if (!confirmed()) {
					ent = glob_cpm_next();
					continue;
				}
			}
		}

		if (!silent) printm(3,"Deleting \"%u:%s\": ",
				directory[ent].user,directory[ent].name);
		freed += directory[ent].size;
		while (ent>=0) {
			directory[ent].user = 0xE5;
			for (i=0;i<BLKNR;i++) {
				if (directory[ent].blk[i]==0) break;
				free_block(directory[ent].blk[i]);
			}

			ent = directory[ent].next;
		};

		if (!silent) printm(3,"%ld Bytes\n",freed);
		total_freed += freed;
		ent = glob_cpm_next();
	}

	update_directory();
	calc_allocation();
	return total_freed;
}


int change_attrib(char* pattern, int set, int reset) {
/*  ^^^^^^^^^^^^^
Change all attributes, masked by <mask> to <set> in all files matching
<pattern>. Answer -1 on error.
*/
int	ent, ent0;

	ent = glob_cpm_file(pattern);
	if (ent<0)
		return errorf(FALSE,"\"%s\" not found",pattern);
	while (ent>=0) {
		printm(3,"Changing \"%s\" from \"%s\"",
			directory[ent].name,
			show_all_attr(directory[ent].attr,TRUE));
		ent0=ent;
		do {
			directory[ent].attr |= set;
			directory[ent].attr &= ~reset;
			ent = directory[ent].next;
		} while (ent>=0);
		printm(3," to \"%s\"\n",
			show_all_attr(directory[ent0].attr,TRUE));
		ent=glob_cpm_next();
	}


	return 0;
}


int sysgen(char *filename) {
/*  ^^^^^^
Copies the CP/M system from <filename> to the first two tracks of the image.
Does not test on Data-Format or sufficient space!
Answer -1 on error
*/
char	buf[INPUTLEN];
int	cpm;
int	t;
short	tracksize = dpb->BPS*dpb->SECS; /* = Disk_header.tracksize - 0x100 */
long	n;

	strcpy(buf,filename);
	if (strchr(buf,'.')==NULL) strcat(buf,".cpm");

	cpm = open(buf,O_RDONLY|O_BINARY,0);
	if (cpm < 0) {
		return errorf(TRUE,"Cannot open \"%s\" for reading",buf);
	}

	for (t=0;t<dpb->OFS;t++) {
/* skip AMSDOS header and position on track <t> */
		n = lseek(cpm,(long)128+t*tracksize,SEEK_SET);
		if (n == -1L) {
			close(cpm);
			return errorf(TRUE,"CP/M Image currupt! "
				"I cannot position on track %d",t);
		}
		read_track(t%dpb->HDS,t/dpb->HDS);
		n = read(cpm,track+0x100,tracksize);
		if (n != tracksize) {
			close(cpm);
			return errorf(TRUE,"CP/M Image currupt! "
			    "I can only read %d bytes (instead of %d bytes) ",
			    n, tracksize);
		}
		write_track();
	}
	printm(2,"CP/M \"%s\" copied to CPC filesystem\n",buf);

	close(cpm);
	dpb->SYS = TRUE;

	return 0;
}


int format(char* name, DPB_type *dpb) {
/*  ^^^^^^
Creates a new image with name <name> and format <dpb>. <dpb> must be out of
the <DPB_store>, not processed by <update_dpb>!
Answers -1 on error.
*/
int	file;
int	i,h,j;
struct t_header *trhd;
time_t	now;

	file = creat(name,0644);
	if (file<0) {
		return errorf(TRUE,"Cannot open \"%s\" for writing",name);
	}


/* fill disk_header */
	printm(3,"Formatting (%s) ",show_format(dpb->ID));
	for (j=0;j<0x2F;j++) disk_header.tag[j] = 0;
	strcpy ((signed char*)disk_header.tag,"MV - CPCEMU / ");
	memset((disk_header.tag)+14,' ',20);
	now = time(NULL);
	strftime(((signed char*)disk_header.tag)+14,20,"%d %b %y %H:%M",
							localtime(&now));
	disk_header.nbof_tracks	= dpb->TRKS;
	disk_header.nbof_heads	= dpb->HDS;
	disk_header.tracksize	= 0x100 + dpb->BPS*dpb->SECS;
	memset(disk_header.unused,0,0xCC);
	if (write(file,&disk_header,0x100) < 0) {
		return errorf(TRUE,"FORMAT");
	}

	track = Malloc(disk_header.tracksize);
	trhd = (struct t_header*)track;
	for (i=0;i<disk_header.nbof_tracks;i++)
	    for (h=0;h<disk_header.nbof_heads;h++) {
		if (Break_Wish) {
			close(file);
			abandonimage();
			do_break();
		}
/* fill track_header */
		putcharm(3,'.'); fflush(stdout);
		strncpy((signed char*)trhd->tag,"Track-Info\r\n",0x10);
		trhd->track	= i;
		trhd->head	= h;
		trhd->unused[0] = 0;
		trhd->unused[1] = 0;
		trhd->BPS	= dpb->BPS/0x100;
		trhd->SPT	= dpb->SECS;
		trhd->GAP3	= 0x4E;
		trhd->filler	= 0xE5;
		for (j=0;j<dpb->SECS;j++) {
			trhd->sector[j].track	= i;
			trhd->sector[j].head	= h;
			trhd->sector[j].sector	= dpb->SEC1+j;
			trhd->sector[j].BPS	= dpb->BPS/0x100;
			trhd->sector[j].status1 = 0;
			trhd->sector[j].status2 = 0;
			trhd->sector[j].unused[0]=0;
			trhd->sector[j].unused[1]=0;
		}
		for (j=dpb->SECS; j<29; j++)  {
			memset(&(trhd->sector[j]),0,8);
		}

		memset(track+0x100,trhd->filler,disk_header.tracksize-0x100);
		if (write(file,track,disk_header.tracksize) < 0) {
			return errorf(TRUE,"FORMAT");
		}
	} /* end for i,h */
	printm(3,"   done\n");
	free(track);
	close(file);
	*disk_header.tag = 0;
	cur_trk = -1;

	return 0;
}


int ren_file(char *from, char *to) {
/*  ^^^^^^^^
Renames a file.
Wildcards are allowed and complete filenames must be given.
See ren_wild().
If a file is renamed to itself, do nothing.
The directory must be updated and written afterwards!
Answer -1 on error.
*/
int	to_user;
char	to_root[INPUTLEN];
char	to_ext[INPUTLEN];
char	to_full[INPUTLEN];
int	from_user;
char	from_root[INPUTLEN];
char	from_ext[INPUTLEN];
char	from_full[INPUTLEN];
int	ent;
const char wild_fmt[] = "\"%s\" may not contain wildcards";

	upper(to);
	upper(from);
	if (has_wildcards('c',from)) {
		return errorf(FALSE,wild_fmt,from);
	}
	if (has_wildcards('c',to)) {
		return errorf(FALSE,wild_fmt,to);
	}

	parse_cpm_filename(from,&from_user,from_root,from_ext);
	if (from_user==-1) from_user = cur_user;
	if (from_user==-2) return errorf(FALSE,"--==>>> ren_file: wild user");
	if (*from_root==0)
		return errorf(FALSE,"No name in \"%s\"",from);
	build_cpm_name((signed char*)from_full, from_user,
		(signed char*)from_root, (signed char*)from_ext);
        
	parse_cpm_filename(to,&to_user,to_root,to_ext);
	if (to_user==-1) to_user = cur_user;
	if (*to_root==0) {
		strcpy(to_root,from_root);
		strcpy(to_ext,from_ext);
	}
	build_cpm_name((signed char*)to_full, to_user,
		(signed char*)to_root, (signed char*)to_ext);

/* test on identity of <to> and <from> */
	if (strcmp(to_full,from_full)==0) {
		printm(2,"Renaming \"%s\" to itself\n",from_full);
		return 0;
	}
	
/* check if already exists */
	if (glob_cpm_file(to_full)>=0) {
		if (Verb > 0) {
			printm(1,"\"%s\" already exists! Overwrite? ",to_full);
			if (confirmed())	delete(TRUE,to_full);
			else			return 0;
		} else return errorf(FALSE,"\"%s\" already exists",to_full);
	}

       	ent = glob_cpm_file(from_full);
	if (ent<0) return errorf(FALSE,"\"%s\" not found",from_full);

	printm(2,"Renaming \"%u:%s\" to \"%s\"\n",
		directory[ent].user, directory[ent].name, to_full);

	do {
		directory[ent].user = to_user;
		str2mem((signed char*)directory[ent].root,
			(signed char*)to_root, 8);
		str2mem((signed char*)directory[ent].ext,
			(signed char*)to_ext, 3);
		ent = directory[ent].next;
	} while (ent>=0);
		
	return 0;
}


int ren_wild(char *pat, int us) {
/*  ^^^^^^^^
Renames all files that match <pat> to <user>.
The directory must be updated and written afterwards!
Answer -1 on error. */
int	ent;
char	src[20], trg[20];

	ent=glob_cpm_file(pat);
	if (ent<0) {
		errorf(FALSE,"\"%s\" not found",pat);
		return -1;
	}
	while (ent>=0) {
		sprintf(src,"%u:%s",directory[ent].user,directory[ent].name);
		sprintf(trg,"%u:%s",us,directory[ent].name);
		glob_env++;
		ren_file(src,trg);
		glob_env--;
		ent=glob_cpm_next();
	}
	return 0;
}


int copy_file(char *from, char *to) {
/*  ^^^^^^^^^
Copies a file to a new file with another name.
This preliminary version goes the way through put() and get() with a
temporary file.
Answer -1 on error. */

char	tempname[INPUTLEN];
int	err;

	tmp_nam(tempname);
	
	printm(3,"Copying \"%s\" to ",from);
	err=get(from,tempname);
	if (err==-1) {
		unlink(tempname);
		return -1;
	}
	err=put(tempname,to);
	if (err < 0) {
		unlink(tempname);
		return -1;
	}
	printm(3,"\"%s\"\n",to);
	
	unlink(tempname);
	return 0;
}


int copy_wild(char *pat, int us) {
/*  ^^^^^^^^^
Copies all files that match <pat> to <user>.
Answer -1 on error. */
int	ent;
char	src[20], trg[20];

	ent=glob_cpm_file(pat);
	if (ent<0) {
		errorf(FALSE,"\"%s\" not found",pat);
		return -1;
	}
	while (ent>=0) {
		sprintf(src,"%u:%s",directory[ent].user,directory[ent].name);
		sprintf(trg,"%u:%s",us,directory[ent].name);
		glob_env++;
		copy_file(src,trg);
		glob_env--;
		ent=glob_cpm_next();
	}
	return 0;
}



/*********
  Dumping
 *********/


int dumpdir (FILE* file) {
/*  ^^^^^^^ */
int	i,j;
char	n[INPUTLEN],e[INPUTLEN];
					
	fprintf(file," #  U NAME         EX RE ATR BLOCKS\t\t\t\t\t    NEX\n");
	for (i=0;i<=dpb->DRM;i++) {
		strncpy(n,(signed char*)directory[i].root,8); 	n[8] = 0;
		strncpy(e,(signed char*)directory[i].ext,3);	e[3] = 0;
		fprintf(file,"%2X%c%2X %s.%s %2X %2X ",
			i, (directory[i].first?'>':' '),
			directory[i].user,
			n, e,
			directory[i].extent, directory[i].rec);
		fprintf(file,"%2X%1X",(directory[i].attr&~0x7)>>3,
			directory[i].attr&0x7);
		for (j=0;j<BLKNR;j++) {
			if (directory[i].blk[j])
				fprintf(file," %2X",directory[i].blk[j]);
			else
				fprintf(file," --");
		}

		if (directory[i].next>=0)
			fprintf(file,">%2X",directory[i].next);
		else
			fprintf(file,"<<<");
		putc(10,file); 
		if (fflush(file)!=0)
			return errorf(TRUE,"DUMP -D");
	}
	return 0;
}


int dump(FILE *file, int block, int h, int t, int s) {
/*  ^^^^
Dump the contents of block <block> to <file> or if <block> = -1, the contents
of <h>,<t>,<s> to <file>.
*/

int	hd, trk, sec;
int	i, j;
int	secs, k;
uchar	*p, *q;

	if (block == -1) {
		hd = h; trk = t; sec = s;
		secs=1;
		block = blk_calc(hd,trk,sec);
	} else {
		hd  = hd_calc(block);
		trk = trk_calc(block);
		sec = sec_calc(block);
		secs=dpb->BLS/dpb->BPS;
	}


	for (k=0;k<secs;k++) {
		read_track(hd,trk);
        
		fprintf(file,
			"\nBlock %d/Part %d   Head %d Track %d Sector %d\n\n",
			block,k,  hd,trk,sec);
		i = 0;
		p = track+0x100+sec*dpb->BPS;
		while (i<dpb->BPS) {
			fprintf(file,"%3X %c ",i,vert);
			q=p;
			for (j=0;j<16;j++) fprintf(file,"%2X ",*p++);
			fprintf(file," %c ",vert);
			p=q;
			for (j=0;j<16;j++) {
				if (*p<32)
					putc(' ',file);
				else if (*p>=127)
					putc('~',file);
				else
					putc(*p,file);
				p++;
			}
			i+=16;
			putc(10,file);
			if (fflush(file)!=0)
				return errorf(TRUE,"DUMP");
		}

		next_sector(&hd,&trk,&sec);
	};
	return 0;
}


int map(FILE *file) {
/* Writes the disk allocation map on <file>. */
int	h, t, s, b;
char	*str;

	str = repstr(' ',max(0,(dpb->SECS*3+2)-9));
						/* 9 = len("%c Head %-2d") */

	fprintf(file,"      ");
	for (h=0;h<dpb->HDS;h++) {
		fprintf(file,"%c Head %-2d%s",vert,h,str);
	}
	fprintf(file,"\n");

	fprintf(file,"Track ");
	for (h=0;h<dpb->HDS;h++) {
		fprintf(file,"%c ",vert);
		for (s=0;s<dpb->SECS;s++) {
			fprintf(file,"%-2d ",s);
		}		
	}
	fprintf(file,"\n");

	str=repstr(hori,6); fprintf(file,"%s",str);
	for (h=0;h<dpb->HDS;h++) {
		str = repstr(hori,dpb->SECS*3+1);
		fprintf(file,"%c%s",cross,str);
	}
	fprintf(file,"\n");

	for (t=0;t<dpb->TRKS;t++) {
		fprintf(file,"%-4d  ",t);
		for (h=0;h<dpb->HDS;h++) {			
			fprintf(file,"%c ",vert);
			for (s=0;s<dpb->SECS;s++) {

				b = blk_calc(h,t,s);
				if (h+dpb->HDS*t < dpb->OFS) {
					if (dpb->SYS) {
						fprintf(file,"$$ ");
					} else {
						fprintf(file,"-- ");
					}
					continue;
				}
				if (b < dpb->DBL) {
					fprintf(file,"DD ");
					continue;
				}
				if (is_free_block(b)) {
					fprintf(file,"-- ");
				} else {	
					fprintf(file,"%2X ",blk_alloc[b]);
				}
			}
		}
		fprintf(file,"\n");
		if (fflush(file)!=0)
			return errorf(TRUE,"DUMP -M");
	}
	return 0;
}


/**********
  Transfer
 **********/


int detectmode (char *buf, int size)  {
/*  ^^^^^^^^^^
Count <size> characters in <buf>. If more than 70% are printable, the
file is considered as Text, otherwise Binary.
Returns M_TEXT or M_BIN.
*/
long	printable, total;
int	k;

	printable = total = 0;
	for (k=0;k<=size; k++)	{
		if (buf[k] == CPM_EOF) break;
		if ((buf[k]==10)||(buf[k]==13)||
		    ((buf[k]>=32)&&(buf[k]<=126)))  {
			printable++;
		}
		total++;
	}
	if (total==0) return M_BIN;	/* i.e. first char = ^Z */
	if ((100*printable/total) > 70)
		return M_TEXT;
	else
		return M_BIN;
}


long get(char *src, char *target) {
/*   ^^^
Get a file from CPC filesystem to local filesystem.
<src> is the CPC filename
<target> is the local filename
Returns number of bytes read or -1 if not found
*/
int	ent, i, k;
int	file;
long	bytes = 0;	/* sum of bytes copied */
int	size;		/* size to copy at a chunk */
bool	last;		/* in last entry? */
int	localmode;
int	err;
uchar	*buf, *p;

/* open CP/M file */
	if (has_wildcards('c',src)) {
		return errorf(FALSE,"\"%s\" may not contain wildcards",src);
	}
	ent = glob_cpm_file(src);
	if (ent<0)	return errorf(FALSE,"\"%s\" not found",src);


/* open DOS file */
	if (access(target,F_OK)==0) {
		if (Verb > 0) {
			printm(1,"\"%s\" already exists! "
					"Overwrite? ",target);
			if (!confirmed())  {
				return -1;
			}
		}
	}
	file=creat(target,0644);
	if (file<0) return errorf(TRUE,"Cannot open \"%s\" for writing",target);


	localmode = -1; /* i.e. unknown */
	do {
		last = directory[ent].next == -1;	/* last entry? */
		for (i=0;i<BLKNR;i++) {
			if (directory[ent].blk[i]==0) {
				if (!last) {
					errorf(FALSE,"Directory entry for "
					  "\"%u:%s\" corrupt",
					  directory[ent].user,
					  directory[ent].name);
					close(file);
					return -1;
				} else
					break;
			}

			buf = read_block((signed)directory[ent].blk[i]);
			if (localmode == -1) {
				if (mode!=M_AUTO)
					localmode = mode;
				else {
					localmode = detectmode(buf,
							(signed)dpb->BPS);
/*					printm(3,"%s detected.\n",
						show_mode(localmode));
*/
				}
			}

/* copy a whole block, except if on last blockpointer in last entry, then
   copy records */
			if (last &&
			    (i==BLKNR-1 ||
			     directory[ent].blk[i+1]==0)) {
				size = directory[ent].rec*RECORDSIZE % dpb->BLS;
				if (size==0) size=dpb->BLS;
				  /* works, because .rec==0 is impossible */
			} else {
				size = dpb->BLS;
			}
			
			if (localmode==M_BIN) {
				err=write(file,buf,size);
				bytes += size;
			} else  {	/* up to ^Z */
				p = memchr(buf,CPM_EOF,size);
				if (p==NULL) {/* no ^Z */
				    err=write(file,buf,size);
				    bytes += size;
				} else {/* get only a fraction of block */
				    k = p-buf;
				    err=write(file,buf,k);
				    bytes += k;
				}
			}
			if (err<0) {
				close(file);
				return errorf(TRUE,"GET");
			}
		}
		ent = directory[ent].next;
	} while (ent>=0);

	close(file);
	return bytes;
}



long put(char *src, char *trg) {
/*   ^^^
Writes the DOS file <src> as CP/M File <trg>.
Returns number of bytes written or -1 if skipped, -2 if error */

uchar	*buf;
int	file;
int	entry,			/* current dir entry */
	blk,			/* pointer in data space */
	i;
long	size;
long	total;			/* total bytes */
long	entry_total;		/* total bytes for one entry */
int	used_entries;
long	bytes_to_copy;		/* size of DOS files */

int	usr;
char	rootname[INPUTLEN];
char	extension[INPUTLEN];
const char wild_fmt[] = "\"%s\" may not contain wildcards";
struct stat stat_buf;

	buf = block_buffer;	/* simply a shortcut */

	if (has_wildcards('d',src)) {
		errorf(FALSE,wild_fmt,src);
		return -2;
	}
	if (has_wildcards('c',trg)) {
		errorf(FALSE,wild_fmt,trg);
		return -2;
	}
	upper(trg);


/* test on existence and size of DOS file */
	if ((file = open(src, O_RDONLY|O_BINARY, 0)) == -1)  {
		errorf(TRUE,"Cannot read \"%s\"",src);
		return -2;
	}
	if (fstat(file,&stat_buf)) {
		errorf(TRUE,"--==>>> put: cannot stat \"%s\"",src);
		return -2;
	}
        bytes_to_copy = stat_buf.st_size;
	if (bytes_to_copy > (long)(dpb->DSM+1)*dpb->BLS) {
		errorf(FALSE,"\"%s\" is bigger than image",src);
		return -1;
	}

/* spilt the <trg> into name and extension */
	if (parse_cpm_filename(trg,&usr,rootname,extension)) return -1;
	if (usr==-1) usr = cur_user;
	if (*rootname==0) {
		errorf(FALSE,"No filename in \"%s\"",trg);
		return -2;
	}


/* test on existence in CP/M directory */
	if (glob_cpm_file(trg) >= 0) {
		if (Verb > 0) {
			printm(1,"\"%s\" already exists! Overwrite? ",trg);
			if (!confirmed())  {
				close(file);
				return -1;
			}
		}
		delete (TRUE,trg);
	}

/* walk thru the directory */
	total		= 0;
	used_entries	= 0;
	for (entry=0;entry<=dpb->DRM;entry++) {
		if (directory[entry].user != 0xE5) continue;

/* preread the first part, necessary if filesize multipe of 16k */
		size = read(file,buf,dpb->BLS);
		if (size==0) break;

/* fill name, user ... */
		directory[entry].user = usr;
		str2mem((signed char*)directory[entry].root,
			(signed char*)rootname, 8);
		str2mem((signed char*)directory[entry].ext,
			(signed char*)extension, 3);
		directory[entry].attr	= 0x00;
		directory[entry].unused[0]	= 0;	/* reserved for CP/M */
		directory[entry].unused[1]	= 0;

/* walk thru the block pointer area */
		entry_total = 0;
		for (i=0;i<BLKNR;i++) {
			if (i>0) size = read(file,buf,dpb->BLS);
			total += size;
			entry_total += size;

			if (size==0) break;
/* get a free block and copy the data */
			blk = get_free_block();
			if (blk==-1)  {
				errorf(FALSE,"CPC Disk full!");
				update_directory();
				close(file);
				delete(TRUE,trg);
				return -2;
			}
			directory[entry].blk[i] = blk;
/* add a ^Z at the end */
			if (size<dpb->BLS) {
				buf[size] = CPM_EOF;
				size++;
			}
/* write the block */
			alloc_block(blk,entry);
			if (write_block(blk,(signed char*)buf)==NULL) {
				errorf(FALSE,"Write error!");
				close(file);
				return -2;
			}
			bytes_to_copy -= dpb->BLS;
		}

/* finish up this direntry */
		while (i<BLKNR) directory[entry].blk[i++] = 0;

/* split the filesize such that <size_so_far> = <extent>*16k + <rec>*128byte */
		directory[entry].extent = entry_total/EXTENTSIZE
			+ (dpb->EXM+1) * used_entries;
		directory[entry].rec = (entry_total+RECORDSIZE-1)
			/ RECORDSIZE % RECORDSIZE;
/* if <rec>=0 and <extent> > 0, then set <rec>:=80h and decrement <extent> */
		if (directory[entry].rec==0 && directory[entry].extent>0) {
			directory[entry].rec = RECORDSIZE;
			directory[entry].extent--;
		}

		used_entries++;
		if (size==0) break;
	}

	close(file);
	update_directory();

/* uncopied data left */
	if (bytes_to_copy > 0) {
		errorf(FALSE,"CPC Directory full!");
		delete(TRUE,trg);
		return -2;
	}

/* <put_directory> needs <block_buffer> too, but it's available now after
<put> is nearly complete */
	put_directory();
	calc_allocation();
	return total;
}



