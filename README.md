Unicode-patched Threaded Read News (trn) 4.0 test77
===================================================

This is a version of trn-4.0-test77 that has been patched to display UTF-8 reasonably correctly.
However, the original “character set” conversions are currently disabled.
Bugs that have nothing to do with UTF-8 support are also being worked on,
as this is the newsreader I’m actually using
(yes, I’ve tried tin and nn, and [no, tin is not easier and nn is not better](https://github.com/acli/trn/wiki/Project-whiteboard)).

Posting is half-working, with Content-Transfer-Encoding declared as 8bit.
A more proper fix is the next step.

Further along, the original conversions need to be put back in,
but in a way that wouldn’t corrupt UTF-8.

The original README from 4.0-test77 is in [README](README).
