#ifndef CLIENT_H
#define CLIENT_H

#define MAXBUFFER 1024
#define PORT 12345
#define MAXLINE 64
#define CODES_FILE "reservationcodes.txt"

int connection(char* ipaddress);
void welcome_message(int socket_fd);
void communication_routine(int socket_fd);
void *response_handler(void *arg);
void handle_shutdown(int sig);

#endif // CLIENT_H