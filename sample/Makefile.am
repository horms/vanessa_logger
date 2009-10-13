######################################################################
# Makefile.am                                           September 2000
# Simon Horman                                      horms@verge.net.au
#
# vanessa_logger
# Generic logging layer
# Copyright (C) 2000-2008  Simon Horman <horms@verge.net.au>
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
#
######################################################################

bin_PROGRAMS = vanessa_logger_sample

man_MANS = vanessa_logger_sample.1

EXTRA_DIST = vanessa_logger_sample_config.h.in vanessa_logger_sample.1

vanessa_logger_sample_SOURCES = \
  vanessa_logger_sample.c \
  vanessa_logger_sample_config.h \
  unused.h

INCLUDES= -I$(top_srcdir)/libvanessa_logger

vanessa_logger_sample_LDADD = \
-L../libvanessa_logger \
-L../libvanessa_logger/.libs/ \
-lvanessa_logger
