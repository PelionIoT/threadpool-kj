/*
 * logging.h
 *
 *  Created on: Jan 24, 2010
 *      Author: ed
 */

#ifndef LOGGING_H_
#define LOGGING_H_

#include <errno.h>
#ifdef _USING_GLIBC_
#include <execinfo.h>
#endif

#include <string>

#include <TW/tw_log.h>
#include <TW/tw_syscalls.h>
#include <TW/tw_utils.h>

#ifndef DDB_LOGGING_NOCOLOR
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#else
#define ANSI_COLOR_RED     ""
#define ANSI_COLOR_GREEN   ""
#define ANSI_COLOR_YELLOW  ""
#define ANSI_COLOR_BLUE    ""
#define ANSI_COLOR_MAGENTA ""
#define ANSI_COLOR_CYAN    ""
#define ANSI_COLOR_RESET   ""
#endif


#define DEBUG_PREFIX       "DEBUG     "
//#define DEBUG_PREFIX_L     "DEBUG (%s,%d)  "
//#define DEBUG_PREFIX_LT    "DEBUG (%s,%d,LWP:%d)  "

#define MONITOR_PREFIX     "MONITOR   "
#define INFO_PREFIX        "INFO      "
#define NOTICE_PREFIX      "NOTICE    "
#define WARNING_PREFIX     "WARNING   "
#define ERROR_PREFIX       "ERROR     "
#define CRITICAL_PREFIX    "CRITICAL  "
#define ALERT_PREFIX       "ALERT     "
#define EMERGENCY_PREFIX   "EMERGENCY "

#define DDB_NEWLINE "\n"

// ## wha?? see: http://stackoverflow.com/questions/5588855/standard-alternative-to-gccs-va-args-trick

#define DDB_MONITOR( s, ... ) TWlib::TW_log::log( TW_LL_NOTIFY, MONITOR_PREFIX TW_PRE_LINE s DDB_NEWLINE, __FILE__, __LINE__, ##__VA_ARGS__ )
#define DDB_INFO( s, ... )    TWlib::TW_log::log( TW_LL_NOTIFY, ANSI_COLOR_BLUE MONITOR_PREFIX TW_PRE_LINE s ANSI_COLOR_GREEN DDB_NEWLINE, __FILE__, __LINE__, ##__VA_ARGS__ )
#define DDB_CONFIG( s, ... )  TWlib::TW_log::log( TW_LL_CONFIG, ANSI_COLOR_GREEN INFO_PREFIX TW_PRE_LINE s ANSI_COLOR_GREEN DDB_NEWLINE, __FILE__, __LINE__, ##__VA_ARGS__ )
#define DDB_WARN( s, ... )    TWlib::TW_log::log( TW_LL_WARN, ANSI_COLOR_YELLOW WARNING_PREFIX TW_PRE_LINE s ANSI_COLOR_RESET DDB_NEWLINE , __FILE__, __LINE__, ##__VA_ARGS__ )
#define DDB_ERROR( s, ... )   TWlib::TW_log::log( TW_LL_ERROR, ANSI_COLOR_RED ERROR_PREFIX TW_PRE_LINE s ANSI_COLOR_RESET DDB_NEWLINE, __FILE__, __LINE__, ##__VA_ARGS__ )
#define DDB_ALERT( s, ... )   TWlib::TW_log::log( TW_LL_ERROR,  ANSI_COLOR_MAGENTA ALERT_PREFIX TW_PRE_LINE s ANSI_COLOR_RESET DDB_NEWLINE, __FILE__, __LINE__, ##__VA_ARGS__ )
#define DDB_CRITICAL( s, ... )  TWlib::TW_log::log( TW_LL_CRITICAL, CRITICAL_PREFIX TW_PRE_LINE s DDB_NEWLINE, __FILE__, __LINE__, ##__VA_ARGS__ )
//#define TW_CRASH( s, ... )  { TWlib::TW_log::log( TW_LL_CRASH, TW_PREFIX_CRASH s, __VA_ARGS__ ); }
//#define TW_CRASH_LT( s, ... )  { TWlib::TW_log::log( TW_LL_CRASH, TW_PREFIX_CRASH TW_PRE_LINE_THREAD s, __FILE__, __LINE__, _TW_LWP_, __VA_ARGS__ ); }
#define DDB_DEBUG( s, ... )  TWlib::TW_log::log( TW_LL_DEBUG1, ANSI_COLOR_RESET DEBUG_PREFIX TW_PRE_LINE s DDB_NEWLINE, __FILE__, __LINE__, ##__VA_ARGS__ )
#define DDB_DEBUGLT( s, ... ) TWlib::TW_log::log( TW_LL_DEBUG1, ANSI_COLOR_RESET DEBUG_PREFIX TW_PRE_LINE_THREAD s DDB_NEWLINE, __FILE__, __LINE__, _TW_LWP_, ##__VA_ARGS__ )


#ifndef CRANK_INFO
#define CRANK_INFO( s, ... ) DDB_INFO( s, ##__VA_ARGS__ )
#endif
#ifndef CRANK_ERROR
#define CRANK_ERROR( s, ... ) DDB_ERROR( s, ##__VA_ARGS__ )
#endif
#ifndef CRANK_CRITICAL
#define CRANK_CRITICAL( s, ... ) DDB_CRITICAL( s, ##__VA_ARGS__ )
#endif
#ifndef CRANK_DEBUG
#define CRANK_DEBUG( s, ... ) DDB_DEBUG( s, ##__VA_ARGS__ )
#endif
#ifndef CRANK_DEBUGL
#define CRANK_DEBUGL( s, ... ) DDB_DEBUG( s, ##__VA_ARGS__ )
#endif
#ifndef CRANK_DEBUGLT
#define CRANK_DEBUGLT( s, ... ) DDB_DEBUGLT( s, ##__VA_ARGS__ )
#endif

#endif /* LOGGING_H_ */



