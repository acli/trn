Unicode-patched Threaded Read News (trn) 4.0 test77
===================================================

This is a version of trn-4.0-test77 that has been patched to display UTF-8 reasonably correctly.
Right now UTF-8 is correctly displaying both in articles and in headers.
However, the original “character set” conversions have been disabled in the process.

The next step, in terms of character set handling would be to put the original conversions back
in a way that doesn’t corrupt UTF-8 again.

The original README from 4.0-test77 is in [README](README).
