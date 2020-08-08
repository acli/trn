all: patchlevel2.h real-all

real-all:
	make -f Makefile all

install: all real-install

real-install:
	make -f Makefile install

patchlevel2.h: patchlevel.h support/hacking/gen_patchlevel .git/index $(wildcard *.o)
	perl support/hacking/gen_patchlevel > $@.tmp && if [ -f $@ ] && diff $@ $@.tmp; then rm -f $@.tmp; else mv -f $@.tmp $@; rm -f trn.o Pnews Pnews.header newsnews; fi

.PHONEY: all real-all install real-install
.DELETE_ON_ERROR:
