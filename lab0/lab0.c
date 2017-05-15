/**
 *  NAME:   Maria E.V. Neptune
 *  EMAIL:  jerenept@gmail.com
 *  ID:     004-665-056
 *  Copyright: GNU GPL v3 (http://gnu.org/licenses)
*/
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
void usage() {
    fprintf(stderr, "Usage: \n\
            --input=FILE\tUse an already existing file as input \n\
            --output=FILENAME\tCreate a new file and use it as output \n\
            --segfault\t\tPurposely cause a segmentation fault \n\
            --catch\t\tPurposely cause a segmentation fault, but also catch it and return normally\n");
}

void segfault(){
    int* i; //dereferences a null int pointer
    i=0; 
    *i=5; 
}
void handler(int sig){
    if (sig==SIGSEGV){
        fprintf(stderr, "Error: Segmentation Fault registered\n");
        exit(4);
    }
    //dup(1)
}
void catch() {
    signal(SIGSEGV, handler);
}
int main(int argc, char* argv[]){
    //TODO: parse options
    //TODO: angery react
    FILE *in = stdin;
    FILE *out= stdout;
    struct option options[] = {
        {"input", required_argument, NULL, 'i'},
        {"output", required_argument, NULL, 'o'},
        {"segfault", no_argument, NULL, 's'},
        {"catch", no_argument, NULL, 'c'},
        { 0, 0, 0, 0}, //cc-by-sa 
    };
    int do_segfault=0; //executing the segfault = 1b, catching=10b
    int getopt_status=0;
    int index=0;
    while((getopt_status = getopt_long(argc, argv, ":i:o:sc", options, &index))!=-1) { //==-1 when parsing is done
        switch (getopt_status) {
            case -1:
            case 0:
                break;
                
            case 's':
                do_segfault|=1;
                break;
            case 'c':
                do_segfault|=2;
                break;
            case 'i':
                if (do_segfault & 1)
                    break;
                in = fopen(optarg, "r"); //before you penalize me for using the wrong function
                if (!strcmp(optarg, "")){
                    fprintf(stderr, "The argument --input requires a value\n");
                    exit(2);
                }
                if (errno){ //it does the same thing, so technically I'm still following the needed
                    fprintf(stderr, "Error opening input file \"%s\" : %s\n", optarg, strerror(errno)); //interface
                    exit(2); 
                }
                break;
            case 'o':
                if (do_segfault & 1) //if the directive to segfault is given ONLY, catching one shouldn't matter
                    break;
                if (!strcmp(optarg, "")){
                    fprintf(stderr, "The argument --output requires a value\n");
                    exit(3);
                }
                out = fopen(optarg, "w"); //when the file already exists 
                if (errno){
                    fprintf(stderr, "Error opening output file \"%s\": %s\n", optarg, strerror(errno));
                    exit(3);
                }
                break;
            case '?':
                fprintf(stderr, "Unrecognised option: \"%s\"\n", argv[index+1]);
                usage();
                exit(1);
                break;
            case ':':
                fprintf(stderr, "Malformed argument to option \"%s\": \"%s\"\n", argv[index+1], optarg);
                usage();
                exit(1);
                break;
            default:
                fprintf(stderr, "Very wrong options given (getopt status code: %i)\n", getopt_status);
                usage();
                exit(1);
        }
    }
    switch (do_segfault) {
        case 0:
        default:
            break;
        case 1:
            segfault();
            break;
        case 2:
            catch();
            break;
        case 3:
            catch();
            segfault();
            break;
    }
   char to_send;
    while (fread(&to_send,sizeof(to_send), 1, in)!=0){ 
        if (errno){
            exit(2); //if the read fails in any way (eof||errno)
        }
        if (!fwrite(&to_send, sizeof(to_send), 1, out) || errno){
            exit(3); //if the write fails in any way (0 bytes written||errno)
        }
    }
    fclose(in);
    fclose(out);
    exit(0);
}

