# Cinema Reservation System

Multi-threaded client-server program in C for reserving cinema seats.

## Compile

Server: `gcc communication.c cinema_room.c write_file.c log.c config.c server.c -o server`

Client: `gcc communication.c response.c client.c -o client`

both can be compiled with `make`.

## Run

Server: `./server` or `./server config`
Client: `./client` or `./client <IP>`

## Client Commands

- `reserve A1 B2 ...` — Reserve seats 
- `cancel <code>`     — Cancel reservation 
- `print`             — Show seating 
- `man`               — Show manual 
- `quit`              — Disconnect

