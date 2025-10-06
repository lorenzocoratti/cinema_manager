#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "write_file.h"

/*
 * config.c
 * Handles server configuration: sets cinema room dimensions, initializes files if necessary.
 */

void config(char *arg)
{
    if (arg == NULL)
    {
        int csv_fd, reservation_codes_fd, log_fd;
        if (((csv_fd = open(STATUS_FILE, O_RDONLY)) < 0) || ((reservation_codes_fd = open(CODES_FILE, O_RDONLY)) < 0) || ((log_fd = open(LOG_FILE, O_RDONLY)) < 0))
        {
            printf("Files not found, forcing config.\n");
            get_info();
            return;
        }

        return;
    }
    else if ((strcmp(arg, "config")) == 0)
    {
        get_info();
        return;
    }
    else
    {
        printf("please enter argv[1] as NULL or 'config'\n");
        exit(0);
    }
}

void get_info()
{
    char get_rows[MAXLINE];
    char get_col[MAXLINE];
    int rows;
    int columns;
    char *endpointer1;
    char *endpointer2;

    printf("Insert rows; 5 <= rows <= 26 : ");
    fgets(get_rows, MAXLINE, stdin);
    rows = strtol(get_rows, NULL, 10);
    int len = strlen(get_rows);
    if (len > 0 && get_rows[len - 1] == '\n')
    {
        get_rows[len - 1] = '\0';
    }

    rows = strtol(get_rows, &endpointer1, 10);
    if (*endpointer1 != '\0')
    {
        printf("Input int!\n");
        exit(EXIT_FAILURE);
    }
    if (rows < 5)
    {
        strcpy(get_rows, "5");
    }
    else if (rows > 26)
    {
        strcpy(get_rows, "26");
    }
    if (get_rows[strlen(get_rows) - 1] == '\n')
    {
        get_rows[strlen(get_rows) - 1] = '\0';
    }
    printf("Insert columns; 5 <= columns <= 30: ");
    fgets(get_col, MAXLINE, stdin);
    len = strlen(get_col);
    if (len > 0 && get_col[len - 1] == '\n')
    {
        get_col[len - 1] = '\0';
    }

    columns = strtol(get_col, &endpointer2, 10);
    if (*endpointer2 != '\0')
    {
        printf("Input int!\n");
        exit(EXIT_FAILURE);
    }
    if (columns < 5)
    {
        strcpy(get_col, "5");
    }
    else if (columns > 30)
    {
        strcpy(get_col, "30");
    }

    init_files(get_rows, get_col);
    return;
    // get input, create files and save Rows and Columns in csv file.
}
