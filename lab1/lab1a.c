#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#define FD_KEYBOARD 0
#define READ_BUFFER_WIDTH 1024 //as recommended by TA
#define CR 0xd
#define LF 0xa
#define EF 0x4

#define print_err() do {if (errno){ fprintf(stderr, "Internal Error: %s\n", strerror(errno)); exit(EXIT_FAILURE); }} while(0)
//what a glorious macro, fuck having good error descriptions
//prints an error and quits if an errno is present
void usage(){
    fprintf(stderr, "Usage: \n\
            --");
}

bool do_shell(int argc, char **argv) {
    int getopt_status = 0;
    int index;
    struct option options[] = {
            {"shell", no_argument, NULL, 's'},
            {0, 0, 0,                    0}
    };
    bool ret_val = false;
    while ((getopt_status = getopt_long(argc, argv, ":s", options, &index) != -1)) {
        switch (getopt_status) {
            case -1:
            case 0:
                break;
            case 's':
                ret_val = true;
                break;
            case '?':
                fprintf(stderr, "Unrecognised option: %s\n", argv[index + 1]);
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

#ifndef TESTING
int main(int argc, char** argv){
    //get modes of terminal

    struct termios initial_status;
    char* shell = getenv("SHELL");
    if (errno) {
        fprintf(stderr, "Could not get the shell. This isn't anything to worry about: %s", strerror(errno));
    } else if (!shell) {
        fprintf(stderr, "Could not determine the shell we're running under. This isn't anything to worry about.\n");
    } else if (!(strstr(shell, "/bash") || strstr(shell, "/sh"))) { //if a nullptr is returned, i.e.
        fprintf(stderr, "This program works best with a bourne shell (sh or bash). \n"
                "Using the one you've chosen (%s) may cause issues. \n"
                "I'm not going to quit because you're an adult and can handle the \n"
                "consequences of what you're doing.\n", shell);
    }
    if (tcgetattr(FD_KEYBOARD, &initial_status)) { //nonzero=error
        fprintf(stderr, "Could not get state of current terminal: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    struct termios set_status = initial_status;
    set_status.c_iflag = ISTRIP;
    set_status.c_oflag = 0;
    set_status.c_lflag = 0;
    //set the terminal to non-canonical, no echo mode
    //suggested by the spec
    if (tcsetattr(FD_KEYBOARD, TCSANOW, &set_status)) {
        fprintf(stderr, "Could not set status of current terminal: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    //read into buffer then read out
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
    //set everything back to how it was
    tcsetattr(FD_KEYBOARD, TCSANOW, &initial_status);
    print_err();
}
#endif



