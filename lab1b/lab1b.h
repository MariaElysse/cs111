/**
 * Library that holds some common functions and data structures for both client and server
 */
#ifndef LAB1B_LAB1B_H
#define LAB1B_LAB1B_H

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <mcrypt.h>

#define print_err() do {if (errno){ fprintf(stderr, "Internal Error: %s\n", strerror(errno)); exit(EXIT_FAILURE);}} while(0)
#define FD_STDIN 0
#define FD_STDOUT 1
#define BUFFER_LEN 1 //DONT CHANGE THIS
#define EF 4
#define CR 0xD
#define LF 0xA
#define INIT_VEC "FFzsxfVUjpoFfGrhgas7NMSnXfJSeKEr" //32 bytes
struct encryption_descriptors {
    MCRYPT encryption;
    MCRYPT decryption;
};
struct behavior {
    int key_len;
    char *encryption_key;
    FILE *logfile;
    unsigned short port;
    struct encryption_descriptors *encryption;
};
struct connection {
    int socket_fd;
    pid_t child_pid;
    int child_stdin;
    int child_stdouterr;
};

struct behavior *argparse(int argc, char **argv);

void move_conn(struct connection list[], size_t index, size_t len);

void encrypt(struct encryption_descriptors *descs, void *buffer, int buffer_len);

void decrypt(struct encryption_descriptors *descs, void *buffer, int buffer_len);

void end_encryption(struct encryption_descriptors descs);

struct encryption_descriptors *init_encryption(struct behavior *behavior);

#endif //LAB1B_LAB1B_H
