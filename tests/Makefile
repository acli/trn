CFLAGS=-I.. -O0 -g -Werror=incompatible-pointer-types -Wreturn-type -Werror=return-type
LDLIBS=-ltermcap

# Part 1 of some stupid magic to link in the entire trn code base minus main()
# This is needed because there's too much dependency in the .c files

everything=../filter.o ../sorder.o ../rcln.o ../sdisp.o ../rt-ov.o ../final.o ../hash.o ../nntpclient.o ../rthread.o ../scoresave.o ../sacmd.o ../backpage.o ../spage.o ../sadisp.o ../init.o ../ngstuff.o ../art.o ../rt-wumpus.o ../edit_dist.o ../list.o ../bits.o ../scan.o ../sw.o ../addng.o ../sathread.o ../uudecode.o ../score-easy.o ../sadesc.o ../datasrc.o ../tktree.o ../term.o ../artsrch.o ../rt-util.o ../ngdata.o ../only.o ../ngsrch.o ../intrp.o ../scorefile.o ../rt-mt.o ../nntpinit.o ../artio.o ../scanart.o ../head.o ../url.o ../mempool.o ../tkstuff.o ../samisc.o ../rt-page.o ../rt-select.o ../color.o ../nntpauth.o ../cache.o ../respond.o ../rt-process.o ../mime.o ../scmd.o ../nntp.o ../samain.o ../univ.o ../rcstuff.o ../smisc.o ../help.o ../util2.o ../ng.o ../charsubst.o ../search.o ../utf.o ../env.o ../opt.o ../kfile.o ../autosub.o ../score.o ../decode.o ../last.o ../util.o ../parsedate.o stub.o

# Normal rules start here

all:
	make test 2>&1|less

test: test_cache test_rt-util test_utf test_mimeify
	make -C..
	./test_cache
	./test_rt-util
	./test_utf
	./test_mimeify

test_cache: test_cache.o $(everything)
test_mimeify: test_mimeify.o
test_rt-util: test_rt-util.o $(everything)
test_utf: test_utf.o ../utf.o

../%.o: ../%.c
	make -C..

# Part 2 of stupid magic

stub.o: ../trn.c
	$(CC) -c $(CFLAGS) -Dmain=not_main -o$@ $<

.PHONEY: all test
