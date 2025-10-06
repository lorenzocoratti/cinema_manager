#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>

/*
 * communication.c
 * ----------------
 * Handles user input and server communication routines for the client:
 *  - Establishes a separate thread to listen for server responses
 *  - Reads user commands from stdin
 *  - Sends commands to the server via TCP socket
 *  - Handles "quit" command and cleanly closes the connection
 */

int connection(char *ipaddress) // enstablish connection with server
{
    int socket_fd;
    struct sockaddr_in serv_addr;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;   // IPv4
    serv_addr.sin_port = htons(PORT); // network order port
    if (inet_pton(AF_INET, ipaddress, &serv_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    if (connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}

void welcome_message(int socket_fd)
{
    char message[MAXBUFFER];
    int bytes_read;
    if ((bytes_read = read(socket_fd, message, MAXBUFFER)) <= 0)
    {
        close(socket_fd);
        perror("read");
        exit(EXIT_FAILURE);
    }
    write(1, message, bytes_read);

    return;
}

void communication_routine(int socket_fd)
{
    char client_request[MAXBUFFER];

    // create a thread that handles messages from server
    pthread_t tid;
    if (pthread_create(&tid, NULL, response_handler, (void *)(intptr_t)socket_fd) != 0)
    {
        close(socket_fd);
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // stdin routine
    while (1)
    {
        memset(client_request, 0, MAXBUFFER);
        // get input
        if (fgets(client_request, MAXBUFFER, stdin) == NULL)
        {
            close(socket_fd);
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        int len = strlen(client_request);

        if (client_request[0] == '\n') // if input is \n
        {
            printf("Please enter a command.\n");
            continue; // get next input
        }
        if (len > 0 && client_request[len - 1] == '\n')
        {
            client_request[len - 1] = '\0'; // ignore \n in input
        }
        if (strcmp(client_request, "quit") == 0)
        {
            write(socket_fd, client_request, strlen(client_request));
            break; // exit input routine
        }
        if (write(socket_fd, client_request, strlen(client_request)) < 0)
        {
            close(socket_fd);
            perror("write");
            exit(EXIT_FAILURE);
        }
    }
}