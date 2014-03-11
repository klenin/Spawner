#include <stdio.h>

int block_count = 10;
int block_size = 1024*1024;


int main(int argc, char *argv[]) {
    if (argc > 2) {
        sscanf(argv[1], "%d", &block_count);
        sscanf(argv[2], "%d", &block_size);
    }
    for (int i = 0; i < block_count; ++i) {
        char *array = new char[block_size];
    }
}