
/*				<<<<Last Modified: Sun Jun 15 19:38:56 1997>>>>
------------------------------------------------------------------------------

        =====
        CPCFS  --  u n i x . h   ---   Unix specific header
        =====

	Version 0.85                    (c) February '96 by Derik van Zuetphen
------------------------------------------------------------------------------
*/



#include <sys/file.h>
#include <getopt.h>
#include <unistd.h>
#include <strings.h>
#include <setjmp.h>
#if USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define SHELLVAR	"SHELL"
#define SHELLDEFAULT	"/bin/sh"
#define PAGERDEFAULT	"more"
#define LDIRCOMMAND	"ls -l"
#define DIRSEPARATOR	'/'
#define FIRST_OPTIND	0
#define O_BINARY	0		/* dummy to make DOS happy */
#define ENTER		10		/* keycode */

extern char Break_Wish;

void break_handler();
void disable_break();

/* empty dummies */
#define save_path()
#define rest_path()

char *glob_file(char *pattern);
char *glob_next();

long coreleft();
char*	tmp_nam(char*);		/* because DOS' tmpnam() ignores $TEMP */

char wait_for_key (int,char);
void clrscr();
void gotoxy(int col,int lin);

void os_init();
