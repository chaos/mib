#*****************************************************************************\
#*  $Id: Makefile.in,v 1.9 2005/11/30 20:24:57 auselton Exp $
#*****************************************************************************
#*  Copyright (C) 2006 The Regents of the University of California.
#*  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
#*  Written by Andrew Uselton <uselton2@llnl.gov>
#*  UCRL-CODE-2006-xxx.
#*  
#*  This file is part of Mib, an MPI-based parallel I/O benchamrk
#*  For details, see <http://www.llnl.gov/linux/mib/>.
#*  
#*  Mib is free software; you can redistribute it and/or modify it under
#*  the terms of the GNU General Public License (as published by the Free
#*  Software Foundation) version 2, dated June 1991.
#*  
#*  Mib is distributed in the hope that it will be useful, but WITHOUT 
#*  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
#*  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
#*  for more details.
#*  
#*  You should have received a copy of the GNU General Public License along
#*  with Mib; if not, write to the Free Software Foundation, Inc.,
#*  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
#*****************************************************************************/

PROJECT       := mib
VERSION       := $(shell awk '/[Vv]ersion:/ {print $$2}' META)
RELEASE       := $(shell awk '/[Rr]elease:/ {print $$2}' META)
SVNURL        := https://eris.llnl.gov/svn/lustre-utils/$(PROJECT)
TRUNKURL      := $(SVNURL)/trunk
TAGURL        := $(SVNURL)/tags/$(PROJECT)-$(VERSION)-$(RELEASE)

SHELL=   /bin/sh
MAKE=    /usr/bin/make
CC = %CC%
INSTALL= /usr/bin/install -c
mkinstalldirs=	 $(SHELL) $(top_srcdir)/aux/mkinstalldirs

MPI_CCFLAGS = -DUSE_STDARG -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STDARG_H=1 -DUSE_STDARG=1 -DMALLOC_RET_VOID=1 
CCFLAGS = ${MPI_CCFLAGS}  -Wextra -g -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -rdynamic
# I don't think it turned out to need the following:
LDFLAGS = #-Wl,-rpath-link -Wl,/usr/lib/mpi/mpi_intel/lib 
LIBDIRS = #-L/usr/lib/mpi/mpi_intel/lib 
LIBS = %LIBS%
MPI_INCDIR = -I/usr/lib/mpi/include
INCDIR = ${MPI_INCDIR}
DEBUG = # Put 'DEBUG="-DDEBUG_CODE=1"' on the command line if you want 
	# debugging, or you can put it here if you really want to be sure
	# and always have the debugging on.  N.B. I don't think it even
	# works right now.

top_srcdir	=	.
prefix		=	/usr
exec_prefix	=	${prefix}
bindir		=	${exec_prefix}/bin
sbindir		= 	${exec_prefix}/sbin
mandir		=	$(prefix)/man
etcdir		=	/etc
packagedir	=	${etcdir}/${PROJECT}
scriptdir       =       /var/lustredata/scripts
docdir          =       ${prefix}/share/doc/${PROJECT}-${VERSION}

HEADERS = miberr.h mpi_wrap.h sys_wrap.h options.h mib_timer.h slurm.h config.h
SOURCES = mib.c mpi_wrap.c sys_wrap.c miberr.c options.c mib_timer.c slurm.c
OBJECTS = $(SOURCES:.c=.o)

all: mib 

mib: $(OBJECTS) $(HEADERS)
	$(CC) $(LDFLAGS) $(LIBDIRS) $(OBJECTS) -o $@ $(LIBS)

# You have to use a dummy target because there really is a "doc" driectory
doc: doc_dummy

doc_dummy:
	$(MAKE) -C doc

tools: tools-dummy

tools-dummy:
	$(MAKE) -C tools

install: mib doc
	$(mkinstalldirs) 			$(DESTDIR)$(bindir)
	$(INSTALL) mib				$(DESTDIR)$(bindir)/
	$(mkinstalldirs)			$(DESTDIR)$(mandir)/man1
	$(INSTALL) mib.1 -m 644			$(DESTDIR)$(mandir)/man1/
	$(mkinstalldirs)			$(DESTDIR)$(scriptdir)
	$(INSTALL) tools/composite.pl		$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/diff.pl		$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/file-layout.sh		$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/hullo-lustre.sh	$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/lfind-dist1.pl		$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/lfind-dist2.pl		$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/lfind-dist3.pl		$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/lfinding.sh		$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/lwatch.pl		$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/lwatch.py		$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/mib-test.sh		$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/mib-test.transcript	$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/mib.sh			$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/profile.pl		$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/rm-files.sh		$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/sum.pl			$(DESTDIR)$(scriptdir)/
	$(INSTALL) tools/viewcalls.pl		$(DESTDIR)$(scriptdir)/
	$(mkinstalldirs) 			$(DESTDIR)$(docdir)
	$(INSTALL) doc/mib.pdf			$(DESTDIR)$(docdir)/
	$(INSTALL) doc/sample.png		$(DESTDIR)$(docdir)/
	$(INSTALL) README			$(DESTDIR)$(docdir)/
	$(INSTALL) ChangeLog			$(DESTDIR)$(docdir)/
	$(INSTALL) COPYING			$(DESTDIR)$(docdir)/
	$(INSTALL) DISCLAIMER			$(DESTDIR)$(docdir)/

.c.o:
	$(CC) $(CCFLAGS)  $(INCDIR) $(DEBUG) -c $<

clean: 
	rm -rf $(PROJECT)+chaos
	rm -f *.rpm
	$(MAKE) -C doc clean
	$(MAKE) -C tools clean
	rm -f *.o mib *~

distclean: clean
	$(MAKE) -C doc distclean
	$(MAKE) -C tools distclean
	rm Makefile config.h mib.spec

apply-patches quilt: $(PROJECT)+chaos

$(PROJECT)+chaos: patches/series
	make clean
	@scripts/quilt.sh $(QUILTFLAGS)

check-vars:
	@echo "Release:  $(PROJECT)-$(VERSION)-$(RELEASE)"
	@echo "Trunk:    $(TRUNKURL)"
	@echo "Tag:      $(TAGURL)"

rpms-working:
	make clean
	@scripts/build --snapshot .

rpms-trunk: check-vars
	@scripts/build --snapshot $(TRUNKURL)

rpms-release: check-vars
	@scripts/build --nosnapshot $(TAGURL)

tagrel:
	svn copy $(TRUNKURL) $(TAGURL)