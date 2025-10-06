#ifndef LOG_H
#define LOG_H

void log_init(char *log_filename);
void log_msg(char *level, char *format, ...);
void log_close();

#endif