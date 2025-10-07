#ifndef SYNC_H
#define SYNC_H

#include <pthread.h>

extern pthread_mutex_t csv_mutex;
extern pthread_mutex_t log_mutex;
extern pthread_mutex_t codes_mutex;

void handle_shutdown(int sig);
void initialize_mutex();
void signal_handler();
void get_info();

#endif