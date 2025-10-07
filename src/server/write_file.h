#ifndef WRITE_FILE_H
#define WRITE_FILE_H

#define STATUS_FILE "../fileserver/cinema_status.csv"
#define CODES_FILE "../fileserver/reservation_codes.txt"
#define LOG_FILE "../fileserver/server.log"
#define CODE_LEN 128
#define LINE_LEN 64
#define MAXLINE 128

void init_files(char *rows, char *columns);
int append_seat_to_csvfile(int R, int C);
char *append_reservation_code(int R, int C, int index);
char *gen_code(char *buffer, int lenght);
int remove_reservation_code(const char *code, int *outR, int *outC);
int remove_seat_from_file(int R, int C);

#endif