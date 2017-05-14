//
// Created by maria on 4/23/17.
//
#include "lab1b.h"
#include <string.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <stdbool.h>

struct connection *new_conn(int listen_fd) {
    struct pollfd ready_conn;
    ready_conn.fd = listen_fd;
    ready_conn.events = POLLIN;
    if (poll(&ready_conn, 1, 0) && (ready_conn.revents & POLLIN)) {
        print_err();
        struct connection *new_c = calloc(1, sizeof(struct connection));
        struct sockaddr_storage remote_addr;
        socklen_t remote_addr_len = sizeof(struct sockaddr_storage);
        new_c->socket_fd = accept(listen_fd, (struct sockaddr *) &remote_addr, &remote_addr_len);
        return new_c;
    } else {
        return NULL;
    }
} //TODO: ALWAYS FREE THESE STRUCTS, ALWAYS. VALGRIND IS NOT HAPPY
//leaking memory in a server makes the users angery

void usage() {
    fprintf(stderr, "Usage: \n");
    fprintf(stderr, "--port=NUM\tSpecify a port to listen for connections from clients\n");
    fprintf(stderr, "--encrypt=KEY\tSpecify that encryption should be used, and the keyfile to use\n");
    exit(EXIT_FAILURE);
}

void shutdown_process(pid_t pid) {
    int wstatus = 0;
    waitpid(pid, &wstatus, 0);
    fprintf(stderr, "SHELL EXIT SIGNAL=%i STATUS=%i\n", WTERMSIG(wstatus), WEXITSTATUS(wstatus));
}

void handler(int sig) {
    if (sig == SIGPIPE) {
        shutdown_process(-1);
    }
}

void shell(int listen_sockfd, struct encryption_descriptors *encryption) {
    struct connection *connections = NULL;
    size_t num_conns = 0;
    bool connection_made = false;
    for (;;) { //ALIEN FACE
        struct connection *conn = new_conn(listen_sockfd);
        if (conn) {
            int to_child[2];
            int from_child[2];
            if (pipe(to_child) || pipe(from_child)) {
                print_err();
            }
            conn->child_pid = fork();
            print_err();
            if (conn->child_pid) {
                signal(SIGPIPE, handler);
                print_err();
                close(to_child[0]);
                print_err();
                close(from_child[1]);
                print_err();
                conn->child_stdin = to_child[1];
                conn->child_stdouterr = from_child[0];
                ++num_conns;
                connections = realloc(connections, num_conns * sizeof(struct connection));
                print_err();
                connections[num_conns - 1] = *conn;
                free(conn);
                print_err(); //STRUCT IS FREED, VALGRIND IS HAPPY
                connection_made = true;
            } else {
                dup2(to_child[0], 0);
                print_err();
                dup2(from_child[1], 1);
                print_err();
                dup2(from_child[1], 2);
                print_err();
                close(from_child[1]);
                print_err();
                close(to_child[0]);
                print_err();
                close(to_child[1]);
                print_err();
                close(from_child[0]);
                print_err();
                char *cmd_to_execute = "/bin/bash";
                char *arguments[2];
                arguments[0] = cmd_to_execute;
                arguments[1] = 0;
                execvp(cmd_to_execute, arguments);
                print_err();
                //started the child process, now hopefully all is well
            }
        }
        if (num_conns == 0) {
            if (!connection_made) { //the server is expected to quit after the first connection is closed
                continue; //this maintains that arbitrary limitation
            } else { //technically, the server can continue similar to a telnet server
                break; //if you comment this entire if block that will happen
            }
        }
        for (size_t i = 0; i < num_conns; ++i) { //iterate along the connections we have open rn
            char buffer[BUFFER_LEN];
            struct pollfd polls[2]; //0 = child process, 1 = socket input
            polls[0].fd = connections[i].child_stdouterr;
            polls[0].events = POLLIN;
            polls[1].fd = connections[i].socket_fd;
            polls[1].events = POLLIN;
            ssize_t bytes_read;
            if (poll(polls, 2, 0)) {
                if (polls[0].revents & POLLIN) {
                    bytes_read = read(polls[0].fd, buffer, BUFFER_LEN);
                    //writing from the child process to the screen (socket)
                    for (ssize_t j = 0; j < bytes_read; ++j) {
                        if (buffer[j] == EF) { //EOF from the shell process
                            shutdown_process(connections[i].child_pid);
                        }
                        encrypt(encryption, buffer, 1);
                        if (write(polls[1].fd, buffer, 1) != 1) {
                            print_err();
                        }
                    }
                }
                if (polls[0].revents & (POLLERR | POLLHUP)) {
                    close(connections[i].socket_fd);
                    print_err();
                    shutdown_process(connections[i].child_pid);
                    move_conn(connections, i, num_conns);
                    --num_conns;
                    connections = realloc(connections, sizeof(struct connection) * num_conns);
                    print_err();
                    continue; //todo: this may not be necessary
                }
                if (polls[1].revents & POLLIN) { //there are things to read from the socket
                    bytes_read = read(polls[1].fd, buffer,
                                      BUFFER_LEN); //read from the socket (or in 1a parlance, the kb)
                    print_err();
                    //reading from what was sent on the socket
                    decrypt(encryption, buffer, bytes_read);
                    for (ssize_t j = 0; j < bytes_read; ++j) {
                        if (buffer[j] == 0x03) {
                            kill(connections[i].child_pid, SIGINT);
                            print_err();
                        } else if (buffer[j] == CR || buffer[j] == LF) {
                            if (write(connections[i].child_stdin, "\n", 1) != 1) {
                                print_err();
                            }
                        } else if (buffer[j] == EF) {
                            kill(connections[i].child_pid, SIGTERM);
                        } else {
                            if (write(connections[i].child_stdin, buffer + j, 1) != 1) {
                                print_err();
                            }
                        }
                        if (buffer[j] == EF || buffer[j] == 0x3) {
                            close(connections[i].socket_fd);
                            print_err();
                            close(connections[i].child_stdin);
                            print_err();
                            shutdown_process(connections[i].child_pid);
                            move_conn(connections, i, num_conns);
                            --num_conns; //close connection
                            connections = realloc(connections, sizeof(struct connection) * num_conns);
                            continue; //todo: this may not be necessary
                        }

                    }
                }
            }
        }
    }
}


int setup_socket(unsigned short int port) {
    char *port_str = calloc(sizeof(char), 6);
    int sockfd;
    struct addrinfo hints, *res;
    sprintf(port_str, "%u", port);
    print_err();
    memset(&hints, 0, sizeof(struct addrinfo));
    print_err();
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo("127.0.0.1", port_str, &hints, &res);
    print_err();
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    print_err();
    bind(sockfd, res->ai_addr, res->ai_addrlen);
    print_err();
    listen(sockfd, 10);
    print_err();
    return sockfd;
}

int main(int argc, char **argv) {
    struct behavior *b = argparse(argc, argv);
    if (b->encryption_key) {
        b->encryption = init_encryption(b);
    }
    int sockfd = setup_socket(b->port);
    shell(sockfd, (b->encryption));
    if (b->encryption) {
        end_encryption(*(b->encryption));
    }
}