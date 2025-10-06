#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include "client.h"

/*
 * response.c
 * -----------
 * Handles server responses and reservation codes for the client:
 *  - Reads server messages continuously in a separate thread
 *  - Processes STATUS messages and prints the seat map
 *  - Writes reservation codes to reservationcodes.txt
 *  - Handles server disconnection and other errors
 */

void write_codes(int connection_fd, char *bodybuf, int nread)
{
    int codes_fd;
    codes_fd = open(CODES_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (codes_fd < 0)
    {
        close(connection_fd);
        perror("open");
        exit(EXIT_FAILURE);
    }
    int written = 0;
    while (written < nread)
    {
        int ret = write(codes_fd, bodybuf + written, nread - written);
        if (ret < 0)
        {
            close(codes_fd);
            perror("write");
            return;
        }
        written += ret;
    }
    close(codes_fd);
    write(1, bodybuf, nread);
    return;
}

void *response_handler(void *arg)
{
    int socket_fd = (intptr_t)arg; // convert thread arg
    while (1)
    {
        char header[MAXLINE];
        int header_read = 0;
        char c;

        // read until \n
        while (header_read < MAXLINE - 1)
        {
            int bytesread = read(socket_fd, &c, 1);
            if (bytesread < 0)
            {
                perror("read");
                close(socket_fd);
                exit(EXIT_FAILURE);
            }
            else if (bytesread == 0) // server disconnected.
            {
                printf("\n[!] Server disconnected.\n");
                close(socket_fd);
                exit(EXIT_FAILURE);
            }
            header[header_read++] = c;
            if (c == '\n') // read until \n
                break;
        }
        header[header_read] = '\0';

        int nread = strlen(header);

        // if == Seat write in reservationcodes.txt
        if (strncmp(header, "Seat ", 5) == 0)
        {
            write_codes(socket_fd, header, nread);
            continue;
        }
        // if != STATUS write output
        else if (strncmp(header, "STATUS ", 7) != 0)
        {
            write(1, header, header_read);
            char buf[MAXBUFFER];
            int n = read(socket_fd, buf, MAXBUFFER);
            if (n > 0)
                write(1, buf, n);
            continue; // keep on reading...
        }
        // == STATUS, print command, get rows and columns
        int ROWS, COLUMNS;
        if (sscanf(header, "STATUS %d %d\n", &ROWS, &COLUMNS) != 2)
        {
            printf("Invalid STATUS header: %s\n", header);
            continue;
        }

        int seats[ROWS][COLUMNS];
        memset(seats, 0, sizeof(seats));

        char bodybuf[MAXBUFFER];
        char line[MAXLINE];
        int pos = 0;
        while (1)
        {
            int bytesread = read(socket_fd, bodybuf, sizeof(bodybuf));
            if (bytesread <= 0)
            {
                printf("\n[!] Server disconnected.\n");
                close(socket_fd);
                exit(0);
            }
            for (int i = 0; i < bytesread; i++)
            {
                char ch = bodybuf[i];
                line[pos++] = ch;
                if (ch == '\n' || pos == sizeof(line) - 1)
                {
                    line[pos] = '\0';

                    if (strcmp(line, "END_STATUS\n") == 0)
                    {
                        goto done_reading;
                    }

                    int tmp1, tmp2;
                    if (sscanf(line, "%d,%d", &tmp1, &tmp2) == 2)
                    {
                        pos = 0;  // reset buffer
                        continue; // skip first line
                    }

                    char row_char;
                    int seat_num, flag;
                    if (sscanf(line, "%c,%d,%d", &row_char, &seat_num, &flag) == 3 && flag == 1)
                    {
                        int r = row_char - 'A';
                        int c = seat_num - 1;
                        if (r >= 0 && r < ROWS && c >= 0 && c < COLUMNS)
                            seats[r][c] = 1;
                    }
                    pos = 0;
                }
            }
        }
    done_reading:
        // print matrix
        printf("  ");
        for (int c = 0; c < COLUMNS; c++)
        {
            printf("%2d ", c + 1);
        }
        printf("\n");
        for (int r = 0; r < ROWS; r++)
        {
            printf("%c ", 'A' + r);
            for (int c = 0; c < COLUMNS; c++)
                printf(" %c ", seats[r][c] ? 'X' : '.');
            printf("\n");
        }
    }
    return NULL;
}
