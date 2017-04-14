#include "test.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define test_arr_size 16
#define CR 0xd
#define LF 0xa

void fill_array(char *arr, size_t arrlen) {
    for (size_t i = 0; i < arrlen; i++) {
        arr[i] = 'a';
    }
}

bool test_sanitizer() {
    fprintf(stderr, "CR at end of buffer: ");
    char *buffer = calloc(1, test_arr_size);
    fill_array(buffer, test_arr_size);
    buffer[test_arr_size - 1] = CR;
    size_t result_buffer_len;
    result_buffer_len = sanitize_crlf(buffer, test_arr_size);
    if (result_buffer_len == test_arr_size || buffer[result_buffer_len - 1] != LF ||
        buffer[result_buffer_len - 2] != CR) {
        return false;
    }
    fprintf(stderr, "passed\n");

    fprintf(stderr, "CR at middle of buffer: ");
    free(buffer);
    buffer = calloc(1, test_arr_size);
    fill_array(buffer, test_arr_size);
    buffer[5] = CR;

    result_buffer_len = sanitize_crlf(buffer, test_arr_size);

    if (buffer[5] != CR || buffer[6] != LF || result_buffer_len != test_arr_size + 1) {
        return false;
    }
    fprintf(stderr, "passed\n");

    fprintf(stderr, "LF at start of buffer: ");
    free(buffer);
    buffer = calloc(1, test_arr_size);
    fill_array(buffer, test_arr_size);
    buffer[0] = LF;
    result_buffer_len = sanitize_crlf(buffer, test_arr_size);
    if (result_buffer_len != test_arr_size + 1 || buffer[0] != CR || buffer[1] != LF) {
        return false;
    }
    fprintf(stderr, "passed\n");

    fprintf(stderr, "CR at start of buffer, no LF: ");
    free(buffer);
    buffer = calloc(1, test_arr_size);
    fill_array(buffer, test_arr_size);
    buffer[0] = CR;
    result_buffer_len = sanitize_crlf(buffer, test_arr_size);
    if (buffer[0] != CR || buffer[1] != LF || result_buffer_len != test_arr_size + 1) {
        return false;
    }
    fprintf(stderr, "passed\n");

    fprintf(stderr, "LF at middle of buffer, no CR before it: ");
    free(buffer);
    buffer = calloc(1, test_arr_size);
    fill_array(buffer, test_arr_size);
    buffer[5] = LF;
    result_buffer_len = sanitize_crlf(buffer, test_arr_size);
    if (buffer[5] != CR || buffer[6] != LF || result_buffer_len != test_arr_size + 1) {
        return false;
    }
    fprintf(stderr, "passed\n");

    fprintf(stderr, "CRLF at middle of buffer: ");
    free(buffer);
    buffer = calloc(1, test_arr_size);
    fill_array(buffer, test_arr_size);
    buffer[6] = CR;
    buffer[7] = LF;

    result_buffer_len = sanitize_crlf(buffer, test_arr_size);
    if (buffer[6] != CR || buffer[7] != LF || buffer[8] != CR || buffer[9] != LF) {
        return false;
    }
    fprintf(stderr, "passed\n");
    free(buffer);
    fprintf(stderr, "All CR/LF fixing Tests passed\n");
    return true;
}


int main() {
    if (test_sanitizer())
        exit(EXIT_SUCCESS);
    else
        exit(EXIT_FAILURE);
}
