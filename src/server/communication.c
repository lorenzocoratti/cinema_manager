#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include "server.h"
#include "cinema_room.h"
#include "log.h"
#include "write_file.h"
#include "config.h"

/*
 * communication.c
 * Handles client connections, command parsing, and cinema seat operations.
 */

const char *wcomm = "Unknown command.\n";
const char *welcome_message = "Connection completed.\nPlease enter a command.\n";

int connection() // ret listen_fd
{
    int listen_fd, connection_fd;
    struct sockaddr_in serv_adrr;
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&serv_adrr, 0, sizeof(serv_adrr));
    serv_adrr.sin_family = AF_INET;
    serv_adrr.sin_addr.s_addr = INADDR_ANY;
    serv_adrr.sin_port = htons(PORT);

    if ((bind(listen_fd, (struct sockaddr *)&serv_adrr, sizeof(serv_adrr)) < 0))
    {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    if ((listen(listen_fd, NCLIENTS)) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    log_msg("INFO", "Server started on port %d", PORT);
    printf("Server started on port %d, waiting for clients to connect...\n", PORT);

    for (;;)
    {
        if ((connection_fd = accept(listen_fd, NULL, NULL)) < 0)
        {
            perror("accept");
            continue;
        }

        // every client handled with a thread so that server keeps listening
        pthread_t tid;
        if ((pthread_create(&tid, NULL, thread_handler, (void *)(intptr_t)connection_fd)) != 0)
        {
            perror("thread");
            close(connection_fd);
            continue;
        }
        pthread_detach(tid); // when thread terminates automatically release resources
    }
    return listen_fd;
}

void *thread_handler(void *arg)
{
    int connection_fd = (int)(intptr_t)arg;
    printf("Client fd = %d connected.\n", connection_fd);
    log_msg("INFO", "Client connected fd = %d", connection_fd);
    handle_client(connection_fd);
    log_msg("INFO", "Client disconnected: fd = %d", connection_fd);
    printf("Client fd = %d quit.\n", connection_fd);
    close(connection_fd);

    return NULL;
}

void handle_client(int connection_fd)
{

    write(connection_fd, welcome_message, strlen(welcome_message) + 1);
    char buffer[MAXBUFFER];
    int bytesread;
    int flag = 0;

    while (flag != 1)
    {
        memset(buffer, 0, MAXBUFFER);
        if ((bytesread = read(connection_fd, buffer, MAXBUFFER - 1)) < 0)
        {
            perror("read");
            close(connection_fd);
            return;
        }
        else if (bytesread == 0)
        {
            break;
        }

        buffer[bytesread] = '\0';

        if (strcmp(buffer, "quit") == 0)
        {
            printf("Client quit.\n");
            flag = 1;
            continue;
        }
        else if (strcmp(buffer, "print") == 0)
        {
            send_cinema_status(connection_fd, STATUS_FILE);
        }
        else if (strncmp(buffer, "reserve", 7) == 0)
        {
            reserve_multiple_seats(connection_fd, buffer);
        }
        else if (strcmp(buffer, "man") == 0)
        {
            print_manual(connection_fd);
        }
        else if (strncmp(buffer, "cancel ", 7) == 0)
        {
            cancel_reservation(connection_fd, buffer);
        }
        else
        {
            write(connection_fd, wcomm, strlen(wcomm) + 1);
        }
    }
    close(connection_fd);
    return;
}

void print_manual(int connection_fd)
{
    int fd = open("manual.txt", O_RDONLY);
    if (fd < 0)
    {
        perror("man open");
        return;
    }
    int bytesread;
    char buffer[MAXBUFFER];
    if ((bytesread = read(fd, buffer, MAXBUFFER)) <= 0)
    {
        perror("read");
        return;
    }
    write(connection_fd, buffer, bytesread);
    close(fd);
    return;
}

void send_cinema_status(int connection_fd, const char *filename)
{
    int fd, bytesread;
    char header[MAXLINE];
    char buf[MAXBUFFER];

    // get rows and columns from csv file
    int rows_columns[2];

    if (get_rows_columns(rows_columns) != 0)
    {
        perror("get rows columns");
        return;
    }
    int rows = rows_columns[0];
    int columns = rows_columns[1];

    int headerlen = snprintf(header, sizeof(header), "STATUS %d %d %s\n", rows, columns, filename);
    // send client header
    if (headerlen < 0 || headerlen >= (int)sizeof(header))
    {
        fprintf(stderr, "send_cinema_status: header too long\n");
        return;
    }
    if (write(connection_fd, header, headerlen) != headerlen)
    {
        perror("write header");
        return;
    }

    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        perror("open status file");
        return;
    }
    pthread_mutex_lock(&csv_mutex);

    int n = 0;
    while ((bytesread = read(fd, buf, sizeof(buf))) > 0)
    {
        if (write(connection_fd, buf, bytesread) != bytesread)
        {
            perror("write to client");
            close(fd);
            pthread_mutex_unlock(&csv_mutex);
            return;
        }
        n += bytesread;
    }
    if (bytesread < 0)
    {
        pthread_mutex_unlock(&csv_mutex);
        perror("read status file");
        return;
    }

    close(fd);
    pthread_mutex_unlock(&csv_mutex);
    const char *end_marker = "END_STATUS\n"; // end_marker for client
    write(connection_fd, end_marker, strlen(end_marker) + 1);
}