#ifndef SERVER_H
#define SERVER_H

#define PORT 12345
#define MAXBUFFER 512
#define MAXLINE 128
#define NCLIENTS 20

int connection();
void *thread_handler(void *arg);
void handle_client(int connection_fd);
void print_manual(int connection_fd);
void cancel_reservation(int connection_fd, char *buffer);
void send_cinema_status(int connection_fd, const char *filename);
void handle_shutdown(int sig);
void signal_handler();
void initialize_mutex();
void config(char *argv1);

#endif // SERVER_H