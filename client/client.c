#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "client.h"

/*
 * client.c
 * --------
 * Main program for the cinema hall management client.
 * Responsibilities:
 *  - Parse command-line arguments (server IP)
 *  - Initialize the reservation codes file
 *  - Register signal handlers for clean shutdown
 *  - Establish connection to the server
 *  - Start the client-server communication routine
 */

int socket_fd;

void handle_shutdown(int sig) // signals handler
{
    close(socket_fd); // close socket upon quitting program
    exit(0);
}

void signal_handler() // intercept sigint and sigterm
{
    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);
    return;
}

void init_file() // init reservationcodes.txt upon client start
{
    int codes_fd = open(CODES_FILE, O_CREAT, 0644);
    if (codes_fd < 0)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    close(codes_fd);
    return;
}

int main(int argc, char **argv)

{
    if (argc > 2)
    {
        fprintf(stderr, "Usage: %s [ipaddress]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char ipaddress[MAXLINE];

    if (argc == 2) // ip as argv[1]
    {
        strncpy(ipaddress, argv[1], sizeof(ipaddress));
        ipaddress[sizeof(ipaddress) - 1] = '\0'; // sicurezza
    }
    else // run as localhost
    {
        strcpy(ipaddress, "127.0.0.1");
    }
    init_file();                       // init reservationcodes.txt
    signal_handler();                  // intercept signals
    socket_fd = connection(ipaddress); // enstablish socket
    welcome_message(socket_fd);
    communication_routine(socket_fd); // input-output routine
}