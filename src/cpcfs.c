
/*					Time-stamp: <98/01/10 14:08:18 derik>
------------------------------------------------------------------------------

	=====
	CPCFS -- CPCEmu Filesystem Maintenance
        =====                   especially for CPCEmu

	Version 0.85                    (c) February '96 by Derik van Zuetphen
------------------------------------------------------------------------------
*/


#include "cpcfs.h"

/***********
  Variables
 ***********/

/****** CPC Filesystem ******/

char	full_imagename[PATH_MAX];	/* full pathname, ... */
char	*imagename;			/* ... only name portion, and ... */
int	imagefile;			/* ... handle of imagefile */

int	cur_user	= 0;
int	cur_format;
int	BLKNR_SIZE 	= 1;
int	BLKNR 		= 16;

uchar	*track = NULL;	/* 0x100 header + track data */
unsigned char filler;	/* copied from track header */

uchar	*block_buffer = NULL;	/* Buffer for 1024 or more bytes */

uchar	*blk_alloc = NULL;	/* Block-Allocation-Table, */
				/* chars point to fcb, FF = free block */
int	allocated_blks;
int	free_blks;
int	total_blks;	/* = allocated_blks + free_blks */
float	percentage;	/* of allocated blocks */

int	mode;		/* Mode for GET-commands */
bool	force;

struct d_header disk_header;


/****** User Interface *****/
int	nbof_args;
char	*arg[INPUTLEN];
char	prompt[INPUTLEN];
int	pagelen, lineno;
bool	Break_Wish;
jmp_buf break_entry;
bool	Interactive;
char	installpath[INPUTLEN];		/* path of argv[0] */
int	Verb = 9;

DirEntry *directory = (DirEntry*)NULL;

char stamp[] = "Compiled: " STAMP ", (C) 1995-98 Derik van Zuetphen";


/****** Disk Parameter Block ******/

/* DPB templates for SYSTEM, DATA, IBM, VORTEX, and user defined */
#define DPB_store_size  5
DPB_type DPB_store[DPB_store_size] = {
/* SYSTEM */
	{	SYSTEMFORMAT, /* ID */
		SYSTEMFORMAT, /* SEC1 */
		9,	/* SECS */
		40,	/* TRKS */
		1,	/* HDS */
		512,	/* BPS */

		36,	/* SPT */
		3,	/* BSH */
		7,	/* BLM */
		0,	/* EXM */
		168,	/* DSM */
		63,	/* DRM */
		0xC0,	/* AL0 */
		0x00,	/* AL1 */
		16,	/* CKS */
		2,	/* OFS */
		0,0,0
	},
/* DATA */
	{	DATAFORMAT, /* ID */
		DATAFORMAT, /* SEC1 */
		9,	/* SECS */
		40,	/* TRKS */
		1,	/* HDS */
		512,	/* BPS */

		36,	/* SPT */
		3,	/* BSH */
		7,	/* BLM */
		0,	/* EXM */
		177,	/* DSM */
		63,	/* DRM */
		0xC0,	/* AL0 */
		0x00,	/* AL1 */
		16,	/* CKS */
		0,	/* OFS */
		0,0,0
	},
/* IBM */
	{	IBMFORMAT, /* ID */
		IBMFORMAT, /* SEC1 */
		8,	/* SECS */
		40,	/* TRKS */
		1,	/* HDS */
		512,	/* BPS */

		36,	/* SPT */
		3,	/* BSH */
		7,	/* BLM */
		0,	/* EXM */
		153,	/* DSM */
		63,	/* DRM */
		0xC0,	/* AL0 */
		0x00,	/* AL1 */
		16,	/* CKS */
		1,	/* OFS */
		0,0,0
	},
/* VORTEX */
	{       VORTEXFORMAT, /* ID */
		0x01,	/* SEC1 */
		9,	/* SECS */
		80,	/* TRKS */
		2,	/* HDS */
		512,	/* BPS */

		36,	/* SPT */
		5,	/* BSH */
		31,	/* BLM */
		3,	/* EXM */
		176,	/* DSM */
		127,	/* DRM */
		0x80,	/* AL0 */
		0x00,	/* AL1 */
		32,	/* CKS */
		2,	/* OFS */
		0,0,0
	}
/* user defined DPB is empty for now */
};


DPB_type *dpb = NULL;	/* pointer to current DPB */


void main (int argc, char **argv) {
int	ui_main (int,char**);
	ui_main (argc,argv);
	exit(0);
}
