#!/bin/sh

# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

THEDIR=`pwd`
cd $srcdir

RC=0

gnu="ftp://ftp.gnu.org/pub/gnu/"

for command in autoconf automake libtoolize; do
        pkg=$command
        case $command in
                libtoolize) pkg=libtool;;
        esac
        URL=$gnu/$pkg/
        if ! $command --version </dev/null >/dev/null 2>&1; then
                RC=$?
cat << !EOF >&2

You must have $pkg installed to compile the vanessa_logger.  
Download the appropriate package for your system, 
or get the source tarball at: $URL
!EOF
        fi
done

if [ $RC != 0 ]; then
        exit $RC
fi


if [ -z "$*" ]; then
	echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

case $CC in
xlc )
    am_opt=--include-deps;;
esac

aclocal $ACLOCAL_FLAGS
(autoheader --version)  < /dev/null > /dev/null 2>&1 && autoheader
automake --add-missing $am_opt
autoconf
cd $THEDIR

$srcdir/configure "$@" || exit -1

echo 
echo "Now type 'make' to compile vanessa_logger."
