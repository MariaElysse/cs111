/**
 * NAME: Maria E. V. Neptune
 * EMAIL:jerenept@gmail.com
 * ID:   004-665-056
 */

#include "lab4-utils.h"
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <mraa.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>

#define temp_constant_multiple 100000.0 //R0, in the formula given
#define thermistor_constant 4275 //B in the formula given
#define min(x,y) (x)<(y)? x : y
void usage(){
    fprintf(stderr, "Usage: \n"
            "--period=PERIOD\tSet the time in integer seconds per logged temperature reading\n"
            "--scale=[F/C/K]\tSet the temperature scale to be Fahrenheit, Celsius or Kelvin\n"
            "--log=LOGFILE\tIndicate a file to be used for logging (data will always be output to stdout)\n");
    exit(EXIT_FAILURE);
}
struct behavior* argparse(int argc, char** argv){
    struct behavior* ret = malloc(sizeof(struct behavior));
    ret->period = 1;
    ret->running = true;
    ret->quit = false;
    ret->scale = FAHRENHEIT;
    ret->log_file = NULL;
    int getopt_status = 0, index = 0;
    struct option options[] = {
            {"period", required_argument, NULL, 'p'},
            {"scale", required_argument, NULL, 's'},
            {"log", required_argument, NULL, 'l' },
    };
    while ((getopt_status = getopt_long(argc, argv, ":p:s:l", options, &index)) != -1){
        switch (getopt_status){
            case -1:
            case 0:
                break;
            case 'p':
                if(sscanf(optarg, "%d", &ret->period) != 1){
                    fprintf(stderr, "Provided argument \"%s\" is not a valid period\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case 's':
                switch (optarg[0]){
                    case 'c':
                    case 'C':
                        ret->scale = CELSIUS;
                        break;
                    case 'f':
                    case 'F':
                        ret->scale = FAHRENHEIT;
                        break;
                    case 'k':
                    case 'K':
                        fprintf(stderr, "I'll implement Kelvin Soon!\n");
                        exit(EXIT_FAILURE);
                    default:
                        fprintf(stderr, "Unrecognised temperature scale: %s\n", optarg);
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                if (!(ret->log_file = fopen(optarg, "a"))){
                    fprintf(stderr, "Invalid logfile: %s\n", optarg);
                    print_err();
                }
                break;
            case '?':
                fprintf(stderr, "Unrecognised option: %s\n",argv[index +1]);
                usage();
                break;
            case ':':
                fprintf(stderr, "Unrecognised argument to option %s: %s\n", argv[index+1], optarg);
                usage();
                break;
            default:
                fprintf(stderr, "Very wrong arguments/options given (getopt status: %i)\n", getopt_status);
                usage();
        }
    }
    return ret;
}

void init_hw(struct behavior* behavior){
    mraa_init();
    behavior->temp_sensor = mraa_aio_init(0);
    behavior->button = mraa_gpio_init(3);
    if (mraa_gpio_dir(behavior->button, MRAA_GPIO_IN) != MRAA_SUCCESS){
        fprintf(stderr, "Couldn't set GPIO direction\n");
        exit(EXIT_FAILURE);
    }
}

double read_temp(struct behavior* behavior){
    int raw_read;
    if ((raw_read = mraa_aio_read(behavior->temp_sensor)) == -1){
        fprintf(stderr, "Failed to read Analogue pin\n");
        exit(EXIT_FAILURE);
    }
    double temp = 1023.0/raw_read-1.0;
    temp *=temp_constant_multiple;
    temp = 1.0/(log(temp/temp_constant_multiple)/thermistor_constant+1/298.15)-273.15;
    if (behavior->scale == FAHRENHEIT){
        return (9.0/5)*temp + 32.0;
    } else {
        return temp;
    }
}
/*
 * return:
 *      -1: stdin closed
 *       0: something read correctly
 *       1: \n detected (
 */
int read_user_input(char* buffer, int* len){
    struct pollfd pollfd;
    pollfd.fd = FD_STDIN;
    pollfd.events = POLLIN | POLLHUP | POLLERR;
    if (*len == 1024){
        fprintf(stderr, "Commands over 1024 characters aren't supported (the rest will be truncated)\n");
        buffer[1024] = 0;
        return 1;
    }
    char temp_char;
    if (poll(&pollfd, 1, 0)){
        if (pollfd.revents & (POLLERR | POLLHUP)){
            return -1;
        }
        *len = *len + 1;
        if (read(FD_STDIN, &temp_char, 1) == -1){
            print_err();
            return -1;
        }
        if (temp_char == '\n'){
            buffer[*len-1]=0;
            return 1;
        } else {
            buffer[*len-1]=temp_char;
            return 0;
        }
    } else { //no poll
        return 0;
    }
}

void write_log(struct behavior* behavior){
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    //print_err();
    if ((now.tv_sec - behavior->last_output.tv_sec) < behavior->period){
        return;
    }
    time_t t = time(NULL);
    struct tm* time_now;
    time_now = localtime(&t);
    if (behavior->quit){
        if (behavior->log_file){
            fprintf(behavior->log_file, "%02d:%02d:%02d SHUTDOWN",time_now->tm_hour, time_now->tm_min, time_now->tm_sec);
            return;
        }
    }
    if (!behavior->running){
        return;
    }
    if (behavior->log_file){
        fprintf(behavior->log_file, "%02d:%02d:%02d %.1f\n", time_now->tm_hour, time_now->tm_min, time_now->tm_sec, read_temp(behavior));
    }
    fprintf(stdout, "%02d:%02d:%02d %.1f\n", time_now->tm_hour, time_now->tm_min, time_now->tm_sec, read_temp(behavior));
    behavior->last_output = now;
}

void read_button(struct behavior* behavior){
    int raw_read = 0;
    if ((raw_read = mraa_gpio_read(behavior->button)) == -1){
        fprintf(stderr, "Failed to read Digital pin\n");
        exit(EXIT_FAILURE);
    }
    if (raw_read == 1){
        behavior->quit = true;
    }
}
void read_cmd(struct behavior* behavior, char* buffer, int* buf_len){
    int arg;
    if (behavior->log_file){
        fprintf(behavior->log_file, "%s\n", buffer);
    }
    if (*buf_len == 4 && !memcmp(buffer, "OFF", 3)){
        behavior->quit = true;
        behavior->running = false;
    } else if (*buf_len == 5 && !memcmp(buffer, "STOP", 4)){
        if (!behavior->running) {
            fprintf(stderr, "Already stopped\n");
        }
        behavior->running = false;
    } else if (*buf_len > 5 && !memcmp(buffer, "START",5)){
        if (behavior->running){
            fprintf(stderr, "Already running\n");
        }
        behavior->running = true;
    } else if (*buf_len == 8 && !memcmp(buffer, "SCALE=F", 7)){
        if (behavior->scale == FAHRENHEIT){
            fprintf(stderr, "Already using Fahrenheit\n");
        }
        behavior->scale = FAHRENHEIT;
    } else if (*buf_len == 8 && !memcmp(buffer, "SCALE=C", 7)){
        if (behavior->scale == CELSIUS) {
            fprintf(stderr, "Already using Celsius");
        }
        behavior->scale = CELSIUS;
    } else if (*buf_len > 7 && !memcmp(buffer, "PERIOD=", 7)){
        if (sscanf(buffer, "PERIOD=%i\n", &arg) == 1){
            behavior->period = (unsigned) arg;
        } else {
            fprintf(stdout, "Invalid command\n");
        }
        print_err();
    }
    *buf_len = 0;
}