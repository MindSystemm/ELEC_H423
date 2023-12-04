#include "Classes.h"

void free_byte_array(Byte_Array* b) {
    free(b->data);
    free(b);
}

void print_byte_array(Byte_Array* b) {
    printf("Byte_Array:\n");

    // Print data
    char* arr = byte_arr_to_str(b->data, b->length);
    printf("\tdata: %s\n", arr);
    printf("\tlength: %d\n", b->length);
    free(arr);
}