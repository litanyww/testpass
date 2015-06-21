#! /bin/sh
aclocal
autoconf
touch NEWS ChangeLog
test -e README || ln -s README.md README
test -e COPYING || ln -s COPYRIGHT COPYING
automake --add-missing
