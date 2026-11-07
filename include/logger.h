#ifndef LOGGER_H
#define LOGGER_H

#include <sys/types.h>

int logger_init(void);

int logger_append_pid(pid_t pid);

int logger_process_batch(pid_t pids[], int max_pids);

int logger_has_pending(void);

int logger_destroy(void);

#endif