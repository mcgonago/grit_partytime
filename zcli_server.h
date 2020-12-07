#ifndef _ZCLI_SERVER_H_
#define _ZCLI_SERVER_H_

#include <syslog.h>

// +++owen - YOU CANNOT REDIRECT using '>' from the command line to a /var/log/{file}
// +++owen - YOU MUST use the /bin/logger facility OR change your program to use openlog/syslog
// used fopen/fwrite facility (stdout)
#define SERVER_LOG_TMP "/var/tmp/ocli-server.log"

// uses openlog/syslog facility (syslogd)
#define SERVER_LOG_IDENTIFIER "ocli-server"

extern FILE *logfile;

int log_printf(const char *format, ...);
int log_printf_columns(const char *file, const char *function, int line, const char *status, const char *format, ...);
int log_tb_printf_columns(char *file, const char *function, int line, const char *status, const char *format, ...);

#define LOG_PRINTF(format, args...) log_printf_columns(__FILE__, __FUNCTION__, __LINE__, "OK", format, ##args)

typedef struct codecTable_s
{
    int xstatIdx;
    int pt;
    const char *codecName;
} codecTable_t;

extern codecTable_t codecTable[];

#endif  /* _ZCLI_SERVER_H_ */
