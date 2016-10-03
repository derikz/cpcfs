
> :warning: There is also an [extended version CPCXFS](http://www.cpctech.org.uk/download/cpcxfs.zip),
> that also allows to maintain extended DSK images (thanks to [Kevin
> Thacker](http://www.cpctech.org.uk/)).

# CPCFS

CPCFS maintains diskimages (DSK files), needed by some Amstrad CPC
emulators.

## What is CPCFS?

CPCFS is a program to maintain filesystems that are needed by some CPC
emulators (especially CPCEmu by Marco Vieth).

If you are familiar with some emulators, you may have already
encountered the problem, how to copy files form DOS into the image of
an emulator, or vice versa.

The methods described in the documentation of CPCEmu only deal with
transferring whole CPC disks to CPCEmu disk images, but not single
files.

If you use CPCEmu you can use the cassette interface to load and save
files from the CPC memory to DOS files. But this method lacks some (in
my sense important) features, e.g.

* transferring files > 42k
* transferring data files without Amsdos-Header
* installing CP/M, if you don't have a 5 1/4 inch drive at your CPC and
  can't copy whole disks.
* transferring is so slow as the emulator
* you can't transfer files in a batch

If you don't use another emulator than CPCEmu, I do not know another
solution other than CPCFS.

### Where can I download CPCFS?

GO to Github's [release tab](https://github.com/derikz/cpcfs/releases)
and download the latest zip file.

### How do I install CPCFS?

Just unpack the archive. Then copy the	*.EXE files, CPCFS.CFG and
CPCFS.HLP to a place where they can be found (somewhere in the
path) or where you like them to be. CPCFS.CFG must be in the
current directory or in the same directory as CPCFS.EXE.

### How do I use CPCFS?

Read [cpcfs.doc](https://github.com/derikz/cpcfs/blob/master/cpcfs.doc)
or type HELP within CPCFS.

### What other products do I need?

* A CPC-Emulator (CPCEmu, CPE, SimCPC, CPCEmuII, ...)
* A CPC Computer with CP/M License, if you want to copy CP/M to an image.

### What is new in version 0.85.4?

This version is a maintenance release only. There is no new
functionality.

New is:

* runs on modern Windows
* BSD 2-clause License
* updated contact data
* 'echo %M' to show free memory makes no sense nowadays

## Main features:

* Vortex images are supported
* User areas, file attributes and wildcards are fully supported
* Comfortable command line interface including history, extensible help,
  and many options to each command
* Special commands for getting an insight in the filesystem structure
* Extensible help system containing nearly the same as the manual

### Some Commands:

* PUT, GET, MPUT, MGET: copy to and from the image
* OPEN, NEW, DIR, DBP: open, create and look at image
* COPY, REN, ERA, ATTRIB: copy files
* TYPE, DUMP: look at files
* LDIR, LCD, !: access your local PC
* HELP: accesses the online manual


### What files are distributed?

See 'FILES'

### How do I make an exe file myself?

See 'BUILD'
