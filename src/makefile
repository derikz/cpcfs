#				<<<<Last Modified: Sun Apr 13 12:38:59 1997>>>>

CC	= gcc
#CCOPT	= -g -DLINUX=1 -Wall -I/local/include
CCOPT	= -g -DLINUX=1 -Wall
#CCOPT	= -O2 -DLINUX=1
LDOPT	= 

CFLAGS	= $(CCOPT)
LDARGS	= $(LDOPT) -lreadline -lhistory -L /usr/lib/termcap -ltermcap 

HEADER=cpcfs.h unix.h
OBJECTS=unix.o match.o tools.o fs.o ui.o cpcfs.o

default: makedoc cpcfs 

cpcfs: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o cpcfs $(LDARGS)

makedoc: makedoc.c
	$(CC) $(CFLAGS) makedoc.c -o makedoc

unix.o: unix.c unix.h
match.o: match.c match.h

tools.o: tools.c $(HEADER)
fs.o: fs.c $(HEADER)
ui.o: ui.c $(HEADER)
cpcfs.o: cpcfs.c $(HEADER)


strip: cpcfs makedoc
	strip cpcfs makedoc

new:
	touch $(HEADER)
	make
