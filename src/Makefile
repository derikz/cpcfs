
CC	= cc
CCOPT	= -g -DLINUX=1 -DUSE_READLINE=1 -Wall
#CCOPT	= -O2 -DLINUX=1 -DUSE_READLINE=1
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

clean:
	-rm -f *.o

distclean: clean
	-rm -f cpcfs makedoc

new: distclean
	make
