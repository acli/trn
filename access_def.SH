: see end of file for revision information
case $CONFIG in
    '') . ./config.sh ;;
esac
echo "Extracting access.def (with variable substitutions)"
cat >access.def <<!GROK!THIS!
# This is a sample access.def file.

#   This section defines the available news sources and gives them an
# easy-to-use [ID] name.  The always-present ID, "default", can also be
# overriden here, if you so desire.
#   When defining an NNTP server, set "NNTP Server" to the domain name of
# your server.  If you need to use a non-standard port number, append ";119"
# (for example) to the name.  If you want to cache a local copy of the
# active file, set "Active File" to the appropriate name.  If you want
# the file refetched after certain intervals, set "Active File Refetch"
# to the appropriate interval (e.g. "1 day, 5 hours, 3 minutes").
#   When defining a local news source (even NFS mounted), set "Active File"
# and "Spool Dir".
#   Specify "none" for "Overview Dir" and/or "Thread Dir" to disable support
# for that database type, omit or leave blank for the default access (either
# "Spool Dir" for local or XOVER/XTHREAD for remote), or enter a custom
# hierarchy path name.
#   Specify "none" for "Active Times" if the host has no new-group support,
# leave it blank for the default (either active + ".times" for local access
# or the NEWGROUPS command for remote), or enter a custom file name for your
# active.times file.
#   Specify "none" for "Group Desc" if you don't have a file with newsgroup
# descriptions, leave it blank for the default (NEWSLIB/newsgroups for local
# access or the XGTITLE/LIST_NEWSGROUPS command for remote), or enter a
# custom file name for your newsgroups file.  If this is an NNTP source
# you can also set "Group Desc Refetch" to a time value when the file
# expires (something like "7 days" might be good).

#[default]
#NNTP Server = $servername
#Active File = $active
#Active File Refetch = [never|1 day, 4 hours, 5 mins]
#Spool Dir = $newsspool
#Thread Dir = $threaddir
#Overview Dir = $overviewdir
#Overview Format File = $overviewfmt
#Active Times = $acttimes
#Group Desc = $groupdesc
#Group Desc Refetch = [never|7 days 1 hour 90 minutes]
#Auth User =
#Auth Password =
#Auth Command =
#Force Auth = [yes|no}
#XHDR Broken = [yes|no]

# This section tells trn what newsrc groups you want to use.  If you
# define multiple entries with the same group number, all of the .newsrcs
# are combined and used together.  The first entry encountered will be
# the first group that trn will open.  Otherwise the numbers dictate the
# presentation order (see Ctrl-N and Ctrl-P).  You can have multiple
# .newsrc's from the same server or different servers, however you like.
# The ID name "default" always exists with your compiled-in defaults.

# If you leave off the newsrc name, the default is supplied.  If you
# don't want any groups added to an rc, specify "no" in the "Add Groups"
# field.  If you don't want new-group checking but want to manually
# add groups, specify "manual".  The default is "yes".

#[Group 1]
#ID = default
#Newsrc = ~/.newsrc
#Add Groups = yes/no/manual

[Group 1]
ID = default
!GROK!THIS!
#
# This software is copyrighted as detailed in the LICENSE file.
