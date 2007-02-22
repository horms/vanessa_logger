/***********************************************************************
 * vanessa_logger.h                                       September 2000
 * Horms                                              horms@verge.net.au
 *
 * vanessa_logger
 * Generic logging layer
 * Copyright (C) 2000-2004  Horms
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
#include <string.h>
#include <errno.h>

#ifndef VANESSA_LOGGER_FLIM
#define VANESSA_LOGGER_FLIM

typedef void vanessa_logger_t;

typedef void (*vanessa_logger_log_function_va_t) 
		(int priority, const char *fmt, va_list ap);

/*
 * NB: There was a fundamental design error in the original
 * implementation of logging to functions such that it cannot work
 * with vanessa_logger's internal framework. Thus
 * vanessa_logger_log_function_t has been replaced by
 * vanessa_logger_log_function_va_t. Please do not use
 * vanessa_logger_log_function_t.
 *
 * I apologies that this breaks backwards compatibility, but
 * there is no way to make the original protoype work.
 * 
 * Perhaps there was something in the air on the train in Northern Germany
 * when I wrote this code. Perhaps I was just tired. But in any case
 * it was wrong.
 *
 * Horms, December 2002
 *
 * typedef int (*vanessa_logger_log_function_t) 
 *		(int priority, const char *fmt, ...);
 */

typedef unsigned int vanessa_logger_flag_t;


/**********************************************************************
 * Flags for filehandle or filename loggers
 **********************************************************************/

#define VANESSA_LOGGER_F_NONE         0x0  /* Default behaviour */
#define VANESSA_LOGGER_F_NO_IDENT_PID 0x1  /* Don't show ident and pid */
#define VANESSA_LOGGER_F_TIMESTAMP    0x2  /* Log the date and time */
#define VANESSA_LOGGER_F_CONS         0x4  /* Log to the console if there
					      is an error while writing 
					      to the filehandle or filename */
#define VANESSA_LOGGER_F_PERROR       0x8  /* Print to stderr as well */

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

vanessa_logger_t *
vanessa_logger_openlog_syslog(const int facility, const char *ident,
		const int max_priority, const int option);


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

vanessa_logger_t *
vanessa_logger_openlog_syslog_byname(const char *facility_name,
		const char *ident, const int max_priority, const int option);


/**********************************************************************
 * vanessa_logger_openlog_filehandle
 * Exported function to open a logger that will log to a filehandle
 * pre: filehandle: open filehandle to log to
 *      ident: Identity to prepend to each log
 *      max_priority: Maximum priority number to log
 *                    Priorities are integers, the levels listed
 *                    in syslog(3) should be used for a syslog logger
 *      flag: flags for logger
 *            See "Flags for filehandle or filename loggers"
 *            in vanessa_logger.h for valid flags
 * post: Logger is opened
 * return: pointer to logger
 *         NULL on error
 **********************************************************************/

vanessa_logger_t *
vanessa_logger_openlog_filehandle(FILE * filehandle, const char *ident,
		const int max_priority, const int flag);


/**********************************************************************
 * vanessa_logger_openlog_filename
 * Exported function to open a logger that will log to a filename
 *          that will be opened
 * pre: filename: filename to log to
 *      ident: Identity to prepend to each log
 *      max_priority: Maximum priority number to log
 *                    Priorities are integers, the levels listed
 *                    in syslog(3) should be used for a syslog logger
 *      flag: flags for logger
 *            See "Flags for filehandle or filename loggers"
 *            in vanessa_logger.h for valid flags
 * post: Logger is opened
 * return: pointer to logger
 *         NULL on error
 **********************************************************************/

vanessa_logger_t *
vanessa_logger_openlog_filename(const char *filename, const char *ident,
		const int max_priority, const int flag);


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

vanessa_logger_t *
vanessa_logger_openlog_function(vanessa_logger_log_function_va_t log_function, 
		const char *ident, const int max_priority, const int option);


/**********************************************************************
 * vanessa_logger_closelog
 * Exported function to close a logger
 * pre: vl: pointer to logger to close
 * post: logger is closed and memory is freed
 * return: none
 **********************************************************************/

void 
vanessa_logger_closelog(vanessa_logger_t * vl);


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

void 
vanessa_logger_change_max_priority(vanessa_logger_t * vl, 
		const int max_priority);


/**********************************************************************
 * vanessa_logger_log
 * Exported function to log a message
 * pre: vl: pointer to logger to log to
 *      priority: Priority to log with.
 *                If priority is more than max_priority as provided to
 *                vanessa_logger_openlog_*.
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
vanessa_logger_log(vanessa_logger_t * vl, int priority, const char *fmt, ...);


/**********************************************************************
 * vanessa_logger_logv
 * Exported function to log a message
 * Same as vanessa_logger_logv but a va_list is given instead
 * of a variable number of arguments.
 **********************************************************************/

void 
vanessa_logger_logv(vanessa_logger_t * vl, int priority, const char *fmt, 
		va_list ap);


/**********************************************************************
 * _vanessa_logger_log_prefix
 * Exported function used by convienience macros to prefix a message
 * with the function name that the message was generated in
 **********************************************************************/

void 
_vanessa_logger_log_prefix(vanessa_logger_t * vl, int priority, 
		const char *prefix, const char *fmt, ...);


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

int 
vanessa_logger_reopen(vanessa_logger_t * vl);


/**********************************************************************
 * vanessa_logger_strherror_r
 * Returns a string describing the error code present in errnum
 * according to the errors for h_errno which is set by gethostbyname(3)
 * gethostbyaddr(3) and others. Analagous to strerror_r(3).
 * pre: errnum: Error to show as a string
 *      buf: buffer to write error string to
 *      n: length of buf in bytes
 * post: on success error string is written to buf
 *       on invalud input errno is set to -EINVAL
 *       if buf is too short then errno is set to -ERANGE
 * return: 0 on success
 *         -1 on error
 **********************************************************************/

int
vanessa_logger_strherror_r(int errnum, char *buf, size_t n);


/**********************************************************************
 * vanessa_logger_strherror
 * Returns a string describing the error code present in errnum
 * according to the errors for h_errno which is set by gethostbyname(3)
 * gethostbyaddr(3) and others. Analagous to strerror_r(3).
 * pre: errnum: Error to show as a string
 * post: none on success
 *       on invalid input errno is set to -EINVAL
 *       if buf is too short then errno is set to -ERANGE
 * return: error string for errnum on success
 *         error string for newly set errno on error
 **********************************************************************/

char *
vanessa_logger_strherror(int errnum);


/**********************************************************************
 * vanessa_logger_set_flag
 * Set flags for logger
 * Should only be used on filehandle or filename loggers,
 * ignored otherewise.
 * pre: vl: logger to set flags of
 *      flag: value to set flags to
 *            See "Flags for filehandle or filename loggers"
 *            in vanessa_logger.h for valid flags
 * post: flag is set
 * return: none
 **********************************************************************/

void
vanessa_logger_set_flag(vanessa_logger_t * vl, vanessa_logger_flag_t flag);


/**********************************************************************
 * vanessa_logger_get_flag
 * Set flags for logger
 * Should only be used on filehandle or filename loggers,
 * will return 0 for other types of loggers.
 * See "Flags for filehandle or filename loggers"
 * in vanessa_logger.h for valid flags
 * pre: vl: logger to get flags of
 * post: none
 * return: flags for logger
 **********************************************************************/

vanessa_logger_flag_t
vanessa_logger_get_flag(vanessa_logger_t * vl);


/**********************************************************************
 * vanessa_logger_str_dump
 * Sanitise a buffer into ASCII
 * pre: vl: Vanessa logger to log errors to. May be NULL.
 *      buffer: buffer to sanitise
 *      size: number of bytes in buffer to sanitise
 *      flag: If VANESSA_LOGGER_STR_DUMP_HEX then a hexidecimal dump
 *            will be done. Else an octal dump will be done.
 * post: a new buffer is alocated. For each byte in buffer
 *       that is a printable ASCII character it is added to
 *       the new buffer. All other characters are represented
 *       in the new buffer as octal, in the form \xxx.
 * return: the new buffer, this should be freed by the caller
 *         NULL on error
 **********************************************************************/


#define VANESSA_LOGGER_STR_DUMP_OCT 0x0
#define VANESSA_LOGGER_STR_DUMP_HEX 0x1

char *
vanessa_logger_str_dump(vanessa_logger_t * vl, const char *buffer, 
		const size_t buffer_length, vanessa_logger_flag_t flag);


/**********************************************************************
 * The code below sets an internal logger and provides convenience
 * macros to use this logger. You may either use this, or keep
 * track of the logger yourself and use the vanessa_logger functions
 * above directly. The latter approach allows for more than
 * one logger to be in use at in a single programme.
 **********************************************************************/


extern vanessa_logger_t *__vanessa_logger_vl;

/**********************************************************************
 * vanessa_logger_vl_set
 * set the logger function to use with convenience macros
 * No logging will take place using conveineince macros if logger is 
 * set to NULL (default). That is you _must_ call this function to 
 * enable logging using convenience macros.
 * pre: logger: pointer to a vanessa_logger
 * post: logger for convenience marcros is set to logger
 * return: none
 **********************************************************************/

#define vanessa_logger_set(_vl) __vanessa_logger_vl=(_vl)


/**********************************************************************
 * vanessa_logger_vl_set_stderr
 * set the logger function to use with convenience macros
 * No logging will take place using conveineince macros if logger is set to
 * NULL (default). That is you _must_ call one of the vanessa_logger_vl_set
 * functions to enable logging using convenience macros.
 * pre: logger: pointer to a vanessa_logger
 * post: logger for convenience marcros is set to logger
 * return: none
 **********************************************************************/

#define vanessa_logger_set_stderr(_vl) __vanessa_logger_vl=(_vl)


/**********************************************************************
 * vanessa_logger_vl_set_filehandle
 * set the logger function to a logger created for the filehandle 
 * No logging will take place using conveineince macros if logger is set to
 * NULL (default). That is you _must_ call one of the vanessa_logger_vl_set
 * functions to enable logging using convenience macros.
 * pre:  fd: filehandle to log to 
 * post: logger for filehandle is created and set as the 
 *       logger for convenience marcros 
 * created for the filehandle return: none
 **********************************************************************/

vanessa_logger_t *
vanessa_logger_set_filehandle(FILE *fh);


/**********************************************************************
 * vanessa_logger_vl_unset
 * set logger to use with convenience macros to NULL
 * That is no logging will take place when convenience macros are called
 * pre: none
 * post: logger is NULL
 * return: none
 **********************************************************************/

#define vanessa_logger_unset() vanessa_logger_set(NULL)


/**********************************************************************
 * vanessa_logger_vl_get
 * retreive the logger function used by convenience macros
 * pre: none
 * post: none
 * return: logger used by convenience macros
 **********************************************************************/

#define vanessa_logger_get() (__vanessa_logger_vl)


/**********************************************************************
 * VANESSA_LOGGER_DEBUG et al
 * Convenience macros for using internal logger set using
 * vanessa_logger_set()
 **********************************************************************/

/*
 * Hooray for format string problems!
 *
 * Each of the logging macros has two versions. The UNSAFE version will
 * accept a format string. You should _NOT_ use the UNSAFE versions if the
 * first argument, the format string, is derived from user input. The safe
 * versions (versions that do not have the "_UNSAFE" suffix) do not accept
 * a format string and only accept one argument, the string to log. These
 * should be safe to use with user derived input.
 */

#define VANESSA_LOGGER_LOG_UNSAFE(priority, fmt, ...) \
	vanessa_logger_log(__vanessa_logger_vl, priority, fmt, __VA_ARGS__);

#define VANESSA_LOGGER_LOG(priority, str) \
	vanessa_logger_log(__vanessa_logger_vl, priority, "%s", str)

#define VANESSA_LOGGER_DEBUG_UNSAFE(fmt, ...) \
	_vanessa_logger_log_prefix(__vanessa_logger_vl, LOG_DEBUG, \
		__FUNCTION__, fmt, __VA_ARGS__);

#define VANESSA_LOGGER_DEBUG(str) \
	_vanessa_logger_log_prefix(__vanessa_logger_vl, LOG_DEBUG, \
		__FUNCTION__, "%s", str);

#define VANESSA_LOGGER_DEBUG_ERRNO(str) \
	_vanessa_logger_log_prefix(__vanessa_logger_vl, LOG_DEBUG, \
		__FUNCTION__, "%s: %s", str, strerror(errno));

#define VANESSA_LOGGER_DEBUG_HERRNO(str) \
	_vanessa_logger_log_prefix(__vanessa_logger_vl, LOG_DEBUG, \
		__FUNCTION__, "%s: %s", str, \
		vanessa_logger_strherror(h_errno));

#define VANESSA_LOGGER_DEBUG_RAW_UNSAFE(fmt, ...) \
	vanessa_logger_log(__vanessa_logger_vl, LOG_DEBUG, \
		fmt, __VA_ARGS__);

#define VANESSA_LOGGER_DEBUG_RAW(str) \
	vanessa_logger_log(__vanessa_logger_vl, LOG_DEBUG, "%s", str);

#define VANESSA_LOGGER_INFO_UNSAFE(fmt, ...) \
	vanessa_logger_log(__vanessa_logger_vl, LOG_INFO, fmt, __VA_ARGS__);

#define VANESSA_LOGGER_INFO(str) \
	vanessa_logger_log(__vanessa_logger_vl, LOG_INFO, "%s", str);

#define VANESSA_LOGGER_ERR_UNSAFE(fmt, ...) \
	vanessa_logger_log(__vanessa_logger_vl, LOG_ERR, fmt, __VA_ARGS__);

#define VANESSA_LOGGER_ERR_RAW_UNSAFE(fmt, ...) \
	vanessa_logger_log(__vanessa_logger_vl, LOG_ERR, fmt, __VA_ARGS__);

#define VANESSA_LOGGER_ERR(str) \
	vanessa_logger_log(__vanessa_logger_vl, LOG_ERR, "%s", str);

#define VANESSA_LOGGER_RAW_ERR(str) \
	vanessa_logger_log(__vanessa_logger_vl, LOG_ERR, "%s", str);

#define VANESSA_LOGGER_DUMP(buffer, buffer_length, flag) \
	vanessa_logger_str_dump(__vanessa_logger_vl, (buffer), \
			(buffer_length), (flag))

#endif
