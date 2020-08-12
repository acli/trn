all: patchlevel2.h real-all

real-all:
	make -f Makefile all
	make mimeify

install: all real-install

real-install:
	make -f Makefile install

test: all
	make -Ctests

# Temporary rule to create patchlevel2.h before I can figure out how to modify the real makefile

patchlevel2.h: patchlevel.h support/hacking/gen_patchlevel .git/index $(wildcard *.o)
	perl support/hacking/gen_patchlevel > $@.tmp && if [ -f $@ ] && diff $@ $@.tmp; then rm -f $@.tmp; else mv -f $@.tmp $@; rm -f trn.o Pnews Pnews.header newsnews; fi

# Temporary rule to create mimeify before I can figure out how to modify the real makefile

mimeify: mimeify.o utf.o

.PHONEY: all real-all install real-install test
.DELETE_ON_ERROR:
