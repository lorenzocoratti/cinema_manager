
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include "log.h"
#include "server.h"
#include "cinema_room.h"
#include "write_file.h"

/*
 * cinema_room.c
 * Handles cinema operations: reservation, cancellation, checking seat status.
 */

int get_rows_columns(int array[2])
{
    int rows, columns, csv_fd;
    char strbuf[16];
    char *pointer;
    int bytesread;

    csv_fd = open(STATUS_FILE, O_RDONLY);
    if (csv_fd < 0)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    bytesread = read(csv_fd, strbuf, 16);
    if (bytesread <= 0)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    strbuf[bytesread] = '\0';
    rows = strtol(strbuf, &pointer, 10); // read first int
    if (*pointer == ',')
    {
        pointer++;                           // ignore ,
        columns = strtol(pointer, NULL, 10); // read second int
    }
    array[0] = rows;
    array[1] = columns;
    return 0;
}

void reserve_multiple_seats(int connection_fd, char *buffer)
{
    int rows_columns[2];
    if (get_rows_columns(rows_columns) != 0)
    {
        perror("get rows columns");
        exit(EXIT_FAILURE);
    }
    int rows = rows_columns[0];
    int columns = rows_columns[1];

    int Rlist[MAXBUFFER], Clist[MAXBUFFER];
    int count = 0;

    // remove \n
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
        buffer[len - 1] = '\0';

    // ignore "reserve "
    char *ptr = buffer + 8;

    char *token = strtok(ptr, " ");
    while (token != NULL && count < 32)
    {
        char row_char;
        int seat_num, pos;
        if (sscanf(token, " %c%d%n", &row_char, &seat_num, &pos) != 2 || token[pos] != '\0')
        {
            write(connection_fd, "Invalid seat format.\n", 22);
            return;
        }

        int R = row_char - 'A';
        int C = seat_num - 1;

        if (R < 0 || R >= rows || C < 0 || C >= columns)
        {
            write(connection_fd, "Error, seat out of range.\n", 27);
            return;
        }
        for (int j = 0; j < count; j++)
        {
            if (Rlist[j] == R && Clist[j] == C)
            {
                char msg[64];
                snprintf(msg, sizeof(msg), "Duplicate seat %c%d in request.\n", 'A' + R, C + 1);
                write(connection_fd, msg, strlen(msg) + 1);
                return;
            }
        }

        Rlist[count] = R;
        Clist[count] = C;
        count++;

        token = strtok(NULL, " "); // next seat
    }

    if (count == 0)
    {
        write(connection_fd, "No seats specified.\n", 21);
        return;
    }

    // verify all seats
    for (int i = 0; i < count; i++)
    {
        if (is_seat_taken(Rlist[i], Clist[i]))
        {
            char msg[64];
            snprintf(msg, sizeof(msg), "Error, Seat %c%d already taken.\n",
                     'A' + Rlist[i], Clist[i] + 1);
            write(connection_fd, msg, strlen(msg) + 1);
            return;
        }
    }

    // all seats free, reserve
    char all_msgs[MAXBUFFER];
    for (int i = 0; i < count; i++)
    {
        if (append_seat_to_csvfile(Rlist[i], Clist[i]) != 0)
        {
            strcat(all_msgs, "Error booking seat.\n");
            break;
        }

        char *code = append_reservation_code(Rlist[i], Clist[i], i);
        if (!code)
        {
            strcat(all_msgs, "Error generating reservation code.\n");
            break;
        }

        char msg[128];
        snprintf(msg, sizeof(msg), "Seat %c%d booked. Code: %s\n",
                 'A' + Rlist[i], Clist[i] + 1, code);
        strcat(all_msgs, msg);
        log_msg("BOOKING", "Client fd = %d reserved seat %c%d with code %s", connection_fd, 'A' + Rlist[i], Clist[i] + 1, code);
        free(code);
    }

    write(connection_fd, all_msgs, strlen(all_msgs));
}

int is_seat_taken(int R, int C)
{
    int fd = open(STATUS_FILE, O_RDONLY);
    if (fd < 0)
    {
        perror("open for read");
        return 0; // free if file is non existent
    }

    char line[MAXLINE];
    int idx = 0;
    int bytesread;

    while ((bytesread = read(fd, &line[idx], 1)) == 1)
    {
        if (line[idx] == '\n' || idx >= (int)sizeof(line) - 2)
        {
            line[idx + 1] = '\0';
            char row_char;
            int seat_num, flag;
            if (sscanf(line, "%c,%d,%d", &row_char, &seat_num, &flag) == 3)
            {
                if (flag == 1 && row_char - 'A' == R && seat_num - 1 == C)
                {
                    close(fd);
                    return 1;
                }
            }
            idx = 0;
        }
        else
        {
            idx++;
        }
    }

    if (bytesread < 0)
        perror("read");
    close(fd);
    return 0;
}

void cancel_reservation(int connection_fd, char *buffer)
{
    char code[MAXLINE]; // read input
    if (sscanf(buffer, "cancel %127s", code) != 1)
    {
        const char *err = "Invalid command, use cancel <code>.\n";
        write(connection_fd, err, strlen(err));
        return;
    }

    // if !22 return
    if (strlen(code) != 22)
    {
        const char *err = "Invalid reservation code length.\n";
        write(connection_fd, err, strlen(err) + 1);
        return;
    }

    int R, C;
    if (remove_reservation_code(code, &R, &C) < 0)
    {
        const char *err = "Reservation code not found.\n";
        write(connection_fd, err, strlen(err) + 1);
        return;
    }

    if (remove_seat_from_file(R, C) < 0)
    {
        const char *err = "Internal error removing seat.\n";
        write(connection_fd, err, strlen(err) + 1);
        return;
    }

    log_msg("INFO", "Client fd = %d cancelled reservation with code %s", connection_fd, code);

    char resp[MAXBUFFER];
    snprintf(resp, sizeof(resp), "Reservation %s canceled, seat %c%d is now free.\n", code, 'A' + R, C + 1);
    write(connection_fd, resp, strlen(resp));
}
