#ifndef _HAVE_PRINT_H
#define _HAVE_PRINT_H

#define DEBUG_LEVEL 0
#define WARNING_LEVEL 1
#define ERROR_LEVEL 2

#define PRINT_LEVEL DEBUG_LEVEL

int log_init();

void log_to_flash(const char *fmt, ...);
int log_do_erase();

void pr_debug(const char *fmt, ...);
void pr_warn(const char *fmt, ...);
void pr_err(const char *fmt, ...);

#endif