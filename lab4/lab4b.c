/**
 * NAME: Maria E. V. Neptune
 * EMAIL:jerenept@gmail.com
 * ID:   004-665-056
 */
#include <stdlib.h>
#include "lab4-utils.h"

int main(int argc, char** argv){
    struct behavior* behavior = argparse(argc, argv);
    char* buffer = malloc(MAX_COMMAND_LEN*sizeof(char));
    int buf_len = 0;
    init_hw(behavior);
    while (!behavior->quit){
        int rc = read_user_input(buffer, &buf_len);
        if (rc == -1){
            behavior->quit = true;
        } else if (rc == 1){
            read_cmd(behavior, buffer, &buf_len);
        } //do nothing if rc = 0, we still need to get more things from the user
        read_button(behavior);
        write_log(behavior);
    }
    if (behavior->log_file){
        fclose(behavior->log_file);
    }
    return 0;
}