#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/time.h>

#include "logging.h"
#include "util.h"

static int loglevel = LOGLEVEL_INFO;

void log_init(void)
{
}

void log_setlevel(int level)
{
    loglevel = level;
}

void log_msg(int level, const char *func, const char *format, ...)
{
    va_list args;
    const char *timeformat = "%Y-%m-%dT%H:%M:%S";
    char timestr[256];
    time_t t;

    t = time(NULL);
    strftime(timestr, sizeof(timestr), timeformat, gmtime(&t));


    if (loglevel >= level || level == LOGLEVEL_CRITICAL)
    {
        if (level == LOGLEVEL_DEBUG)
            fprintf(stderr, "[%s()] ", func);
        va_start(args, format);
        fprintf(stderr, "[%s] ", timestr);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\r\n");
        va_end(args);
    }

    if (level == LOGLEVEL_CRITICAL)
        exit(1);
}

void log_msg_noln(int level, const char *func, const char *format, ...)
{
    va_list args;

    if (loglevel >= level || loglevel == LOGLEVEL_CRITICAL)
    {
        if (level == LOGLEVEL_DEBUG)
            fprintf(stderr, "[%s()] ", func);
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
    }

    if (level == LOGLEVEL_CRITICAL)
        exit(1);
}

