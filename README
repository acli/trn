			  Trn Kit, Version 4.0

		    Copyright (c) 1995, Wayne Davison

			Based on rn, Version 4.4

		     Copyright (c) 1985, Larry Wall
                     Copyright (c) 1991, Stan Barber

You may copy the trn kit in whole or in part as long as you don't try to
make money off it, or pretend that you wrote it.
--------------------------------------------------------------------------

See the file INSTALL for installation instructions.  Failure to do so
may void your warranty. :-)

After you have unpacked your kit, you should have all the files listed
in MANIFEST (Configure checks this for you).

If you're unsure if you have the latest release, check ftp.uu.net:

    ftp.uu.net:networking/news/readers/trn/trn.tar.gz -> [latest.version].gz

[A .Z version is also available and mthreads resides in the same spot,
if you need it.]

What is trn?
------------
Trn is Threaded RN -- a newsreader that uses an article's references to
order the discussions in a very natural, reply-ordered sequence called
threads.  Having the replies associated with their parent articles not
only makes following the discussion easier, but also makes it easy to back-
track and (re-)read a specific discussion from the beginning.  Trn also
has a visual representation of the current thread in the upper right corner
of the header, which will give you a feel for how the discussion is going
and how the current article is related to the last one you read.

In addition, a thread selector makes it easy to browse through a large
group looking for interesting articles.  You can even browse through the
articles you've already read and select the one(s) you wish to read again.
Other nice features include the extract commands for the source and binary
groups, thread-oriented kill directives, a better newgroup finding strategy,
and lots more.  See the change-log for a list of the things that are new to
trn 4.0 from previous versions (either view the file HelpFiles/changelog
or type 'h' (help) from inside trn and select the "What's New?" entry).

To make trn work faster you will probably want to create an auxiliary news
database that summarises the available articles.  Trn know how to use two
different kinds (so far):  thread files, which are maintained by the mthreads
package and typically requires 3-5% of your newsspool size in disk storage;
and overview files, which are maintained by INN v1.3 (or greater) or a
modified version of C news and typically requires 8-10% of your newsspool
size in disk storage.  (Note that the space that mthreads saves you on your
disk is paid for by a higher demand on your cpu and disks while updating
the files.)  See the package of your choice for details on how to setup
the adjunct database, but it is not necessary to do this before trying
out trn.

Trn supports local news groups and news accessed remotely via NNTP.  If you
opt for remote access you will probably want to make the adjunct database
available too.  You can do this in a variety of way, but I recommend that
you send the database from the server to the client via NNTP.  To do this
you either need to use INN or a modified reference NNTP -- version 1.5.11-t5
is the latest as of this writing.  See ftp.uu.net:networking/news/nntp for
the file nntp-t5.tar.gz. This version supports the XOVER command (to send
overview files), the XTHREAD command (to send thread files), and the XINDEX
command (though trn doesn't support using it).  The alternative is to either
mount the disk containing your database via NFS, or build it locally.  See
the mthreads package for details on how to do this.

Note that trn is based on rn, and so it does a great job of pretending to
be rn for those people that simply don't like to change their newsreading
habits.  It is possible to install trn as both rn and trn linked together
and have it act as both newsreaders, thus saving you the hassle of maint-
aining two separate newsreaders.  A Configuration question will ask you if
you want trn to check its name on startup.

Where to send bug reports
-------------------------
Mail your bug reports to trn-workers@lists.sourceforge.net.  Use the
'v'ersion command from the newsgroup selection level of trn to be
reminded of this address and see some configuration information to
send along with your bug report.
