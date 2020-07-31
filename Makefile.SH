case $CONFIG in
    '') . ./config.sh ;;
esac
echo "Extracting Makefile (with variable substitutions)"
if test "X$src" = "X." ; then
	vpath=''
	vincludes=''
	srcdir=`pwd`
	extra_cleanup=
else
	vpath="$src"
	vincludes="-I. -I$src"
	case "$src" in
	/*) srcdir="$src";;
	*) srcdir=`pwd`/$src ;;
	esac
	extra_cleanup=HelpFiles
fi
$cat >Makefile <<!GROK!THIS!
# Makefile.SH
#
# This software is copyrighted as detailed in the LICENSE file.
# 
# optional defines you can add to the CFLAGS
#    -DDEBUG                      compile in support for the -D option
#    -DNO_LISTGROUP               avoids attempting to use LISTGROUP
#    -DREPLYTO_POSTER_CHECKING    makes -p also check Reply-To (slow)

srcdir = $srcdir
VPATH = $vpath
extra_cleanup = $extra_cleanup

CC = $cc
#MSDOSLINK = tlink
TOUCH = touch
YACC = $yacc
#LINT = lint
lintflags = -phbvxac $vincludes

CFLAGS = $optimize $ccflags $vincludes
TKINC = $tkinc
LDFLAGS = $ldflags
TKLDFLAGS = 
#MSDOSLINKFLAGS = $linkflags

#MSDOSexe = .exe
o = o
#MSDOSo = obj

rnbin = $installbin
rnlib = $installprivlib
#INEWSinewsbin = $installinews
mansrc = $installmansrc
manext = $manext
#NNTPNNTPFLAGS = $nntpflags
ndirc = $ndirc
ndiro = $ndiro
strftimec = $strftimec
strftimeo = $strftimeo
installfilexp = $filexp

libs = $libndir $termlib $jobslib -lm $libs
tklibs = $tklibs
!GROK!THIS!
$cat >>Makefile <<'!NO!SUBS!'
public_backup = trn$(exe) trn-artchk$(exe) $(nntpexe) $(msdosexe)
public_diff = Pnews Rnmail
public = $(public_backup) $(public_diff)
private_preserve = access.def
private_diff = newsnews Speller Pnews.header
private_blast = norm.saver mbox.saver makedir filexp
private = $(private_preserve) $(private_diff) $(private_blast)
util = makedepend mkpro mktd HelpFiles/config/environment
manpages = trn.1 Pnews.1 Rnmail.1
#INEWSinewsexe=inews$(exe)
#NNTPnntpexe=nntplist$(exe)
#NNTPnntpsrc=nntpinit.c nntpclient.c nntpauth.c nntp.c
#NNTPnntpobj1=nntpinit.$(o) nntpclient.$(o) nntpauth.$(o)
#NNTPnntpobj2=nntp.$(o)
#MSDOSmsdosexe=winsock.dll
#MSDOSmsdossrc=popen.c
#MSDOSmsdosobj=popen.$(o)

h1 = addng.h art.h artio.h artsrch.h autosub.h backpage.h bits.h cache.h
h2 = charsubst.h datasrc.h common.h decode.h env.h final.h hash.h head.h
h3 = help.h init.h intrp.h kfile.h last.h list.h mime.h ng.h ngdata.h ngsrch.h
h4 = ngstuff.h only.h opt.h rcln.h rcstuff.h respond.h rthread.h rt-mt.h
h5 = rt-ov.h rt-page.h rt-process.h rt-select.h rt-util.h rt-wumpus.h
h6 = search.h sw.h term.h trn.h util.h util2.h wildmat.h color.h filter.h
h7a = scan.h scmd.h sdisp.h smisc.h sorder.h spage.h
h7b = scanart.h samain.h samisc.h sadisp.h sacmd.h sadesc.h sathread.h
h7c = url.h mempool.h univ.h
h7all = $(h7a) $(h7b) $(h7c)
h8 = score.h scorefile.h scoresave.h score-easy.h
h9 = tkstuff.h tktree.h

h = $(h1) $(h2) $(h3) $(h4) $(h5) $(h6) $(h7all) $(h8) $(h9)

c1 = addng.c art.c artio.c artsrch.c autosub.c backpage.c bits.c cache.c
c2 = charsubst.c datasrc.c decode.c edit_dist.c env.c final.c hash.c head.c
c3 = help.c init.c intrp.c kfile.c last.c list.c $(ndirc) mime.c ng.c ngdata.c
c4 = ngsrch.c ngstuff.c only.c opt.c rcln.c rcstuff.c respond.c rthread.c
c5 = rt-mt.c rt-ov.c rt-process.c rt-page.c rt-select.c rt-util.c rt-wumpus.c
c6 = search.c $(strftimec) sw.c term.c trn.c util.c util2.c
c7 = uudecode.c $(nntpsrc) $(msdossrc) wildmat.c color.c filter.c
c8a = scan.c scmd.c sdisp.c smisc.c sorder.c spage.c
c8b = scanart.c samain.c samisc.c sadisp.c sacmd.c sadesc.c sathread.c
c8c = url.c mempool.c univ.c
c8all = $(c8a) $(c8b) $(c8c)
c9 = score.c scorefile.c scoresave.c score-easy.c
c10 = tkstuff.c tktree.c
c = $(c1) $(c2) $(c3) $(c4) $(c5) $(c6) $(c7) $(c8all) $(c9) $(c10)

obj1 = addng.$(o) art.$(o) artio.$(o) artsrch.$(o) autosub.$(o) backpage.$(o)
obj2 = bits.$(o) cache.$(o) charsubst.$(o) datasrc.$(o) decode.$(o)
obj3 = edit_dist.$(o) env.$(o) final.$(o) hash.$(o) head.$(o) help.$(o)
obj4 = init.$(o) intrp.$(o) kfile.$(o) last.$(o) list.$(o) $(ndiro) mime.$(o) ng.$(o)
obj5 = ngdata.$(o) ngsrch.$(o) ngstuff.$(o) only.$(o) opt.$(o) rcln.$(o)
obj6 = rcstuff.$(o) respond.$(o) rthread.$(o) rt-mt.$(o) rt-ov.$(o)
obj7 = rt-process.$(o) rt-page.$(o) rt-select.$(o) rt-util.$(o) rt-wumpus.$(o)
obj8 = search.$(o) $(strftimeo) sw.$(o) term.$(o) trn.$(o) util.$(o) util2.$(o)
obj9 = uudecode.$(o) parsedate.$(o) $(nntpobj1) $(nntpobj2)
obj10= $(msdosobj) wildmat.$(o) color.$(o) filter.$(o)
obj11a = scan.$(o) scmd.$(o) sdisp.$(o) smisc.$(o) sorder.$(o) spage.$(o)
obj11b = scanart.$(o) samain.$(o) samisc.$(o) sadisp.$(o) sacmd.$(o) sadesc.$(o) sathread.$(o)
obj11c = url.$(o) mempool.$(o) univ.$(o)
obj11all = $(obj11a) $(obj11b) $(obj11c)
obj12 = score.$(o) scorefile.$(o) scoresave.$(o) score-easy.$(o)
obj13 = tkstuff.$(o) tktree.$(o)

obj = $(obj1) $(obj2) $(obj3) $(obj4) $(obj5) $(obj6) $(obj7) $(obj8) \
	$(obj9) $(obj10) $(obj11all) $(obj12) $(obj13)

addedbymake = $(public) $(inewsexe) $(private) $(util)\
	Makefile.old config.h parsedate.c cppstdin myinstall all

# grrr
SHELL = /bin/sh

.c.$(o):
	$(CC) -c $(CFLAGS) $<

all: Makefile $(public) $(inewsexe) $(private) $(util) Policy.sh
	$(TOUCH) all
#MSDOS
#MSDOStrn: trn.exe

trn$(exe): $(obj)
#MSDOS	$(LINK) $(LINKFLAGS) @trn.lnk
#MSDOS
#MSDOSjunk1:
	$(CC) $(LDFLAGS) $(TKLDFLAGS) $(obj) $(tklibs) $(libs) -o trn$(exe)
#NNTP
#NNTPnntpinit.$(o): nntpinit.c
#NNTP	$(CC) -c $(CFLAGS) $(NNTPFLAGS) $<
#NNTP#MSDOS
#NNTP#MSDOSnntplist: nntplist.exe
#NNTP
#NNTPnntplistobjs = nntplist.$(o) $(nntpobj1) util2.$(o) util3.$(o) \
#NNTP	wildmat.$(o)
#NNTP
#NNTPnntplist$(exe): $(nntplistobjs)
#NNTP#MSDOS	$(LINK) $(LINKFLAGS) @nntplist.lnk
#NNTP#MSDOS
#NNTP#MSDOSjunk2:
#NNTP	$(CC) $(LDFLAGS) $(nntplistobjs) -o nntplist$(exe) $(libs)
#INEWS#MSDOS
#INEWS#MSDOSinews: inews.exe
#INEWS
#INEWSinewsobjs = inews.$(o) $(nntpobj1) env.$(o) util2.$(o) util3.$(o)
#INEWS
#INEWSinews$(exe): $(inewsobjs)
#INEWS#MSDOS	$(LINK) $(LINKFLAGS) @inews.lnk
#INEWS#MSDOS
#INEWS#MSDOSjunk3:
#INEWS	$(CC) $(LDFLAGS) $(inewsobjs) -o inews$(exe) $(libs)

#MSDOStrn-artchk: trn-artchk.exe
#MSDOS
trnchkobjs = trn-artchk.$(o) $(nntpobj1) util2.$(o) util3.$(o)

trn-artchk$(exe): $(trnchkobjs)
#MSDOS	$(LINK) $(LINKFLAGS) @trn-artchk.lnk
#MSDOS
#MSDOSjunk4:
	$(CC) $(LDFLAGS) $(trnchkobjs) -o trn-artchk$(exe) $(libs)
#MSDOS
#MSDOSwinsock.dll: winsock.c winsock.def
#MSDOS	$(CC) -c -WXDE -ml -w-par winsock.c
#MSDOS	$(LINK) /c -v -Twd -Lc:\bc4\lib @winsock.lnk

tkstuff.o: tkstuff.c
	$(CC) -c $(CFLAGS) $(TKINC) $<

tktree.o: tktree.c
	$(CC) -c $(CFLAGS) $(TKINC) $<

parsedate.c: parsedate.y
	@echo 'Expect 6 shift/reduce conflicts'
	$(YACC) $(srcdir)/parsedate.y
	mv -f y.tab.c parsedate.c
#MSDOS
#MSDOSparsedate.$(o): parsedate.c
#MSDOS	$(CC) -c $(CFLAGS) -w- $<
#STRFTIME
#STRFTIME$(strftimeo): $(strftimec)
#STRFTIME	$(CC) -c $(CFLAGS) $(strftimec)

#MSDOSunipatch: unipatch.exe
#MSDOS
unipatch$(exe): support/unipatch.$(o)
#MSDOS	$(CC) $(LDFLAGS) support/unipatch.$(o)
#MSDOS
#MSDOSjunk5:
	$(CC) $(LDFLAGS) support/unipatch.$(o) -o unipatch$(exe)

Pnews.header: Pnews

# if a .h file depends on another .h file...
$(h):
	-$(TOUCH) $@

install: $(public) $(inewsexe) $(private) $(manpages) myinstall
# won't work with csh
	export PATH || false # stop the make unless we are compatible
	- ./makedir `./filexp $(rnbin)`
	./myinstall -sox `./filexp $(rnbin)` $(public_backup)
	./myinstall -dox `./filexp $(rnbin)` $(public_diff)
	- if test $(installfilexp) = Rnfilexp; then\
 ./myinstall -xf Rnfilexp `./filexp $(rnbin)` ./filexp; fi
	- ./makedir `./filexp $(rnlib)`
	- chmod o+r `./filexp $(rnlib)`
#INEWS	./myinstall -sox `./filexp $(inewsbin)` inews$(exe)
	./myinstall -x   `./filexp $(rnlib)`    $(private_blast)
	- if test "X$(mansrc)" != X -a "X`pwd`" != X`./filexp $(mansrc)`; then\
  ./makedir `./filexp $(mansrc)`;\
  for page in $(manpages); do\
   page=`basename $$page`;\
   dest=$(mansrc)/`basename $$page .1`.$(manext);\
   rm -f $$dest; cp $(srcdir)/$$page $$dest; chmod 444 $$dest;\
  done;\
 fi
	./myinstall -dor `./filexp $(rnlib)` $(private_diff)
	./myinstall -dnr `./filexp $(rnlib)` $(srcdir)/INIT $(private_preserve)
	./myinstall -oR `./filexp $(rnlib)` `pwd`/HelpFiles
	@echo ' '
	@echo 'Use "make installclean" to remove any .old or .new files that'
	@echo 'were just created, but only after you are sure that the files'
	@echo 'contain no interesting changes and/or no one is running the'
	@echo 'old executables.'

installclean:
	- (cd `./filexp $(rnbin)` ; for file in $(public) $(private_diff) ; do\
  rm -f $${file}.old;\
 done)
	- (cd `./filexp $(rnlib)` ; for file in HelpFiles ; do\
  rm -rf $${file}.old;\
 done)
	- (cd `./filexp $(rnlib)` ; for file in INIT $(private_preserve) ; do\
  rm -f $${file}.new;\
 done)
#INEWS	- (cd `./filexp $(inewsbin)` ; rm -f inews$(exe).old)

clean:
	@echo 'Use "make realclean" to also remove the Makefile.'
	@echo 'Use "make spotless" to even remove config.sh, Policy.sh, and .config.'
	rm -rf UU $(extra_cleanup)
	rm -f *.$(o) core $(addedbymake)

realclean:
	@echo 'You can use "Configure -S" to reverse this.'
	rm -rf UU
	rm -f *.$(o) core $(addedbymake) Makefile

spotless:
	rm -rf UU .config
	rm -f *.$(o) core $(addedbymake) Makefile config.sh Policy.sh

# The following lint has practically everything turned on.  Unfortunately,
# you have to wade through a lot of mumbo jumbo that can't be suppressed.
# If the source file has a /*NOSTRICT*/ somewhere, ignore the lint message
# for that spot.

lint: $(c) parsedate.c
	$(LINT) $(lintflags) $(defs) $? > trn.fuzz 2>&1

sabertrn: $(c) parsedate.c
	#load $(c) parsedate.c $(libs)

depend: config.h Makefile makedepend
	./makedepend

pro: config.h mkpro
	./mkpro

td: config.h mktd
	./mktd

Makefile: Makefile.SH config.sh dependencies
	/bin/sh $(srcdir)/Makefile.SH
	@echo "The Makefile has changed -- please restart the make."
	false # stop the make

Pnews: patchlevel.h
newsnews: patchlevel.h

tar: MANIFEST patchlevel.h
	export PATH || false # stop the make unless we are compatible
	@(ver=trn-`awk '{print $$4}' $(srcdir)/patchlevel.h` ; \
	 cd $(srcdir)/.. ; \
	 echo Creating $$ver.tar.gz ; \
	 ln -s trn4 $$ver ; \
	 tar cvzf $$ver.tar.gz \
		`ls -1d trn4/hints/* | fgrep -v /CVS | \
		 cat trn4/MANIFEST - | \
		 sed -e "s:^\(trn4/\)*\([^ 	]*\).*:$$ver/\2:"` ; \
	 rm $$ver )

# AUTOMATICALLY GENERATED MAKE DEPENDENCIES--PUT NOTHING BELOW THIS LINE
!NO!SUBS!
if test -f dependencies; then
	$cat dependencies >>Makefile
else
	$cat $src/dependencies >>Makefile
fi
case "$d_nntp" in
define)	sedcmd="-e '/^#NNTP/s/^#NNTP//'" ;;
*)	sedcmd="-e '/^#NNTP/d'" ;;
esac
case "$d_inews" in
define)	sedcmd="$sedcmd -e '/^#INEWS/s/^#INEWS//'" ;;
*)	sedcmd="$sedcmd -e '/^#INEWS/d'" ;;
esac
case "$d_msdos" in
define)	sedcmd="$sedcmd -e '/^#MSDOS/s/^#MSDOS//'" ;;
*)	sedcmd="$sedcmd -e '/^#MSDOS/d'" ;;
esac
case "$strftimec" in
''|' ')	sedcmd="$sedcmd -e '/^#STRFTIME/d'" ;;
*)	sedcmd="$sedcmd -e '/^#STRFTIME/s/^#STRFTIME//'" ;;
esac
eval "sed <Makefile $sedcmd >Makefile.new"
mv Makefile.new Makefile
$eunicefix Makefile
