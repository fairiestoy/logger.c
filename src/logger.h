#ifndef KVK_LOGGER_H
#define KVK_LOGGER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#ifdef LOGGERTESTSUITE

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <unistd.h>

#endif /* Test Suite */

/*
To not collide with Linux Syslog, we add a check for certain values
and define accordingly.

Syslog should be included before the logging library, otherwise it
might result in issues with compilation.

TODO: Find a better way to determine if syslog is available
*/
#if defined(LOG_EMERG) && defined(LOG_ALERT) && defined(LOG_CRIT) && defined(LOG_ERR) && defined(LOG_WARNING) && defined(LOG_NOTICE) && defined(LOG_INFO) && defined(LOG_DEBUG)
  enum {
    LOGGER_EMERGENCY = LOG_EMERG,
    LOGGER_ALERT = LOG_ALERT,
    LOGGER_CRITICAL = LOG_CRIT,
    LOGGER_ERROR = LOG_ERR,
    LOGGER_WARNING = LOG_WARNING,
    LOGGER_NOTICE = LOG_NOTICE,
    LOGGER_INFO = LOG_INFO,
    LOGGER_DEBUG = LOG_DEBUG
  };
#else
  enum {
    LOGGER_EMERGENCY = 0,
    LOGGER_ALERT = 1,
    LOGGER_CRITICAL = 2,
    LOGGER_ERROR = 3,
    LOGGER_WARNING = 4,
    LOGGER_NOTICE = 5,
    LOGGER_INFO = 6,
    LOGGER_DEBUG = 7
  };
#endif

typedef int (*logger_push_log)(void const * const,char const * const);
typedef char *(*logger_transform)(time_t const,int const,char const * const, int const,char *);

#define logger_emergency(...) logger_log(LOGGER_EMERGENCY,__FILE__,__LINE__,__VA_ARGS__)
#define logger_alert(...) logger_log(LOGGER_ALERT,__FILE__,__LINE__,__VA_ARGS__)
#define logger_critical(...) logger_log(LOGGER_CRITICAL,__FILE__,__LINE__,__VA_ARGS__)
#define logger_error(...) logger_log(LOGGER_ERROR,__FILE__,__LINE__,__VA_ARGS__)
#define logger_warning(...) logger_log(LOGGER_WARNING,__FILE__,__LINE__,__VA_ARGS__)
#define logger_notice(...) logger_log(LOGGER_NOTICE,__FILE__,__LINE__,__VA_ARGS__)
#define logger_info(...) logger_log(LOGGER_INFO,__FILE__,__LINE__,__VA_ARGS__)
#define logger_debug(...) logger_log(LOGGER_DEBUG,__FILE__,__LINE__,__VA_ARGS__)

#define LOGGER_MESSAGE_BUFFER 2048

extern void logger_log(int,char const * const,int,char const * const, ...);
extern int logger_setup_context(int,void *,logger_push_log,logger_transform,bool);
extern int logger_set_output_callback(logger_push_log);
extern int logger_set_loglevel(int);
extern int logger_set_transform(logger_transform);
extern void logger_toggle(bool);
extern bool logger_get_status(void);
extern bool logger_is_initialized(void);
extern int logger_factory_console(int);
extern int logger_factory_file(int,char const * const);

#endif // HEADER CHECK
