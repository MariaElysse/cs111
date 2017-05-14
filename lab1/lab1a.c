/**
 * YOU'RE ABOUT TO WITNESS THE STRENGTH OF STREET KNOWLEDGE
 */

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <wait.h>

#define FD_KEYBOARD 0
#define READ_BUFFER_WIDTH 1 //as recommended by TA
#define CR 0xd
#define LF 0xa
#define EF 0x4
pid_t child_pid;
bool sh;
struct termios initial_status;
#ifdef DEBUG_PIPES
#include <time.h>
void write_log(FILE* logfile, char* msg){
    time_t t = time(0);
    fprintf(logfile, "pid: %u time: %s ", getpid(), ctime(&t));
    fprintf(logfile, "%s\n", msg);
    fflush(logfile);
}
#endif
#define print_err() do {if (errno){ fprintf(stderr, "Internal Error: %s\n", strerror(errno)); exit(EXIT_FAILURE); }} while(0)
//what a glorious macro, fuck having good error descriptions
//prints an error and quits if an errno is present
void usage(){
    fprintf(stderr, "Usage: \n"
            "--shell\tSpawn a shell (/bin/bash), passing input to this subprocess, and receiving its output\n"
            "NO OPTIONS\tEcho standard input to standard output, with some changes to newlines");
}
void handler(int sig) {
    if (!sh) {
        int returns;
        waitpid(child_pid, &returns, 0);
        print_err();
        int exit_stat = WEXITSTATUS(returns);
        int exit_sig = 0;
        if (WIFSIGNALED(returns)) {
            exit_sig = WTERMSIG(returns);
        }
        if (sig == 0) {
            exit_sig = 0;
        }
        fprintf(stderr, "SHELL EXIT SIGNAL=%i STATUS=%i\n", exit_sig, exit_stat);
    }
}
bool do_shell(int argc, char **argv) {
    int getopt_status = 0;
    int index;
    struct option options[] = {
            {"shell", optional_argument, NULL, 's'},
            {0, 0, 0, 0}
    };
    bool ret_val = false;
    while ((getopt_status = getopt_long(argc, argv, ":s", options, &index)) != -1) {
        switch (getopt_status) {
            case -1:
            case 0:
                break;
            case 's':
                ret_val = true;
                break;
            case '?':
                fprintf(stderr, "Unrecognised option given\n");
                usage();
                exit(EXIT_FAILURE);
            case ':':
                fprintf(stderr, "Argument given to option %s: %s\n", argv[index + 1], optarg);
                usage();
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "Very wrong arguments given (getopt status code: %i)\n", getopt_status);
                usage();
                exit(EXIT_FAILURE);
        }
    }
    if (index < argc -2){
        fprintf(stderr, "Unwanted argument(s) given\n");
        usage();
        exit(EXIT_FAILURE);
    }
    return ret_val;
}

size_t sanitize_crlf(char *buffer, ssize_t buffer_len) {
    /**
     * When:
     * there is a CR: add an LF after
     * there is an LF: add a CR in its spot and an LF later
     *
     */
    size_t actual_buffer_width;
    if (buffer_len < 0) {
        fprintf(stderr, "Error in buffer: buffer length less than zero\n");
        exit(EXIT_FAILURE);
    } else {
        actual_buffer_width = (size_t) buffer_len;
    }
    if (actual_buffer_width == 1) {
        switch (buffer[0]) {
            case CR:
                buffer = realloc(buffer, ++actual_buffer_width);
                print_err();
                buffer[0] = CR; //put an LF
                buffer[1] = LF;
                break;
            case LF:
                buffer = realloc(buffer, ++actual_buffer_width);
                print_err();
                buffer[0] = CR;
                buffer[1] = LF;
                break;
            default:
                break;
        }
        return actual_buffer_width;
    }
    bool previously_added = false;
    for (int i = 0; i < actual_buffer_width; i++) {
        switch (buffer[i]) {
            case CR:
                buffer = realloc(buffer, ++actual_buffer_width);
                print_err();
                for (size_t j = actual_buffer_width; j > i + 1; j--) {
                    char tmp = buffer[j]; //swap until the 0 is at the front
                    buffer[j] = buffer[j - 1];
                    buffer[j - 1] = tmp;
                }
                buffer[i + 1] = LF;
                previously_added = true;
                break;
            case LF:
                //there's no CR here, and we have to put one
                if (!previously_added) { //if this lf was added by a previous iteration,
                    buffer = realloc(buffer, ++actual_buffer_width); //don't add anything
                    print_err();
                    buffer[actual_buffer_width - 1] = 0;
                    for (size_t j = actual_buffer_width; j > i + 1; j--) {
                        char tmp = buffer[j]; //swap until the 0 is at the front
                        buffer[j] = buffer[j - 1];
                        buffer[j - 1] = tmp;
                    }
                    buffer[i] = CR;
                    buffer[i + 1] = LF;
                    previously_added = true;
                } else {
                    previously_added = false;
                }
            default:
                break;

        }
    }
    return actual_buffer_width;

}

void check_shell(){
    char* shell = getenv("SHELL");
    if (errno) {
        fprintf(stderr, "Could not get the shell. This isn't anything to worry about. Error: %s", strerror(errno));
    } else if (!shell) {
        fprintf(stderr, "Could not determine the shell we're running under. This isn't anything to worry about.\n");
    } else if (!(strstr(shell, "/bash") || strstr(shell, "/sh"))) { //if a nullptr is returned, i.e.
        fprintf(stderr, "This program works best with a bourne shell (sh or bash). \n"
                "Using the one you've chosen (%s) may cause issues. \n"
                "I'm not going to quit because you're an adult and can handle the \n"
                "consequences of what you're doing.\n", shell);
    }
}
void set_terminal_modes(struct termios *initial_status){
    if (tcgetattr(FD_KEYBOARD, initial_status)) { //nonzero=error
        fprintf(stderr, "Could not get state of current terminal: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    struct termios *set_status = initial_status;
    set_status->c_iflag = ISTRIP;
    set_status->c_oflag = 0;
    set_status->c_lflag = 0;
    //set the terminal to non-canonical, no echo mode
    //suggested by the spec
    if (tcsetattr(FD_KEYBOARD, TCSANOW, set_status)) {
        fprintf(stderr, "Could not set status of current terminal: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    //read into buffer then read out
}

void echo(){
    bool got_eof = false;
    char *buffer = calloc(1, READ_BUFFER_WIDTH);
    print_err();
    while (!got_eof) {
        ssize_t chars_read = read(FD_KEYBOARD, buffer, READ_BUFFER_WIDTH);
        print_err();
        chars_read = sanitize_crlf(buffer, chars_read);
        if (chars_read > 0) {
            for (int i = 0; i < chars_read; ++i) {
                if (buffer[i] != EF) { //the last thing read will always be eof as far as I can tell
                    ssize_t bytes_written = write(1, buffer + i, 1);
                    if (bytes_written <= 0) {
                        print_err();
                    }
                } else {
                    got_eof = true; //apparently eof is 4 (C-d)
                }
            }
        }
    }
}

void shell(){
    /*
     * 1. Fork to create a new process, then exec a shell whose stdin in a pipe from parent, stdout/err are a pipe to parent
     * 2. read input from keyboard and echo it, as well as forward it to the shell
     * 3. sanitise_crlf when echoing but to shell it should be lf
     * 4. read output from the shell and echo it to stdout, lf should be sanitised to crlf
     */
    //write logs to log.txt
#ifdef DEBUG_PIPES
    FILE* log = fopen("log.txt", "a");
    write_log(log, "Starting program");
#endif
    int to_child[2];
    if (pipe(to_child))
        print_err();
    int from_child[2];
    if (pipe(from_child))
        print_err();
    child_pid = fork();
    print_err();
    if (child_pid){
        signal(SIGPIPE, handler);
        //signal(SIGCHLD, handler);
        //signal()
        //parent
#ifdef DEBUG_PIPES
        write_log(log, "Forked: this is the parent process");
        write_log(log, "Closing unnecessary pipes");
        fprintf(log, "Child child_pid: %u\n", child_pid);
        fflush(log);
#endif
        close(to_child[0]);
        print_err();
        close(from_child[1]);
        print_err();
       char buffer[READ_BUFFER_WIDTH];
        ssize_t bytes_read=0;
        struct pollfd kb_poll;
        kb_poll.fd = FD_KEYBOARD;
        kb_poll.events = POLLIN | POLLERR | POLLHUP;
        struct pollfd child_out_probe;
        child_out_probe.fd = from_child[0];
        child_out_probe.events = POLLIN | POLLERR | POLLHUP;
        struct pollfd polls[2];
        polls[0] = kb_poll;
        polls[1] = child_out_probe;
        while (true) { //no errors
            if (poll(polls, 2, 0)){
                if (polls[0].revents & POLLIN) {
#ifdef DEBUG_PIPES
                    write_log(log, "read from kb");
                    fflush(log);
#endif
                    print_err();
                    bytes_read = read(FD_KEYBOARD, buffer, READ_BUFFER_WIDTH);
                    print_err();
                    if (*buffer == 3){
                        kill(child_pid, SIGINT); //YOU PROCESS ONE THING AT A TIME ANYWAY QUIT PRETENDING WITH THOSE STUPID FOR LOOPS
                        print_err();
                    } else if (*buffer == EF){
                        if (write(to_child[1], "\004", 1)==-1){
                            print_err();
                        }
                        close(to_child[1]);
                        close(from_child[0]);
                        kill(child_pid, SIGKILL);
                        int returns;
                        waitpid(child_pid, &returns, 0);
                        print_err();
                        int exit_stat = WEXITSTATUS(returns);
                        tcsetattr(FD_KEYBOARD, TCSANOW, &initial_status);
                        print_err();
                        fprintf(stderr, "SHELL EXIT SIGNAL=0 STATUS=%i\n", exit_stat);
                        print_err();
                        exit(0);
                    } else if (*buffer == CR || *buffer == LF){
                        if (write(1, "\r\n", 2)==-1)
                            print_err();
                        if (write(to_child[1], "\n", 1)==-1)
                            print_err();
#ifdef DEBUG_PIPES
                        write_log(log, "wrote to child: LF");
#endif
                    } else {
                        if (write(1, buffer, (size_t)bytes_read)==-1)
                            print_err();
                        if (write(to_child[1], buffer, (size_t)bytes_read)==-1)
                            print_err();
#ifdef DEBUG_PIPES
                        fprintf(log, "wrote to child: %c", buffer);
                        fflush(log);
#endif
                    }
                } else if (polls[0].revents & (POLLERR | POLLHUP)){
                    //???when??? will i ever need this?? when will this ever happen???
                    break;
                    //shell exited normally todo: proper shutdown processing
#ifdef DEBUG_PIPES
                    write_log(log, "pollerr or pollhup received from kb");
#endif
                }
                if (polls[1].revents & POLLIN){

                    bytes_read = read(from_child[0], buffer, 1);
                    print_err();
#ifdef DEBUG_PIPES
                    write_log(log, "read from child");
                    fflush(log);
#endif
                    if (*buffer == LF){
                        if (write(1, "\r\n", 2)!=2) {
                            print_err();
#ifdef DEBUG_PIPES
                            write_log(log, "didnt write properly to stdout");
                            fflush(log);
#endif
                        }
                    } else {
#ifdef DEBUG_PIPES
                        write_log(log, "about to write the data we got from the child");
                        fflush(log);
#endif
                        if (write(2, buffer, (size_t) bytes_read)!=bytes_read){
                            print_err();
#ifdef DEBUG_PIPES
                            write_log(log, "didnt write properly to stdout");
                            fflush(log);
#endif
                        }
                        print_err();
                    }
                }

            }
            if (polls[1].revents & POLLHUP) {
                break;
#ifdef DEBUG_PIPES
                write_log(log, "pollhup recvd from child");
#endif
                print_err();
            } else if (polls[1].revents & POLLERR) {
#ifdef DEBUG_PIPES
                write_log(log, "pollerr recvd from child");
#endif
                print_err();
                break;
            }
        }//end of while
    } else {
        //child process
#ifdef DEBUG_PIPES
        write_log(log, "Forked: this is the child process");
        fflush(log);
#endif
        dup2(to_child[0], 0); //fd for stdin
        print_err();
        dup2(from_child[1], 1); //FD for stdout
        print_err();
        dup2(1, 2); //dup stderr to pipe to process
        print_err();
        close(to_child[0]);
        print_err();
        close(from_child[1]);
        print_err(); //I'm so glad the preprocessor is doing this copy-paste for me

#ifdef DEBUG_PIPES
        write_log(log, "set up pipes in child, starting process");
        fflush(log);
#endif
        char* command_execute = "/bin/bash";
        char* arguments[2];
        arguments[0] = command_execute;
        arguments[1] = 0;

        execvp(command_execute, arguments);
        print_err();

    }
#ifdef DEBUG_PIPES
    fclose(log);
#endif
}
#ifndef TESTING_CS111_MARIA
int main(int argc, char** argv){
    //get modes of terminal

    check_shell();
    sh = do_shell(argc, argv);
    if (sh){
        set_terminal_modes(&initial_status);
        shell();
    } else {
        set_terminal_modes(&initial_status);
        echo();
    }
    if (!sh) {
        int returns = 0;
        waitpid(child_pid, &returns, 0);
        print_err();
        int exit_stat = WEXITSTATUS(returns);
        int exit_sig = 0;
        if (WIFSIGNALED(returns)) {
            exit_sig = WTERMSIG(returns);
        }
        fprintf(stderr, "SHELL EXIT SIGNAL=%i STATUS=%i\n", exit_sig, exit_stat);
    }
    //set everything back to how it was
    tcsetattr(FD_KEYBOARD, TCSANOW, &initial_status);
    print_err();
}
#endif



