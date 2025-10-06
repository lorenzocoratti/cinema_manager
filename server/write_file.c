#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include "server.h"
#include "cinema_room.h"
#include "write_file.h"
#include "config.h"

/*
 * write_file.c
 * Handles writing and updating files for reservations, generating codes.
 */

void init_files(char *rows, char *columns)
{
    int csv_fd, log_fd, reservationcodes_fd;
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%s,%s\n", rows, columns);
    if (len < 0 || len >= sizeof(buf))
    {
        fprintf(stderr, "snprintf error\n");
        return;
    }
    csv_fd = open(STATUS_FILE, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (csv_fd < 0)
    {
        perror("open csv");
        close(csv_fd);
        exit(EXIT_FAILURE);
    }
    if (write(csv_fd, buf, len) != len)
    {
        perror("write csv");
        close(csv_fd);
        exit(EXIT_FAILURE);
    }
    close(csv_fd);

    log_fd = open(LOG_FILE, O_CREAT | O_TRUNC, 0644);
    if (log_fd < 0)
    {
        perror("open log");
        close(log_fd);
        exit(EXIT_FAILURE);
    }
    close(log_fd);

    reservationcodes_fd = open(CODES_FILE, O_CREAT | O_TRUNC, 0644);
    if (reservationcodes_fd < 0)
    {
        perror("open reservation");
        close(reservationcodes_fd);
        exit(EXIT_FAILURE);
    }
    close(reservationcodes_fd);
    printf("Config successfully done.\n");
}

int append_seat_to_csvfile(int R, int C)
{
    int fd = open(STATUS_FILE, O_WRONLY | O_APPEND);
    if (fd < 0)
    {
        perror("open for append");
        pthread_mutex_unlock(&csv_mutex);
        return -1;
    }

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%c,%d,1\n", 'A' + R, C + 1);
    if (len < 0 || len >= sizeof(buf))
    {
        fprintf(stderr, "snprintf error\n");
        pthread_mutex_unlock(&csv_mutex);
        close(fd);
        return -1;
    }
    pthread_mutex_lock(&csv_mutex);
    if (write(fd, buf, len) != len)
    {
        perror("write append");
        pthread_mutex_unlock(&csv_mutex);
        close(fd);
        return -1;
    }

    pthread_mutex_unlock(&csv_mutex);
    close(fd);
    return 0;
}

char *append_reservation_code(int R, int C, int index)
{
    char init_code[CODE_LEN];
    char line[LINE_LEN];
    int fd, len;

    // generate unique code
    gen_code(init_code, sizeof(init_code));

    // add index
    char code[CODE_LEN];
    snprintf(code, sizeof(code), "%s-%d", init_code, index);

    // write on CODES_FILE
    len = snprintf(line, sizeof(line), "%s-%c%d\n", code, 'A' + R, C + 1);
    if (len < 0 || len >= (int)sizeof(line))
    {
        fprintf(stderr, "snprintf\n");
        return NULL;
    }
    pthread_mutex_lock(&codes_mutex);

    fd = open(CODES_FILE, O_WRONLY | O_APPEND);
    if (fd < 0)
    {
        perror("open reservation codes file");
        pthread_mutex_unlock(&codes_mutex);
        return NULL;
    }
    if (write(fd, line, len) != len)
    {
        perror("write reservation code");
        pthread_mutex_unlock(&codes_mutex);
        close(fd);
        return NULL;
    }
    pthread_mutex_unlock(&codes_mutex);
    close(fd);

    // ret dynamic code : allocate memory on heap with malloc (code is a local variable)
    char *ret = malloc(strlen(code) + 1);
    if (!ret)
    {
        perror("malloc for reservation code");
        return NULL;
    }
    strcpy(ret, code);
    return ret;
}

char *gen_code(char *buffer, int lenght)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int r = rand() % 10000;
    snprintf(buffer, lenght, "%04d%02d%02d-%02d%02d%02d-%04d",
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, r);
    return buffer;
}

int remove_reservation_code(const char *code, int *outR, int *outC)
{
    pthread_mutex_lock(&codes_mutex);
    int fd = open(CODES_FILE, O_RDONLY);
    if (fd < 0)
    {
        perror("open codes read");
        pthread_mutex_unlock(&codes_mutex);
        return -1;
    }

    char prefix[64];
    snprintf(prefix, sizeof(prefix), "%s-", code);

    char line[MAXLINE];
    int idx = 0;
    int n;
    int found = 0;

    char *out = NULL;
    int outlen = 0, outcap = 0;

    while ((n = read(fd, &line[idx], 1)) == 1)
    {
        if (line[idx] == '\n' || idx + 1 == sizeof(line) - 1)
        {
            line[idx + 1] = '\0';
            if (!found && strncmp(line, prefix, strlen(prefix)) == 0)
            {
                char row;
                int seat;
                if (sscanf(line + strlen(prefix), "%c%d", &row, &seat) == 2)
                {
                    *outR = row - 'A';
                    *outC = seat - 1;
                    found = 1;
                }
            }
            if (!found || strncmp(line, prefix, strlen(prefix)) != 0)
            {
                int linelen = strlen(line);
                if (outlen + linelen + 1 > outcap)
                {
                    outcap = (outlen + linelen + 1) * 2;
                    out = realloc(out, outcap);
                    if (!out)
                    {
                        pthread_mutex_unlock(&codes_mutex);
                        perror("realloc");
                        close(fd);
                        return -1;
                    }
                }
                memcpy(out + outlen, line, linelen);
                outlen += linelen;
                out[outlen] = '\0';
            }
            idx = 0;
        }
        else
        {
            idx++;
        }
    }
    close(fd);
    if (!found)
    {
        free(out);
        pthread_mutex_unlock(&csv_mutex);
        return -1;
    }

    fd = open(CODES_FILE, O_WRONLY | O_TRUNC);
    if (fd < 0)
    {
        perror("open codes write");
        pthread_mutex_unlock(&codes_mutex);
        free(out);
        return -1;
    }
    if (write(fd, out, outlen) != (int)outlen)
    {
        perror("write codes");
        pthread_mutex_unlock(&codes_mutex);
        close(fd);
        free(out);
        return -1;
    }
    pthread_mutex_unlock(&codes_mutex);
    close(fd);
    free(out);
    return 0;
}

int remove_seat_from_file(int R, int C)
{
    pthread_mutex_lock(&csv_mutex);
    int fd = open(STATUS_FILE, O_RDONLY);
    if (fd < 0)
    {
        perror("open status read");
        pthread_mutex_unlock(&csv_mutex);
        return -1;
    }

    char line[MAXLINE];
    int idx = 0;
    int n;
    int found = 0;
    char *out = NULL;
    int outlen = 0, outcap = 0;

    while ((n = read(fd, &line[idx], 1)) == 1)
    {
        if (line[idx] == '\n' || idx + 1 == sizeof(line) - 1)
        {
            line[idx + 1] = '\0';
            char row;
            int seat, flag;
            if (!found &&
                sscanf(line, "%c,%d,%d", &row, &seat, &flag) == 3 &&
                flag == 1 &&
                row - 'A' == R &&
                seat - 1 == C)
            {
                found = 1;
            }
            else
            {
                int linelen = strlen(line);
                if (outlen + linelen + 1 > outcap)
                {
                    outcap = (outlen + linelen + 1) * 2;
                    out = realloc(out, outcap);
                    if (!out)
                    {
                        pthread_mutex_unlock(&csv_mutex);
                        perror("realloc");
                        close(fd);
                        return -1;
                    }
                }
                memcpy(out + outlen, line, linelen);
                outlen += linelen;
                out[outlen] = '\0';
            }
            idx = 0;
        }
        else
        {
            idx++;
        }
    }
    close(fd);
    if (!found)
    {
        free(out);
        pthread_mutex_unlock(&csv_mutex);

        return -1;
    }

    fd = open(STATUS_FILE, O_WRONLY | O_TRUNC);
    if (fd < 0)
    {
        perror("open status write");
        pthread_mutex_unlock(&csv_mutex);
        free(out);
        return -1;
    }
    if (write(fd, out, outlen) != (int)outlen)
    {
        perror("write status");
        pthread_mutex_unlock(&csv_mutex);
        close(fd);
        free(out);
        return -1;
    }
    pthread_mutex_unlock(&csv_mutex);
    close(fd);
    free(out);
    return 0;
}