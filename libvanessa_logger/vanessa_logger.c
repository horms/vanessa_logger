/**********************************************************************
 * vanessa_logger.c                                      September 2000
 * Simon Horman                                      horms@verge.net.au
 *
 * vanessa_logger
 * Generic logging layer
 * Copyright (C) 2000-2008  Simon Horman <horms@verge.net.au>
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


#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>

#define SYSLOG_NAMES
#include <syslog.h>

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif


/**********************************************************************
 * Sun Solaris, AIX and probably others don't to define facilitynames 
 * so the following compatibility code is provided.
 **********************************************************************/

#ifndef WITH_FACILITYNAMES

#define LOG_AUTHPRIV    (10<<3)	/* security/authorization messages (private) */
#define LOG_FTP         (11<<3)	/* ftp daemon */

#define LOG_MAKEPRI(fac, pri)   (((fac) << 3) | (pri))

#define INTERNAL_NOPRI  0x10	/* the "no priority" priority */
				/* mark "facility" */
#define INTERNAL_MARK   LOG_MAKEPRI(LOG_NFACILITIES, 0)
typedef struct _code {
	char *c_name;
	int c_val;
} CODE;

CODE facilitynames[] = {
	{"auth", LOG_AUTH},
	{"authpriv", LOG_AUTHPRIV},
	{"cron", LOG_CRON},
	{"daemon", LOG_DAEMON},
	{"ftp", LOG_FTP},
	{"kern", LOG_KERN},
	{"lpr", LOG_LPR},
	{"mail", LOG_MAIL},
	{"mark", INTERNAL_MARK},	/* INTERNAL */
	{"news", LOG_NEWS},
	{"security", LOG_AUTH},	/* DEPRECATED */
	{"syslog", LOG_SYSLOG},
	{"user", LOG_USER},
	{"uucp", LOG_UUCP},
	{"local0", LOG_LOCAL0},
	{"local1", LOG_LOCAL1},
	{"local2", LOG_LOCAL2},
	{"local3", LOG_LOCAL3},
	{"local4", LOG_LOCAL4},
	{"local5", LOG_LOCAL5},
	{"local6", LOG_LOCAL6},
	{"local7", LOG_LOCAL7},
	{NULL, -1}
};

#endif /* WITH_FACILITYNAMES */


#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "vanessa_logger.h"

extern int errno;
vanessa_logger_t *__vanessa_logger_vl;

/**********************************************************************
 * Internal data structures
 **********************************************************************/

typedef struct {
	FILE *filehandle;
	char *filename;
} __vanessa_logger_filename_data_t;

typedef union {
	void *d_any;
	FILE *d_filehandle;
	__vanessa_logger_filename_data_t *d_filename;
	int *d_syslog;
	vanessa_logger_log_function_va_t d_function;
} __vanessa_logger_data_t;

typedef enum {
	__vanessa_logger_filehandle,
	__vanessa_logger_filename,
	__vanessa_logger_syslog,
	__vanessa_logger_function,
	__vanessa_logger_none
} __vanessa_logger_type_t;

typedef enum {
	__vanessa_logger_true,
	__vanessa_logger_false
} __vanessa_logger_bool_t;

typedef struct {
	__vanessa_logger_type_t type;
	__vanessa_logger_data_t data;
	__vanessa_logger_bool_t ready;
	char *ident;
	char *buffer;
	size_t buffer_len;
	int max_priority;
	unsigned int flag;
} __vanessa_logger_t;


/**********************************************************************
 * Internal Defines
 **********************************************************************/

#define __VANESSA_LOGGER_BUF_SIZE (size_t)1024

#define __VANESSA_LOGGER_SAFE_FREE(ptr) \
  if(ptr!=NULL){ \
    free(ptr); \
    ptr=NULL; \
  }


/**********************************************************************
 * Prototype of internal functions
 **********************************************************************/

static __vanessa_logger_t *
__vanessa_logger_create(void);

static void 
__vanessa_logger_destroy(__vanessa_logger_t * vl);

static void 
__vanessa_logger_reset(__vanessa_logger_t * vl);

static __vanessa_logger_t *
__vanessa_logger_set(__vanessa_logger_t * vl, const char *ident,
		const int max_priority, const __vanessa_logger_type_t type, 
		void *data, const int option);

static void 
__vanessa_logger_log(__vanessa_logger_t * vl, int priority, 
		const char *prefix, const char *fmt, va_list ap);

static int 
__vanessa_logger_reopen(__vanessa_logger_t * vl);


/**********************************************************************
 * __vanessa_logger_create
 * Internal function to create a new logger
 * pre: none
 * post: Memory for logger is allocated and values are initialised
 *       to null state
 *       NULL on error
 * return: pointer to new logger
 **********************************************************************/

static __vanessa_logger_t *
__vanessa_logger_create(void)
{
	__vanessa_logger_t *vl;

	vl = (__vanessa_logger_t *) malloc(sizeof(__vanessa_logger_t));
	if (!vl) {
		perror("__vanessa_logger_create: malloc");
		return (NULL);
	}

	vl->type = __vanessa_logger_none;
	vl->data.d_any = NULL;
	vl->ready = __vanessa_logger_false;
	vl->ident = NULL;
	vl->buffer = NULL;
	vl->buffer_len = 0;
	vl->max_priority = 0;

	return (vl);
}


/**********************************************************************
 * __vanessa_logger_destroy
 * Internal function to destroy a logger
 * pre: vl: pointer to logger to destroy
 * post: Memory for logger and any internal memory is freed
 *       Nothing if vl is NULL
 * return: none
 **********************************************************************/

static void 
__vanessa_logger_destroy (__vanessa_logger_t * vl)
{
	if (!vl) {
		return;
	}

	__vanessa_logger_reset(vl);
	__VANESSA_LOGGER_SAFE_FREE(vl);
}


/**********************************************************************
 * __vanessa_logger_reset
 * Internal function to set all the values of a logger to their null state
 * and free any allocated memory.
 * pre: vl: pointer to logger to reset
 * post: All internal memory is freed
 *       All internal values are set to null state
 *       Nothing if vl is NULL
 * return: none
 **********************************************************************/

static void __vanessa_logger_reset(__vanessa_logger_t * vl)
{
	__vanessa_logger_bool_t ready;

	if (!vl) {
		return;
	}

	/* 
	 * Logger is no longer ready
	 */
	ready = vl->ready;	/* Remember state logger _was_ in */
	vl->ready = __vanessa_logger_false;

	/*
	 * Close filehandles or log facilities as necessary
	 * Free any memory used in storing data
	 */
	switch (vl->type) {
	case __vanessa_logger_filename:
		if (ready == __vanessa_logger_true) {
			if (fclose(vl->data.d_filename->filehandle)) {
				perror("__vanessa_logger_reset: fclose");
			}
		}
		if (vl->data.d_filename != NULL) {
			__VANESSA_LOGGER_SAFE_FREE(vl->data.d_filename->
						   filename);
		}
		__VANESSA_LOGGER_SAFE_FREE(vl->data.d_filename);
		break;
	case __vanessa_logger_syslog:
		__VANESSA_LOGGER_SAFE_FREE(vl->data.d_syslog);
		if (vl->ready == __vanessa_logger_true) {
			closelog();
		}
		break;
	default:
		break;
	}

	/*
	 * Reset type and data
	 */
	vl->type = __vanessa_logger_none;
	vl->data.d_any = NULL;

	/*
	 * Reset ident
	 */
	__VANESSA_LOGGER_SAFE_FREE(vl->ident);

	/*
	 * Reset buffer, buffer_len
	 */
	__VANESSA_LOGGER_SAFE_FREE(vl->buffer);
	vl->buffer_len = 0;

	/*
	 * Reset max_priority
	 */
	vl->max_priority = 0;
}


/**********************************************************************
 * __vanessa_logger_set
 * Internal function to seed the values of a logger
 * pre: vl: pointer to logger to seed values of
 *      ident: Identity string to prepend to each log
 *      type: Type of logger to initialise
 *      data: Type specific data for logger typecast to (void *)
 * post: Values of logger are set to allow logging as per type
 *       Nothing if: vl is NULL or
 *                   type is __vanessa_logger_none or
 *                   data is NULL or
 *                   ident is NULL
 *       On error vl is destroyed as per __vanessa_logger_destroy
 * return: vl on success
 *         NULL on error
 **********************************************************************/

static __vanessa_logger_t *
__vanessa_logger_set(__vanessa_logger_t * vl, const char *ident,
		const int max_priority, const __vanessa_logger_type_t type, 
		void *data, const int option)
{
	if (!vl || type == __vanessa_logger_none || !data || !ident) {
		return (NULL);
	}

	/*
	 * Free any previously allocated memory
	 */
	__vanessa_logger_reset(vl);

	/*
	 * Set ident
	 */
	vl->ident = strdup(ident);
	if (!vl->ident) {
		perror("__vanessa_logger_set: strdup 1");
		__vanessa_logger_destroy(vl);
		return (NULL);
	}

	/*
	 * Set buffer
	 */
	vl->buffer = (char *) malloc(__VANESSA_LOGGER_BUF_SIZE);
	if (!vl->buffer) {
		perror("__vanessa_logger_set: malloc 1");
		__vanessa_logger_destroy(vl);
		return (NULL);
	}
	vl->buffer_len = __VANESSA_LOGGER_BUF_SIZE;

	/*
	 * Set type
	 */
	vl->type = type;

	/*
	 * Set data
	 */
	switch (vl->type) {
	case __vanessa_logger_filehandle:
		vl->flag = option;
		vl->data.d_filehandle = (FILE *) data;
		break;
	case __vanessa_logger_filename:
		vl->flag = option;
		if ((vl->data.d_filename =
		     (__vanessa_logger_filename_data_t *)
		     malloc(sizeof(__vanessa_logger_filename_data_t)
		     )) == NULL) {
			perror("__vanessa_logger_set: malloc 2");
			__vanessa_logger_destroy(vl);
			return (NULL);
		}
		if ((vl->data.d_filename->filename =
		     strdup((char *) data)) == NULL) {
			perror("__vanessa_logger_set: malloc strdup 2");
			__vanessa_logger_destroy(vl);
			return (NULL);
		}
		vl->data.d_filename->filehandle =
		    fopen(vl->data.d_filename->filename, "a");
		if (vl->data.d_filename->filehandle == NULL) {
			perror("__vanessa_logger_set: fopen");
			__vanessa_logger_destroy(vl);
			return (NULL);
		}
		break;
	case __vanessa_logger_syslog:
		vl->flag = VANESSA_LOGGER_F_NO_IDENT_PID;
		if ((vl->data.d_syslog =
		     (int *) malloc(sizeof(int))) == NULL) {
			perror("__vanessa_logger_set: malloc 3");
			__vanessa_logger_destroy(vl);
			return (NULL);
		}
		*(vl->data.d_syslog) = *((int *) data);
		openlog(vl->ident, LOG_PID | option, *(vl->data.d_syslog));
		break;
	case __vanessa_logger_function:
		vl->data.d_function = (vanessa_logger_log_function_va_t) data;
		break;
	case __vanessa_logger_none:
		break;
	}

	/*
	 * Set max_priority
	 */
	vl->max_priority = max_priority;

	/*
	 * Set ready
	 */
	vl->ready = __vanessa_logger_true;

	return (vl);
}


/**********************************************************************
 * __vanessa_logger_reopen
 * Internal function to reopen a logger
 * pre: vl: pointer to logger to reopen
 * post: In the case of a filename logger the logger is closed
 *       if it was open and then opened regardless of weather it
 *       was originally open or not.
 *       In the case of a none, syslog or filehandle logger or if vl is NULL
 *       nothing is done.
 *       If an error occurs -1 is returned and vl->ready is set to
 *       __vanessa_logger_false
 * return: 0 on success
 *         -1 on error
 **********************************************************************/

static int 
__vanessa_logger_reopen(__vanessa_logger_t * vl)
{
	if (!vl || vl->type == __vanessa_logger_none) {
		return (0);
	}

	switch (vl->type) {
	case __vanessa_logger_filename:
		if (vl->ready == __vanessa_logger_true) {
			vl->ready = __vanessa_logger_false;
			if (fclose(vl->data.d_filename->filehandle)) {
				perror("__vanessa_logger_reopen: fclose");
				return (-1);
			}
		}
		vl->data.d_filename->filehandle =
		    fopen(vl->data.d_filename->filename, "a");
		if (vl->data.d_filename->filehandle == NULL) {
			perror("__vanessa_logger_reopen: fopen");
			return (-1);
		}
		vl->ready = __vanessa_logger_true;
		break;
	default:
		break;
	}

	return (0);
}


/**********************************************************************
 * __vanessa_logger_va_func_wrapper
 * Internal wrapper function be able to use a 
 * vanessa_logger_log_function_t, which uses va_list, with a ...
 * pre: func: function to call
 *      priority: priority pass to func
 *      fmt: to pass to func
 *      ...: args to convert to an ap and pass to func
 * post: func is called 
 * return: none
 **********************************************************************/

static void 
__vanessa_logger_va_func_wrapper(vanessa_logger_log_function_va_t func,
		int priority, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	func(priority, fmt, ap);
	va_end(ap);
}



/**********************************************************************
 * __vanessa_logger_log
 * Internal function to log a message
 * pre: vl: logger to use
 *      priority: priority to log with
 *                Only used if log type is __vanessa_logger_syslog
 *                Ignored otherwise
 *      fmt: format for log message
 *      ap: varargs for format
 * post: message is logged to appropriate logger
 *       vl->ident[pid]: will be prepended to each log
 *       '\n' will be appended to each log that doesn't already end with
 *       a '\n'
 *       Nothing on error
 * return: none
 **********************************************************************/

int __vanessa_logger_do_fmt(__vanessa_logger_t *vl, 
		const char *prefix, const char *fmt)
{
	int len;
	size_t offset = 0;
	int add_colon = 0;

	memset(vl->buffer, 0, vl->buffer_len);

	if(vl->flag & VANESSA_LOGGER_F_TIMESTAMP) {
		struct tm *tm;
		time_t now;

		now = time(NULL);
		if (now == (time_t)-1) {
			return -1;
		}
		tm = localtime(&now);
		if (!tm) {
			return -1;
		}
		len = strftime(vl->buffer + offset, 
				vl->buffer_len - offset - 1, "%b %e %H:%M:%S ",
				tm);
		if (len < 0) {
			return -1;
		}
		offset += len;
		add_colon++;
	}

	if(vl->ident && !(vl->flag & VANESSA_LOGGER_F_NO_IDENT_PID)) {
		len = snprintf(vl->buffer + offset , 
				vl->buffer_len - offset - 1, "%s[%d] ",
				vl->ident, getpid());
		if (len < 0) {
			return -1;
		}
		offset += len;
		add_colon++;
	}

	if (add_colon) {
		len = snprintf(vl->buffer + offset - 1, 
				 vl->buffer_len - offset, ": ");
		if (len < 0) {
			return -1;
		}
		offset++;
	}

	if(prefix) {
		len = strlen(prefix) + 2;
		if (offset + len + 1 > vl->buffer_len) {
			return -1;
		}
		memcpy(vl->buffer + offset, prefix, len - 2);
		memcpy(vl->buffer + offset + len - 2, ": ", 2);
		offset += len;
	}

	len = strlen(fmt);
	if (offset + len + 1 > vl->buffer_len) {
		return -1;
	}
	memcpy(vl->buffer + offset, fmt, len);
	offset += len;

	if (*(vl->buffer+offset-1) != '\n') {
		if(offset + 2 > vl->buffer_len) {
			return -1;
		}
		*(vl->buffer+offset)='\n';
		*(vl->buffer+offset+1)='\0';
	}

	return(0);
}

void __vanessa_logger_do_fh(__vanessa_logger_t * vl, const char *prefix, 
		const char *fmt, FILE *fh, va_list ap) 
{
	if (__vanessa_logger_do_fmt(vl, prefix, fmt) < 0) {
		fprintf(fh, "__vanessa_logger_do_fh: output truncated\n");
		return;
	}

	if (((vfprintf(fh, vl->buffer, ap) < 0  || fflush(fh) == EOF) && 
			vl->flag & VANESSA_LOGGER_F_CONS) ||
			vl->flag & VANESSA_LOGGER_F_PERROR){
		vfprintf(stderr, vl->buffer, ap);
		fflush(stderr);
	}
}

void __vanessa_logger_do_func(__vanessa_logger_t * vl, int priority, 
		const char *prefix, const char *fmt, va_list ap, 
		vanessa_logger_log_function_va_t func)
{
	if (__vanessa_logger_do_fmt(vl, prefix, fmt) < 0) {
		__vanessa_logger_va_func_wrapper(func, priority, 
				"__vanessa_logger_do_fh: output truncated\n");
		return;
	}

	(func)(priority, vl->buffer, ap);
}


static void 
__vanessa_logger_log(__vanessa_logger_t * vl, int priority, 
		const char *prefix, const char *fmt, va_list ap)
{
	if (vl == NULL || vl->ready == __vanessa_logger_false
	    || priority > vl->max_priority) {
		return;
	}

	switch (vl->type) {
		case __vanessa_logger_filehandle:
			__vanessa_logger_do_fh(vl, prefix, fmt, 
					vl->data.d_filehandle, ap);
			break;
		case __vanessa_logger_filename:
			__vanessa_logger_do_fh(vl, prefix, fmt,
			       	vl->data.d_filename->filehandle, ap);
			break;
		case __vanessa_logger_syslog:
			__vanessa_logger_do_func(vl, priority, prefix, fmt, ap, 
					vsyslog);
			break;
		case __vanessa_logger_function:
			__vanessa_logger_do_func(vl, priority, prefix, fmt, ap,
					vl->data.d_function);
			break;
		case __vanessa_logger_none:
			break;
	}
}


/**********************************************************************
 * __vanessa_logger_get_facility_byname
 * Given the name of a syslog facility as an ASCII string,
 * return the facility as an integer.
 * Relies on facilitynames[] being defined in syslog.h
 * pre: facility_name: syslog facility as an ASCII string
 * post: none
 * return: logger as an int
 *         -1 if facility_name cannot be found in facilitynames[],
 *            if facility_name is NULL
 *            or other error
 **********************************************************************/

static int 
__vanessa_logger_get_facility_byname(const char *facility_name)
{
	int i;

	extern CODE facilitynames[];

	if (facility_name == NULL) {
		fprintf(stderr,
			"__vanessa_logger_get_facility_byname: "
			"facility_name is NULL\n");
		return (-1);
	}

	for (i = 0; facilitynames[i].c_name != NULL; i++) {
		if (!strcmp(facility_name, facilitynames[i].c_name)) {
			return (facilitynames[i].c_val);
		}
	}

	fprintf(stderr,
		"__vanessa_logger_get_facility_byname: facility \"%s\" "
		"not found\n", facility_name);
	return (-1);
}

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
vanessa_logger_openlog_syslog(int facility, const char *ident,
		const int max_priority, const int option)
{
	__vanessa_logger_t *vl;

	if ((vl = __vanessa_logger_create()) == NULL) {
		fprintf(stderr, "vanessa_logger_openlog_syslog: "
			"__vanessa_logger_create\n");
		return (NULL);
	}

	if (__vanessa_logger_set(vl, ident, max_priority,
			 __vanessa_logger_syslog, (void *) &facility, 
			option) == NULL) {
		fprintf(stderr, "vanessa_logger_openlog_syslog: "
			"__vanessa_logger_set\n");
		return (NULL);
	}

	return ((vanessa_logger_t *) vl);
}


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
		const char *ident, const int max_priority, const int option)
{
	__vanessa_logger_t *vl;
	int facility;

	facility = __vanessa_logger_get_facility_byname(facility_name);
	if (facility < 0) {
		fprintf(stderr, "vanessa_logger_open_syslog_byname: "
			"__vanessa_logger_get_facility_byname\n");
		return (NULL);
	}

	vl = vanessa_logger_openlog_syslog(facility, ident, max_priority,
					   option);
	if (!vl) {
		fprintf(stderr,
			"vanessa_logger_openlog_syslog: "
			"vanessa_logger_openlog_syslog\n");
		return (NULL);
	}

	return ((vanessa_logger_t *) vl);
}


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
 *           in vanessa_logger.h for valid flags
 * post: Logger is opened
 * return: pointer to logger
 *         NULL on error
 **********************************************************************/

vanessa_logger_t *
vanessa_logger_openlog_filehandle(FILE * filehandle, const char *ident,
		const int max_priority, const int flag)
{
	__vanessa_logger_t *vl;

	if ((vl = __vanessa_logger_create()) == NULL) {
		fprintf(stderr,
			"vanessa_logger_openlog_filehandle: "
			"__vanessa_logger_create\n");
		return (NULL);
	}

	if (__vanessa_logger_set(vl, ident, max_priority, 
			__vanessa_logger_filehandle, (void *) filehandle, 
			flag) == NULL) {
		fprintf(stderr, "vanessa_logger_openlog_filehandle: "
				"__vanessa_logger_set\n");
		return (NULL);
	}

	return ((vanessa_logger_t *) vl);
}


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
 *           in vanessa_logger.h for valid flags
 * post: Logger is opened
 * return: pointer to logger
 *         NULL on error
 **********************************************************************/

vanessa_logger_t *
vanessa_logger_openlog_filename(const char *filename, const char *ident,
		const int max_priority, const int flag)
{
	__vanessa_logger_t *vl;

	vl = __vanessa_logger_create();
	if (!vl) {
		fprintf(stderr, "vanessa_logger_openlog_filename: "
			"__vanessa_logger_create\n");
		return (NULL);
	}

	if (__vanessa_logger_set(vl, ident, max_priority,
			 __vanessa_logger_filename, (void *) filename, 
			flag) == NULL) {
		fprintf(stderr, "vanessa_logger_openlog_filename: "
			"__vanessa_logger_set\n");
		return (NULL);
	}

	return ((vanessa_logger_t *) vl);
}


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
		const char *ident, const int max_priority, const int option)
{
	__vanessa_logger_t *vl;

	if ((vl = __vanessa_logger_create()) == NULL) {
		fprintf(stderr, "vanessa_logger_openlog_function: "
			"__vanessa_logger_create\n");
		return (NULL);
	}

	if (__vanessa_logger_set(vl, ident, max_priority,
			 __vanessa_logger_function, (void *) log_function, 
			option) == NULL) {
		fprintf(stderr, "vanessa_logger_openlog_function: "
			"__vanessa_logger_set\n");
		return (NULL);
	}

	return ((vanessa_logger_t *) vl);
}


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
		const int max_priority)
{
	if (vl == NULL) {
		return;
	}

	((__vanessa_logger_t *) vl)->max_priority = max_priority;
}


/**********************************************************************
 * vanessa_logger_closelog
 * Exported function to close a logger
 * pre: vl: pointer to logger to close
 * post: logger is closed and memory is freed
 *       If global logger set by vanessa_logger_set() is being closed
 *       then it is unset using vanessa_logger_unset()
 * return: none
 **********************************************************************/

void 
vanessa_logger_closelog(vanessa_logger_t * vl)
{
	if(vanessa_logger_get() == vl) {
		vanessa_logger_unset();
	}
	__vanessa_logger_destroy((__vanessa_logger_t *) vl);
}


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
vanessa_logger_log(vanessa_logger_t * vl, int priority, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	__vanessa_logger_log((__vanessa_logger_t *) vl, priority, NULL, 
			fmt, ap);
	va_end(ap);
}


/**********************************************************************
 * vanessa_logger_logv
 * Exported function to log a message
 * Same as vanessa_logger_log but a va_list is given instead
 * of a variable number of arguments.
 **********************************************************************/

void 
vanessa_logger_logv(vanessa_logger_t * vl, int priority, const char *fmt, 
		va_list ap)
{
	__vanessa_logger_log((__vanessa_logger_t *) vl, priority, NULL,
			fmt, ap);
}


/**********************************************************************
 * _vanessa_logger_log_prefix
 * Exported function used by convenience macros to prefix a message
 * with the function name that the message was generated in
 **********************************************************************/

void 
_vanessa_logger_log_prefix(vanessa_logger_t * vl, int priority, 
		const char *prefix, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	__vanessa_logger_log((__vanessa_logger_t *) vl, priority, prefix,
			fmt, ap);
	va_end(ap);
}

/**********************************************************************
 * vanessa_logger_set_flag
 * Set flags for logger
 * Should only be used on filehandle or filename loggers,
 * ignored otherwise.
 * pre: vl: logger to set flags of
 *      flag: value to set flags to
 *            See "Flags for filehandle or filename loggers"
 *            in vanessa_logger.h for valid flags
 * post: flag is set
 * return: none
 **********************************************************************/

void
vanessa_logger_set_flag(vanessa_logger_t * vl, vanessa_logger_flag_t flag)
{
	switch (((__vanessa_logger_t *)vl)->type) {
		case __vanessa_logger_filehandle:
		case __vanessa_logger_filename:
			((__vanessa_logger_t *)vl)->flag = flag;
			break;
		case __vanessa_logger_syslog:
		case __vanessa_logger_function:
		case __vanessa_logger_none:
			break;
	}
}


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
vanessa_logger_get_flag(vanessa_logger_t * vl)
{
	switch (((__vanessa_logger_t *)vl)->type) {
		case __vanessa_logger_filehandle:
		case __vanessa_logger_filename:
			return ((__vanessa_logger_t *)vl)->flag;
		case __vanessa_logger_syslog:
		case __vanessa_logger_function:
		case __vanessa_logger_none:
			return 0;
	}

	/* Not reached */
	return 0;
}


static char vanessa_logger_strherror_str[34];

/**********************************************************************
 * vanessa_logger_strherror_r
 * Returns a string describing the error code present in errnum
 * according to the errors for h_errno which is set by gethostbyname(3)
 * gethostbyaddr(3) and others. Analogous to strerror_r(3).
 * pre: errnum: Error to show as a string
 *      buf: buffer to write error string to
 *      n: length of buf in bytes
 * post: on success error string is written to buf
 *       on invalid input errno is set to -EINVAL
 *       if buf is too short then errno is set to -ERANGE
 * return: 0 on success
 *         -1 on error
 **********************************************************************/

int
vanessa_logger_strherror_r(int errnum, char *buf, size_t n)
{
	size_t len;
	char *str;

	switch(errnum) {
		case HOST_NOT_FOUND:
			str = "Unknown host";
			break;
		case NO_ADDRESS:
		/* case NO_DATA: */
			str = "Host has no address";
			break;
		case NO_RECOVERY:
			str = "Non-recoverable name server error";
			break;
		case TRY_AGAIN:
			str = "Transient lookup error";
			break;
		default:
			errno = -EINVAL;
			return(-1);
	}

	len = strlen(str) + 1;
	if(len > n) {
		errno = -ERANGE;
		return(-1);
	}

	memcpy(buf, str, len);
	return(0);
}


/**********************************************************************
 * vanessa_logger_strherror
 * Returns a string describing the error code present in errnum
 * according to the errors for h_errno which is set by gethostbyname(3)
 * gethostbyaddr(3) and others. Analogous to strerror_r(3).
 * pre: errnum: Error to show as a string
 * post: none on success
 *       on invalid input errno is set to -EINVAL
 *       if buf is too short then errno is set to -ERANGE
 * return: error string for errnum on success
 *         error string for newly set errno on error
 **********************************************************************/

char *
vanessa_logger_strherror(int errnum)
{
	int status;

	status = vanessa_logger_strherror_r(errnum, 
			vanessa_logger_strherror_str,
			sizeof(vanessa_logger_strherror_str));
	if(status < 0) {
		return(strerror(errno));
	}

	return(vanessa_logger_strherror_str);
}


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

int vanessa_logger_reopen(vanessa_logger_t * vl)
{
	if (__vanessa_logger_reopen((__vanessa_logger_t *) vl)) {
		fprintf(stderr,
			"vanessa_logger_reopen: __vanessa_logger_reopen\n");
		return (-1);
	}
	return (0);
}


/**********************************************************************
 * vanessa_logger_str_dump
 * Sanitise a buffer into ASCII
 * pre: vl: Vanessa logger to log errors to. May be NULL.
 *      buffer: buffer to sanitise
 *      size: number of bytes in buffer to sanitise
 *      flag: If VANESSA_LOGGER_STR_DUMP_HEX then a hexadecimal dump
 *            will be done. Else an octal dump will be done.
 * post: a new buffer is allocated. For each byte in buffer
 *       that is a printable ASCII character it is added to
 *       the new buffer. All other characters are represented
 *       in the new buffer as octal, in the form \xxx.
 * return: the new buffer, this should be freed by the caller
 *         NULL on error
 **********************************************************************/

static char *
__vanessa_logger_str_dump_oct(vanessa_logger_t * vl, 
		const char *buffer, const size_t buffer_length)
{
	const char *in_pos;
	const char *in_top;
	char *out_pos;
	char *out;

	out = (char *) malloc(buffer_length * 4 + 1);
	if (!out) {
		vanessa_logger_log(vl, LOG_DEBUG, 
				"vanessa_logger_str_dump: malloc: %s",
				strerror(errno));
		return (NULL);
	}

	out_pos = out;
	in_top = buffer + buffer_length;
	for (in_pos = buffer; in_pos < in_top; in_pos++) {
		switch(*in_pos) {
			case '\a':
				*out_pos++ = '\\';
				*out_pos++ = 'a';
				continue;
			case '\b':
				*out_pos++ = '\\';
				*out_pos++ = 'b';
				continue;
			case '\t':
				*out_pos++ = '\\';
				*out_pos++ = 't';
				continue;
			case '\n':
				*out_pos++ = '\\';
				*out_pos++ = 'n';
				continue;
			case '\v':
				*out_pos++ = '\\';
				*out_pos++ = 'v';
				continue;
			case '\f':
				*out_pos++ = '\\';
				*out_pos++ = 'f';
				continue;
			case '\r':
				*out_pos++ = '\\';
				*out_pos++ = 'r';
				continue;
			case '\\': 
			case '"': 
			case '\'':
				*out_pos++ = '\\';
				*out_pos++ = *in_pos;
				continue;
		}
		if (isgraph(*in_pos) || *in_pos == ' ') {
			*out_pos++ = *in_pos;
		} 
		else {
			snprintf(out_pos, 5, "\\%03o", *in_pos & 0xff);
			out_pos += 4;
		}
	}

	*out_pos++ = '\0';

	return (out);
}


static char *
__vanessa_logger_str_dump_hex(vanessa_logger_t * vl, const char *buffer,
			      const size_t buffer_length)
{
	const char *in_pos;
	const char *in_top;
	char *out_pos;
	char *out;
	int i;

	out = (char *) malloc((buffer_length << 1) + (buffer_length >> 2) + 1);
	if (!out) {
		vanessa_logger_log(vl, LOG_DEBUG, 
				"vanessa_logger_str_dump: malloc: %s",
				strerror(errno));
		return (NULL);
	}

	i = 0;
	out_pos = out;
	in_top = buffer + buffer_length;
	for (in_pos = buffer; in_pos < in_top; in_pos++) {
		sprintf(out_pos, "%02x", *in_pos);
		out_pos += 2;
		if((i++ & 0x3) == 3 && in_pos + 1 != in_top) {
			*out_pos++ = ' ';
		}
	}

	*out_pos++ = '\0';

	return (out);
}


char *
vanessa_logger_str_dump(vanessa_logger_t * vl, const char *buffer, 
		const size_t buffer_length, vanessa_logger_flag_t flag)
{
	if(flag == VANESSA_LOGGER_STR_DUMP_HEX) {
		return(__vanessa_logger_str_dump_hex(vl, buffer,
						     buffer_length));
	}

	return(__vanessa_logger_str_dump_oct(vl, buffer, buffer_length));
}			 
