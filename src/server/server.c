#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include "write_file.h"
#include "log.h"
#include <signal.h>
#include "config.h"
#include "server.h"

/*
 * server.c
 * Main server program: initializes server, handles signals, logging, and starts listening for clients.
 */

pthread_mutex_t csv_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t codes_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
int listen_fd;

int main(int argc, char **argv)
{
    if (argc > 2)
    {
        printf("please enter argc <= 2, argv[1] = NULL or 'config'\n");
        exit(0);
    }
    config(argv[1]);             // config server
    signal_handler();            // intercept signals
    srand((unsigned)time(NULL)); // initialize seed for code generation
    initialize_mutex();
    listen_fd = connection(); // enstablish socket
}

void handle_shutdown(int sig)
{
    log_msg("INFO", "Server shut down on port %d", PORT);
    log_close();
    close(listen_fd);
    exit(0);
}

void initialize_mutex()
{
    pthread_mutex_init(&csv_mutex, NULL);
    pthread_mutex_init(&log_mutex, NULL);
    pthread_mutex_init(&codes_mutex, NULL);
    return;
}

void signal_handler()
{
    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);
    signal(SIGPIPE, SIG_IGN);
    return;
}