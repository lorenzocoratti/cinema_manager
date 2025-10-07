#include "log.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "config.h"
#include "write_file.h"

void log_msg(char *level, char *fmt, ...)
{
    pthread_mutex_lock(&log_mutex);
    int log_fd = open(LOG_FILE, O_WRONLY | O_APPEND);
    if (log_fd < 0)
        return;

    char buffer[1024];
    char timestamp[64];

    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_info);

    va_list args;
    va_start(args, fmt);
    char msg[512];
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    int len = snprintf(buffer, sizeof(buffer), "[%s] [%s] %s\n",
                       timestamp, level, msg);

    write(log_fd, buffer, len);
    pthread_mutex_unlock(&log_mutex);
}

void log_close()
{

    int log_fd = open(LOG_FILE, O_RDONLY);
    if (log_fd < 0)
        return;
    if (log_fd >= 0)
    {
        close(log_fd);
        log_fd = -1;
    }
}
