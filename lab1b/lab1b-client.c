#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include "lab1b.h"

struct termios *init_modes;

void usage() {
    fprintf(stderr, "Usage: \n");
    fprintf(stderr, "--port=NUM:\tSpecify a port to communicate with the server\n");
    fprintf(stderr, "--encrypt=KEYFILE\tSpecify that encryption should be used, and the keyfile to use\n");
    fprintf(stderr, "--log=LOGFILE\tSpecify a file that logging information should be written to\n");
    exit(EXIT_FAILURE);
}

void comm(int sockfd, struct behavior *behavior) {
    char *buffer = malloc(BUFFER_LEN);
    struct pollfd polls[2]; //kb = 0, sock = 1
    polls[0].fd = FD_STDIN;
    polls[0].events = POLLIN;
    polls[1].fd = sockfd;
    polls[1].events = POLLIN;
    ssize_t bytes_read = 0, bytes_sent = 0;
    for (;;) { //alien face
        if (poll(polls, 2, 0)) {
            if (polls[0].revents & POLLIN) { //kb
                bytes_read = read(polls[0].fd, buffer, BUFFER_LEN);
                print_err();
                if (bytes_read >= 0) {
                    //wouldn't want to send -1 bytes now would we
                    if (*buffer == CR || *buffer == LF) {
                        if (write(FD_STDOUT, "\r\n", 2) == -1)
                            print_err();
                    } else {
                        if (write(FD_STDOUT, buffer, 1) == -1) {
                            print_err();
                        }
                    }
                    encrypt(behavior->encryption, buffer, bytes_read);
                    bytes_sent = send(sockfd, buffer, (size_t) bytes_read, 0);
                    print_err();
                    if (behavior->logfile) {
                        fprintf(behavior->logfile, "SENT %li bytes: ", bytes_sent);
                        print_err();
                        fwrite(buffer, (size_t) bytes_sent, 1, behavior->logfile);
                        print_err();
                        fwrite("\n", 1, 1, behavior->logfile);
                        print_err();
                    }
                }
                //Beej: "if it's less that 1kB, your data will "probably" all be sent in one go
                //I'm inclined to believe it, so I'm not going to complicate things with resending
            }
            if (polls[1].revents & POLLIN) {//sock
                bytes_read = recv(sockfd, buffer, BUFFER_LEN, 0);
                print_err();
                if (!bytes_read) {
                    break;
                }
                if (behavior->logfile) {
                    fprintf(behavior->logfile, "RECEIVED %li bytes: ", bytes_read);
                    print_err();
                    if (bytes_read >= 0) {
                        if (fwrite(buffer, (size_t) bytes_read, 1, behavior->logfile) == -1)
                            print_err();
                    }
                    fwrite("\n", 1, 1, behavior->logfile);
                    print_err();
                }
                decrypt(behavior->encryption, buffer, bytes_read);
                if (*buffer == CR || *buffer == LF) {
                    if (write(FD_STDOUT, "\r\n", 2) == -1) {
                        print_err();
                    }
                } else {
                    if (write(FD_STDOUT, buffer, bytes_read) == -1) {
                        print_err();
                    }
                }
            }
            if (polls[1].revents & (POLLERR | POLLHUP)) {
                break;
            }
        }
        fflush(behavior->logfile);
    }
}

int setup_socket(unsigned short int port) {
    /**
     * Opens and connects a client socket, and returns the file descriptor for use
     */
    char *port_str = calloc(sizeof(char), 6);
    sprintf(port_str, "%u", port);
    print_err();
    int sockfd = -1;
    struct addrinfo hints, *res;
    res = calloc(1, sizeof(struct addrinfo));
    print_err();
    memset(&hints, 0, sizeof(struct addrinfo));
    print_err();
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo("127.0.0.1", port_str, &hints, &res);
    print_err();
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    print_err();
    //MVP: Connect to localhost with TCP/IP
    connect(sockfd, res->ai_addr, res->ai_addrlen);
    print_err();
    return sockfd;
}

void set_terminal_modes(struct termios *initial_status) {
    /**
     * struct* is an empty termios struct that will be filled with the initial modes
     * sets the terminal modes required in the spec
     *
     * Will Set the required modes if called with a null pointer,
     * Or, set to whatever is behind the given pointer
     */
    if (!initial_status) {
        initial_status = malloc(sizeof(struct termios));
        if (tcgetattr(FD_STDIN, initial_status)) {
            fprintf(stderr, "Could not get state of current terminal: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        struct termios *set_status = malloc(sizeof(struct termios));
        memcpy(set_status, initial_status, sizeof(struct termios));
        set_status->c_lflag = ISTRIP;
        set_status->c_oflag = 0;
        set_status->c_lflag = 0; //suggested by spec
        if (tcsetattr(FD_STDIN, TCSANOW, set_status)) {
            fprintf(stderr, "Could not set status of current terminal: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        free(set_status);
    } else {
        tcsetattr(FD_STDIN, TCSANOW, initial_status);
        print_err();
    }

}

int main(int argc, char **argv) {
    init_modes = 0;
    struct behavior *behavior = argparse(argc, argv);
    if (behavior->encryption_key) {
        behavior->encryption = init_encryption(behavior);
    }
    int sock = setup_socket(behavior->port);
    set_terminal_modes(init_modes);
    comm(sock, behavior);
    if (behavior->encryption) {
        end_encryption(*(behavior->encryption));
    }
    set_terminal_modes(init_modes);
    return 0;
}

