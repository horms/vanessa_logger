dnl vanessa_logger
dnl Generic logging layer
dnl Copyright (C) 2000-2008  Simon Horman <horms@verge.net.au>
dnl 
dnl This library is free software; you can redistribute it and/or modify it
dnl under the terms of the GNU Lesser General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or (at
dnl your option) any later version.
dnl 
dnl This library is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
dnl General Public License for more details.
dnl 
dnl You should have received a copy of the GNU Lesser General Public License
dnl along with this library; if not, write to the Free Software Foundation,
dnl Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA


AC_INIT(libvanessa_logger/vanessa_logger.h)
AM_CONFIG_HEADER(config.h)

AC_CONFIG_MACRO_DIR([libltdl/m4])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
LT_INIT
LTDL_INIT
AM_INIT_AUTOMAKE(vanessa_logger, 0.0.10)

AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_CHECK_DECL(facilitynames,
	AC_DEFINE(WITH_FACILITYNAMES,1,[Is facilitynames in syslog.h]), ,
	[ #define SYSLOG_NAMES 1
	  #include <stdlib.h>
	  #include <syslog.h> ])

AC_OUTPUT(
libvanessa_logger/Makefile 
sample/Makefile 
sample/vanessa_logger_sample_config.h 
Makefile
libvanessa_logger0.spec
debian/Makefile 
INSTALL
vanessa-logger.pc
)
