
> :warning: CPCFS is discontinued, but there is an extended version
> [CPCXFS](http://www.cpctech.org.uk/download/cpcxfs.zip), that also
> allows to maintain extended DSK images (modified by [Kevin
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

## What other products do I need?

* A CPC-Emulator (CPCEmu, CPE, SimCPC, CPCEmuII, ...)
* A CPC Computer with CP/M License, if you want to copy CP/M to an image.

### Main features:

* Vortex images are supported
* User areas, file attributes and wildcards are fully supported
* Comfortable command line interface including history, extensible help,
  and many options to each command
* Special commands for getting an insight in the filesystem structure

### Some commands:

* GET, PUT, MGET, MPUT transfer files in an FTP-like manner
* COPY, REN, ERA, ATTRIB manipulate files within the image
* TYPE shows the contents of files, either text or as hex dump
* LDIR, LCD, ! (Shell escape) access your local PC
* COMMENT describes your images or places a time stamp in it

*See the [INTRO](https://github.com/derikz/cpcfs/blob/master/INTRO) file for a
complete overview*

*Read the documentation in [cpcfs.doc](https://github.com/derikz/cpcfs/blob/master/cpcfs.doc)
(it's already contained in the zip file)*
