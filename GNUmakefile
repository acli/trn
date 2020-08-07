all: patchlevel2.h
	make -f Makefile

install: all
	make -f Makefile install

patchlevel2.h: patchlevel.h support/hacking/gen_patchlevel .git/index $(wildcard *.o)
	perl support/hacking/gen_patchlevel > $@.tmp && if [ -f $@ ] && diff $@ $@.tmp; then rm $@.tmp; else mv $@.tmp $@; fi

