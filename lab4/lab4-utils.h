/**
 * NAME: Maria E. V. Neptune
 * EMAIL:jerenept@gmail.com
 * ID:   004-665-056
 */
#ifndef LAB4_LAB4_UTILS_H
#define LAB4_LAB4_UTILS_H
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <mraa.h>
#include <mraa/aio.h>
#include <mraa/gpio.h>
#define print_err() do {if (errno){fprintf(stderr, "Internal Error: %s\n", strerror(errno)); exit(EXIT_FAILURE);}} while (0)
#define FD_STDIN 0
#define MAX_COMMAND_LEN 1024
enum scale {FAHRENHEIT, CELSIUS};
struct behavior {
    unsigned period;
    FILE* log_file;
    enum scale scale;
    bool running, quit;
    struct timespec last_output;
    mraa_aio_context temp_sensor;
    mraa_gpio_context button;
};
struct behavior* argparse(int argc, char** argv);
void usage();
void write_log(struct behavior* behavior);
int read_user_input(char* buffer, int* len);
double read_temp(struct behavior* behavior);
void init_hw(struct behavior* behavior);
void read_button(struct behavior* behavior);
void read_cmd(struct behavior* behavior, char* buffer, int* buf_len);
#endif //LAB4_LAB4_UTILS_H
