/***********************************************************************
 * vanessa_logger.h                                       September 2000
 * Horms                                              horms@vergenet.net
 *
 * vanessa_logger
 * Generic logging layer
 * Copyright (C) 2000-2002  Horms
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA
 *
 **********************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

#ifndef VANESSA_LOGGER_FLIM
#define VANESSA_LOGGER_FLIM

typedef void vanessa_logger_t;

typedef int
 (*vanessa_logger_log_function_t) (int priority, const char *fmt, ...);

typedef unsigned int vanessa_logger_flag_t;


/**********************************************************************
 * vanessa_logger_openlog_syslog
 * Exported function to open a logger that will log to syslog
 * pre: facility: facility to log to syslog with
 *      ident: Identity to prepend to each log
 *      max_priority: Maximum priority no to log
 *                    Priorities are integers, the levels listed
 *                    in syslog(3) should be used for a syslog logger
 *      option: options to pass to the openlog command
 *              Will be logically ored with LOG_PID
 * post: Logger is opened
 * return: pointer to logger
 *         NULL on error
 **********************************************************************/

vanessa_logger_t *vanessa_logger_openlog_syslog(const int facility,
						const char *ident,
						const int max_priority,
						const int option);


/**********************************************************************
 * vanessa_logger_openlog_syslog_byname
 * Exported function to open a logger that will log to syslog
 * pre: facility_name: Name of facility to log to syslog with
 *      ident: Identity to prepend to each log
 *      max_priority: Maximum priority no to log
 *                    Priorities are integers, the levels listed
 *                    in syslog(3) should be used for a syslog logger
 *      option: options to pass to the openlog command
 *              Will be logically ored with LOG_PID
 * post: Logger is opened
 * return: pointer to logger
 *         NULL on error
 **********************************************************************/

vanessa_logger_t *vanessa_logger_openlog_syslog_byname(const char
						       *facility_name,
						       const char *ident,
						       const int
						       max_priority,
						       const int option);


/**********************************************************************
 * vanessa_logger_openlog_filehandle
 * Exported function to open a logger that will log to a filehandle
 * pre: filehandle: open filehandle to log to
 *      ident: Identity to prepend to each log
 *      max_priority: Maximum priority number to log
 *                    Priorities are integers, the levels listed
 *                    in syslog(3) should be used for a syslog logger
 *      option: ignored
 * post: Logger is opened
 * return: pointer to logger
 *         NULL on error
 **********************************************************************/

vanessa_logger_t *vanessa_logger_openlog_filehandle(FILE * filehandle,
						    const char *ident,
						    const int max_priority,
						    const int option);


/**********************************************************************
 * vanessa_logger_openlog_filename
 * Exported function to open a logger that will log to a filename
 *          that will be opened
 * pre: filename: filename to log to
 *      ident: Identity to prepend to each log
 *      max_priority: Maximum priority number to log
 *                    Priorities are integers, the levels listed
 *                    in syslog(3) should be used for a syslog logger
 *      option: ignored
 * post: Logger is opened
 * return: pointer to logger
 *         NULL on error
 **********************************************************************/

vanessa_logger_t *vanessa_logger_openlog_filename(const char *filename,
						  const char *ident,
						  const int max_priority,
						  const int option);


/**********************************************************************
 * vanessa_logger_openlog_function
 * Exported function to open a logger that will log to a given function
 * pre: function: function to use for logging
 *      ident: Identity to prepend to each log
 *      max_priority: Maximum priority number to log
 *                    Priorities are integers, the levels listed
 *                    in syslog(3) should be used for a syslog logger
 *      option: ignored
 * post: Logger is opened
 * return: pointer to logger
 *         NULL on error
 **********************************************************************/

vanessa_logger_t
    *vanessa_logger_openlog_function(vanessa_logger_log_function_t
				     log_function, const char *ident,
				     const int max_priority,
				     const int option);


/**********************************************************************
 * vanessa_logger_closelog
 * Exported function to close a logger
 * pre: vl: pointer to logger to close
 * post: logger is closed and memory is freed
 * return: none
 **********************************************************************/

void vanessa_logger_closelog(vanessa_logger_t * vl);


/**********************************************************************
 * vanessa_logger_change_max_priority
 * Exported function to change the maximum priority that the logger
 * will log.
 * pre: vl: logger to change the maximum priority of
 *      max_priority: Maximum priority number to log
 *                    Priorities are integers, the levels listed
 *                    in syslog(3) should be used for a syslog logger
 * post: maximum priority of logger is changed
 *       nothing if vl is NULL
 * return: none
 **********************************************************************/

void vanessa_logger_change_max_priority(vanessa_logger_t * vl,
					const int max_priority);


/**********************************************************************
 * vanessa_logger_log
 * Exported function to log a message
 * pre: vl: pointer to logger to log to
 *      priority: Priority to log with.
 *                If priority is more than max_priority as provided to
 *                vanessa_logger_openlog_filehandle, 
 *                vanessa_logger_openlog_filename or
 *                vanessa_logger_openlog_syslog. 
 *                Levels described in syslog(3) should be used for
 *                syslog loggers as the priority will be used when
 *                logging to syslog. These priorities may also be
 *                used for filehandle and filename loggers.
 *                Strangely with syslog higher priorities have
 *                _lower_ priority numbers. For this
 *                reason vanessa_logger regards messages with
 *                lower priority numbers as being higher priority
 *                than messages with lower priority. I suggest
 *                just using the syslog priorities to avoid confusion.
 *      fmt: format of message to log as per sprintf(3) for
 *           filename and filehandle loggers and as
 *           per syslog(3) for syslog loggers
 *      ...: data for fmt
 * post: Message is logged
 * return: none
 **********************************************************************/

void
vanessa_logger_log(vanessa_logger_t * vl, int priority, const char *fmt,
		   ...);


/**********************************************************************
 * vanessa_logger_logv
 * Exported function to log a message
 * Same as vanessa_logger_logv but a va_list is given instead
 * of a variable number of arguments.
 **********************************************************************/

void vanessa_logger_logv(vanessa_logger_t * vl,
			 int priority, char *fmt, va_list ap);


/**********************************************************************
 * vanessa_logger_reopen
 * Exported function to reopen a logger
 * pre: vl: pointer to logger to reopen
 * post: logger is reopened
 * return: 0 on success
 *         -1 on error
 *
 * Note: May be used as part of a signal handler to reopen logger
 *       on for instance receiving a SIGHUP
 **********************************************************************/

int vanessa_logger_reopen(vanessa_logger_t * vl);


/**********************************************************************
 * vanessa_logger_str_dump
 * Sanitise a buffer into ASCII
 * pre: vl: Vanessa logger to log errors to. May be NULL.
 *      buffer: buffer to sanitise
 *      size: number of bytes in buffer to sanitise
 *      flag: Unused, should be set to 0.
 * post: a new buffer is alocated. For each byte in buffer
 *       that is a printable ASCII character it is added to
 *       the new buffer. All other characters are represented
 *       in the new buffer as octal, in the form \xxx.
 * return: the new buffer, this should be freed by the caller
 *         NULL on error
 **********************************************************************/

char *vanessa_logger_str_dump(vanessa_logger_t * vl,
		const unsigned char *buffer, const size_t buffer_length,
		vanessa_logger_flag_t flag);

#endif
