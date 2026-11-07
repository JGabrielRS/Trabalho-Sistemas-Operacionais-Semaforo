#define _POSIX_C_SOURCE 200809L

#include "../include/logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/stat.h>

#define LOGGER_FILE "lng.txt"
#define LOGGER_TMP_FILE "lng.txt.tmp"
#define LOGGER_SEM_NAME "/sem_block"

static sem_t *logger_sem = SEM_FAILED;

static int logger_created_sem = 0;

static int logger_lock(void)
{
    if (logger_sem == SEM_FAILED) {
        errno = EINVAL;
        return -1;
    }

    while (sem_wait(logger_sem) == -1) {
        if (errno == EINTR)
            continue;

        return -1;
    }

    return 0;
}

static int logger_unlock(void)
{
    if (logger_sem == SEM_FAILED) {
        errno = EINVAL;
        return -1;
    }

    return sem_post(logger_sem);
}

static int parse_pid_line(const char *line, pid_t *pid)
{
    char *end;
    long value;

    errno = 0;
    value = strtol(line, &end, 10);

    if (line == end || errno == ERANGE ||
        value <= 0 || (long)(pid_t)value != value) {
        return -1;
    }

    while (*end != '\0' && isspace((unsigned char)*end))
        end++;

    if (*end != '\0')
        return -1;

    *pid = (pid_t)value;

    return 0;
}

int logger_init(void)
{
    FILE *file = NULL;
    int locked = 0;
    int saved_errno = 0;

    if (logger_sem != SEM_FAILED)
        return 0;

    logger_sem = sem_open(
        LOGGER_SEM_NAME,
        O_CREAT | O_EXCL,
        0600,
        1
    );

    if (logger_sem != SEM_FAILED) {
        logger_created_sem = 1;
    } else if (errno == EEXIST) {
        logger_sem = sem_open(LOGGER_SEM_NAME, 0);
    }

    if (logger_sem == SEM_FAILED)
        return -1;

    if (logger_lock() == -1) {
        saved_errno = errno;
        goto error;
    }

    locked = 1;

    file = fopen(LOGGER_FILE, "a");

    if (file == NULL) {
        saved_errno = errno;
        goto error;
    }

    if (fclose(file) == EOF) {
        file = NULL;
        saved_errno = errno;
        goto error;
    }

    file = NULL;

    if (logger_unlock() == -1) {
        locked = 0;
        saved_errno = errno;
        goto error;
    }

    locked = 0;

    return 0;

error:

    if (file != NULL)
        fclose(file);

    if (locked)
        logger_unlock();

    sem_close(logger_sem);
    logger_sem = SEM_FAILED;

    if (logger_created_sem) {
        sem_unlink(LOGGER_SEM_NAME);
        logger_created_sem = 0;
    }

    errno = saved_errno;
    return -1;
}

int logger_append_pid(pid_t pid)
{
    FILE *file = NULL;
    int result = -1;
    int saved_errno = 0;

    if (pid <= 0 || logger_sem == SEM_FAILED) {
        errno = EINVAL;
        return -1;
    }

    if (logger_lock() == -1)
        return -1;

    file = fopen(LOGGER_FILE, "a");

    if (file == NULL) {
        saved_errno = errno;
        goto finish;
    }

    if (fprintf(file, "%ld\n", (long)pid) < 0) {
        saved_errno = errno;
        goto finish;
    }

    if (fclose(file) == EOF) {
        file = NULL;
        saved_errno = errno;
        goto finish;
    }

    file = NULL;
    result = 0;

finish:

    if (file != NULL)
        fclose(file);

    if (logger_unlock() == -1 && result == 0) {
        result = -1;
        saved_errno = errno;
    }

    if (result == -1 && saved_errno != 0)
        errno = saved_errno;

    return result;
}

int logger_process_batch(pid_t pids[], int max_pids)
{
    FILE *input = NULL;
    FILE *output = NULL;

    char *line = NULL;
    size_t line_capacity = 0;
    ssize_t line_length;

    int count = 0;
    int result = -1;
    int saved_errno = 0;

    if (pids == NULL || max_pids <= 0 ||
        logger_sem == SEM_FAILED) {
        errno = EINVAL;
        return -1;
    }

    if (logger_lock() == -1)
        return -1;

    input = fopen(LOGGER_FILE, "r");

    if (input == NULL) {
        saved_errno = errno;
        goto finish;
    }

    output = fopen(LOGGER_TMP_FILE, "w");

    if (output == NULL) {
        saved_errno = errno;
        goto finish;
    }

    while ((line_length = getline(
                &line,
                &line_capacity,
                input)) != -1) {

        pid_t pid;

        (void)line_length;

        if (parse_pid_line(line, &pid) == -1) {
            saved_errno = EINVAL;
            goto finish;
        }

        if (count < max_pids) {
            pids[count++] = pid;
        }

        else if (fprintf(output, "%ld\n", (long)pid) < 0) {
            saved_errno = errno;
            goto finish;
        }
    }

    if (ferror(input)) {
        saved_errno = errno ? errno : EIO;
        goto finish;
    }

    if (fclose(input) == EOF) {
        input = NULL;
        saved_errno = errno;
        goto finish;
    }

    input = NULL;

    if (fflush(output) == EOF) {
        saved_errno = errno;
        goto finish;
    }

    if (fclose(output) == EOF) {
        output = NULL;
        saved_errno = errno;
        goto finish;
    }

    output = NULL;

    if (rename(LOGGER_TMP_FILE, LOGGER_FILE) == -1) {
        saved_errno = errno;
        goto finish;
    }

    result = count;

finish:

    if (input != NULL)
        fclose(input);

    if (output != NULL)
        fclose(output);

    free(line);

    if (result == -1)
        unlink(LOGGER_TMP_FILE);

    if (logger_unlock() == -1 && result >= 0) {
        result = -1;
        saved_errno = errno;
    }

    if (result == -1 && saved_errno != 0)
        errno = saved_errno;

    return result;
}

int logger_has_pending(void)
{
    FILE *file = NULL;

    char *line = NULL;
    size_t line_capacity = 0;
    ssize_t line_length;

    pid_t pid;

    int result = -1;
    int saved_errno = 0;

    if (logger_sem == SEM_FAILED) {
        errno = EINVAL;
        return -1;
    }

    if (logger_lock() == -1)
        return -1;

    file = fopen(LOGGER_FILE, "r");

    if (file == NULL) {
        saved_errno = errno;
        goto finish;
    }

    line_length = getline(&line, &line_capacity, file);

    if (line_length == -1) {

        if (ferror(file)) {
            saved_errno = errno ? errno : EIO;
            goto finish;
        }

        result = 0;
    }

    else if (parse_pid_line(line, &pid) == -1) {
        saved_errno = EINVAL;
        goto finish;
    }

    else {
        result = 1;
    }

finish:

    if (file != NULL)
        fclose(file);

    free(line);

    if (logger_unlock() == -1 && result >= 0) {
        result = -1;
        saved_errno = errno;
    }

    if (result == -1 && saved_errno != 0)
        errno = saved_errno;

    return result;
}

int logger_destroy(void)
{
    int result = 0;
    int saved_errno = 0;

    if (logger_sem == SEM_FAILED)
        return 0;

    if (sem_close(logger_sem) == -1) {
        result = -1;
        saved_errno = errno;
    }

    logger_sem = SEM_FAILED;

    if (logger_created_sem) {

        if (sem_unlink(LOGGER_SEM_NAME) == -1 &&
            errno != ENOENT && result == 0) {

            result = -1;
            saved_errno = errno;
        }

        logger_created_sem = 0;
    }

    if (result == -1)
        errno = saved_errno;

    return result;
}

