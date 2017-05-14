/**
 * Library that holds some common functions and data structures for both client and server
 */
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lab1b.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void usage();

struct behavior *argparse(int argc, char **argv) {
    struct stat enckey_stat;
    int getopt_status = 0, index = 0, enckey_file;
    struct behavior *ret = calloc(1, sizeof(struct behavior));
    struct option options[] = {
            {"port", required_argument, NULL, 'p'},
            {"encrypt", required_argument, NULL, 'e'},
#ifndef SERVER
            {"log", required_argument, NULL, 'l'},
#endif
            {0, 0, 0, 0},
    };
#ifdef SERVER
    char* shortopts = ":p:e";
#else
    char *shortopts = ":p:e:l";
#endif
    while ((getopt_status = getopt_long(argc, argv, shortopts, options, &index)) !=
           -1) { //todo: verify that this is not always -1, dont be stupid
        switch (getopt_status) {
            case -1:
            case 0:
                break;
            case 'p':
                if (sscanf(optarg, "%hu", &(ret->port)) != 1) {
                    fprintf(stderr, "%s isn't a valid port number!\n", optarg);
                    usage();
                }
                fprintf(stderr, "%s is the port \n", optarg);
                break;
            case 'e':
                enckey_file = open(optarg, O_RDONLY);
                print_err();
                fstat(enckey_file, &enckey_stat);
                print_err();
                ret->key_len = (int) enckey_stat.st_size;
                if (enckey_stat.st_size > 0) {
                    ret->encryption_key = malloc((unsigned long long) enckey_stat.st_size); //this is positive
                    print_err();
                    if (read(enckey_file, ret->encryption_key, (unsigned long long) enckey_stat.st_size) !=
                        enckey_stat.st_size) {
                        print_err();
                        fprintf(stderr, "Could not read encryption key\n");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    fprintf(stderr, "File %s can't be used as an encryption key (it has size %li)\n", optarg,
                            enckey_stat.st_size);
                    exit(1);
                }
                break;
                //this would never happen anyway in the server, but removing dead code is alright
#ifndef SERVER
            case 'l':
                if (!strcmp(optarg, "")) {
                    fprintf(stderr, "Option --log requires an argument\n");
                    usage();
                }
                ret->logfile = fopen(optarg, "a");
                if (errno || (!ret->logfile)) {
                    fprintf(stderr, "Error opening logfile %s: %s\n", optarg, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                break;
#endif
            case '?':
                fprintf(stderr, "Unrecognized option: %s\n", argv[index + 1]);
                usage();
                break;
            case ':':
                fprintf(stderr, "Unrecognized argument to option %s: %s\n", argv[index + 1], optarg);
                usage();
                break;
            default:
                fprintf(stderr, "Very wrong arguments and options given (getopt status code: %i)\n", getopt_status);
                usage();
        }
    }
    if (!ret->port) {
        usage();
    }
    return ret;
}

void move_conn(struct connection list[], size_t index, size_t len) {
    for (size_t i = index; i < (len) - 1; ++i) {
        list[i + 1] = list[i];
    }
}

struct encryption_descriptors *init_encryption(struct behavior *behavior) {
    struct encryption_descriptors *ret = malloc(sizeof(struct encryption_descriptors));
    ret->encryption = mcrypt_module_open("twofish", NULL, MCRYPT_CFB, NULL);
    ret->decryption = mcrypt_module_open("twofish", NULL, MCRYPT_CFB, NULL);
    if (ret->decryption == MCRYPT_FAILED || ret->encryption == MCRYPT_FAILED) {
        fprintf(stderr, "Failed to set up encryption scheme\n");
        exit(EXIT_FAILURE);
    }
    if (
            (mcrypt_generic_init(ret->decryption, behavior->encryption_key, mcrypt_enc_get_key_size(ret->decryption),
                                 INIT_VEC) < 0)
            || mcrypt_generic_init(ret->encryption, behavior->encryption_key, mcrypt_enc_get_key_size(ret->encryption),
                                   INIT_VEC) < 0) {
        fprintf(stderr, "Failed to set up encryption scheme\n");
        exit(EXIT_FAILURE);
    }
    return ret;
}

void encrypt(struct encryption_descriptors *descs, void *buffer, int buffer_len) {
    if (!descs) {
        return;
    }
    if (mcrypt_generic(descs->encryption, buffer, buffer_len)) {
        fprintf(stderr, "Encryption failed\n");
        exit(EXIT_FAILURE);
    }
}

void decrypt(struct encryption_descriptors *descs, void *buffer, int buffer_len) {
    if (!descs) {
        return;
    }
    if (mdecrypt_generic(descs->decryption, buffer, buffer_len)) {
        fprintf(stderr, "Decryption failed\n");
        exit(EXIT_FAILURE);
    }
}

void end_encryption(struct encryption_descriptors descs) {
    mcrypt_generic_deinit(descs.encryption);
    mcrypt_generic_deinit(descs.decryption);
}