
/*				<<<<Last Modified: Sun Jun 15 19:38:06 1997>>>>
------------------------------------------------------------------------------

        =====
        CPCFS  --  d o s . h   ---   Borland C specific header
        =====

	Version 0.85                    (c) February '96 by Derik van Zuetphen
------------------------------------------------------------------------------
*/

#ifndef DOS_H_INCLUDED
#define DOS_H_INCLUDED


#include <dir.h>
#include <string.h>
#include <fcntl.h>
#include <setjmp.h>
#include <errno.h>
#if USE_READLINE
#include <conio.h>
#endif

#define SHELLVAR	"COMSPEC"
#define SHELLDEFAULT	"c:\\command"
#define PAGERDEFAULT	"more"
#define LDIRCOMMAND	"dir"
#define DIRSEPARATOR	'\\'
#define FIRST_OPTIND	1
#define	F_OK		0		/* for access */
#define R_OK		4		/* dto. */
#define ENTER		13

extern char Break_Wish;

/* this getwd() is not exactly the same as Unix' one, but it works */
extern char	cwdbuffer[256];
#define getwd(DUMMY)		getcwd(cwdbuffer,256)

int break_handler();
void disable_break();

void save_path();
void rest_path();

char *glob_file(char *pattern);
char *glob_next();

int add_history(char*);

char* tmp_nam(char*);

/* prototypes of getopt.c */
extern int	optind;
extern char	*optarg;
extern int	opterr;
int getopt(int argc, char *argv[], char *optionS);



#endif
