/**********************************************************************
 * vanessa_logger_sample.c                               September 2000
 * Horms                                             horms@vergenet.net
 *
 * vanessa_logger
 * Generic logging layer
 * Copyright (C) 2000  Horms
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

#include <vanessa_logger.h>
#include <unistd.h>
#include <sys/types.h>

#include "vanessa_logger_sample_config.h"

#define MIN_UID 100

/**********************************************************************
 * Muriel the main function
 **********************************************************************/

int main (int argc, char **argv){
  vanessa_logger_t *log_fh=NULL;
  vanessa_logger_t *log_fn=NULL;
  vanessa_logger_t *log_sl=NULL;
  vanessa_logger_t *log_sl_bn=NULL;

  printf("vanessa_logger_sample version %s Copyright Horms\n", VERSION);


  /* 
   * Make sure this is _not_ being run by a privileged user
   *
   * This programme is not suitable to be run by privileged users
   * as the filename logger that is opened opens a file
   * in the PWD. If vanessa logger is used in a programme
   * designed to be used by a privileged user then a full
   * pathname should be given to vanessa_logger_openlog_filename()
   */
  if(getuid()<MIN_UID || geteuid()<MIN_UID){
    fflush(stdout);
    fprintf(
      stderr,
      "Error: Run by privileged user with UID<%d or EUID<%d. Exiting\n",
      MIN_UID,
      MIN_UID
    );
    exit(-1);
  }

  /* 
   * Open logger to filehandle stderr
   */
  printf("\nOpening loggers\n");

  log_fh=vanessa_logger_openlog_filehandle(
    stderr, 
    "vanessa_logger_sample", 
    LOG_DEBUG,
    0
  );
  if(log_fh==NULL){
    fprintf(stderr, "Error: vanessa_logger_openlog_filehandle\n");
    exit(-1);
  }

  /* 
   * Open logger to filename ./vanessa_logger_sample.log
   */
  log_fn=vanessa_logger_openlog_filename(
    "./vanessa_logger_sample.log", 
    "vanessa_logger_sample", 
    LOG_DEBUG,
    0
  );
  if(log_fn==NULL){
    fprintf(stderr, "Error: vanessa_logger_openlog_filename\n");
    exit(-1);
  }

  /* 
   * Open logger to syslog facility LOG_USER
   */
  log_sl=vanessa_logger_openlog_syslog(
    LOG_USER, 
    "vanessa_logger_sample", 
    LOG_DEBUG, 
    0
  );
  if(log_sl==NULL){
    fprintf(stderr, "Error: vanessa_logger_openlog_syslog\n");
    exit(-1);
  }

  /* 
   * Open logger to syslog facility "mail" by name
   */
  log_sl_bn=vanessa_logger_openlog_syslog_byname(
    "mailstix",
    "vanessa_logger_sample", 
    LOG_DEBUG, 
    0
  );
  if(log_sl==NULL){
    fprintf(stderr, "Error: vanessa_logger_openlog_syslog\n");
    exit(-1);
  }

  /*
   * Send a message to each logger
   */
  printf("Logging message to stderr\n");
  fflush(stdout);
  vanessa_logger_log(log_fh, LOG_DEBUG, "This should log to stderr: %d\n", 7);

  printf("Logging message to ./vanessa_logger_sample.log\n");
  vanessa_logger_log(
    log_fn,
    LOG_DEBUG,
    "This should log to ./vanessa_logger_sample.log: %d",
    7
  );

  printf(
    "Logging message to syslog facility LOG_USER, priority LOG_DEBUG\n"
    "If the message is not logged to syslog then you may need to add\n"
    "the following to /etc/syslog.conf and restart syslogd:\n"
    "user.debug                                    /var/log/messages\n"
  );
  vanessa_logger_log(
    log_sl, 
    LOG_DEBUG,
    "This should log to syslog facility LOG_USER, priority LOG_DEBUG: %d",
    7
  );

  printf(
    "Logging message to syslog facility LOG_MAIL (\"mail\"), priority\n"
    "LOG_DEBUG, If the message is not logged to syslog then you may need\n"
    "to add the following to /etc/syslog.conf and restart syslogd:\n"
    "user.mail                                     /var/log/mail\n"
  );
  vanessa_logger_log(
    log_sl_bn, 
    LOG_DEBUG,
    "This should log to syslog facility LOG_MAIL, priority LOG_DEBUG: %d",
    7
  );

  fflush(stderr);


  /*
   * Reopen each log
   */
  printf("\nReopening loggers\n");
  vanessa_logger_reopen(log_fh);
  vanessa_logger_reopen(log_fn);
  vanessa_logger_reopen(log_sl);

  /*
   * Send another message to each logger
   */
  printf("Logging another message to stderr\n");
  fflush(stdout);
  vanessa_logger_log(log_fh, LOG_INFO, "This should also log to stderr\n");

  printf("Logging another message to ./vanessa_logger_sample.log\n");
  vanessa_logger_log(
    log_fn, 
    LOG_INFO,
    "This should also log to ./vanessa_logger_sample.log"
  );

  printf(
    "Logging another message to syslog facility LOG_USER, priority LOG_INFO\n"
  );
  vanessa_logger_log(
    log_sl, 
    LOG_INFO,
    "This should also log to syslog facility LOG_USER, priority LOG_INFO"
  );

  printf(
    "Logging another message to syslog facility LOG_MAIL, priority LOG_INFO\n"
  );
  vanessa_logger_log(
    log_sl_bn, 
    LOG_INFO,
    "This should also log to syslog facility LOG_MAIL, priority LOG_INFO"
  );

  fflush(stderr);


  /*
   * Change the maximum priority for each logger to LOG_INFO.
   * The maximum priority is counter-intuitive and sets the
   * minimum priority that will be accepted for logging.
   */
  vanessa_logger_change_max_priority(log_fh, LOG_INFO);
  vanessa_logger_change_max_priority(log_fn, LOG_INFO);
  vanessa_logger_change_max_priority(log_sl, LOG_INFO);
  vanessa_logger_change_max_priority(log_sl_bn, LOG_INFO);

  /*
   * These messages should not get logged as their priority,
   * LOG_DEBUG, is lower than the minimum priority LOG_INFO 
   * set when each logger was opened
   */
  printf("\nTesting that logs are filtered out by priority\n");
  printf("No logs should appear after this line\n");
  fflush(stdout);
  vanessa_logger_log(log_fh, LOG_DEBUG, "This should not log to stderr\n");
  vanessa_logger_log(
    log_fn, 
    LOG_DEBUG,
    "This should not log to ./vanessa_logger_sample.log"
  );
  vanessa_logger_log(
    log_sl, 
    LOG_DEBUG,
    "This should not log to syslog facility LOG_USER, priority LOG_INFO"
  );
  vanessa_logger_log(
    log_sl_bn, 
    LOG_DEBUG,
    "This should not log to syslog facility LOG_MAIL, priority LOG_INFO"
  );

  /*
   * Close each logger
   */
  vanessa_logger_closelog(log_fh);
  vanessa_logger_closelog(log_fn);
  vanessa_logger_closelog(log_sl);
  vanessa_logger_closelog(log_sl_bn);

  return(0);
}
