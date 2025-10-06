#ifndef CINEMA_ROOM_H
#define CINEMA_ROOM_H

int is_seat_taken(int Row, int Column);
void reserve_multiple_seats(int connection_fd, char *buffer);
int get_rows_columns(int array[2]);

#endif