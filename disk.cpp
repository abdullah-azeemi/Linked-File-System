#define _CRT_SECURE_NO_WARNINGS
#include "filesystem.h"

void createDiskFile(const char* filename) {
    FILE* disk = fopen(filename, "wb");
    if (!disk) {
        perror("Error creating disk file");
        exit(EXIT_FAILURE);
    }

    char garbage = 0xFF;
    for (size_t i = 0; i < DISK_SIZE; i++) {
        fwrite(&garbage, 1, 1, disk);
    }
    fclose(disk);
    printf("Disk file '%s' created successfully.\n", filename);
}

void writeBlock(FILE* disk, int blockNumber, const void* buffer) {
    fseek(disk, blockNumber * BLOCK_SIZE, SEEK_SET);
    fwrite(buffer, BLOCK_SIZE, 1, disk);
}

void readBlock(FILE* disk, int blockNumber, void* buffer) {
    fseek(disk, blockNumber * BLOCK_SIZE, SEEK_SET);
    fread(buffer, BLOCK_SIZE, 1, disk);
}
