#include "filesystem.h"


void printFAT() {
    printf("FAT Table:\n");
    for (int i = 0; i < FAT_SIZE; i++) {
        if (FAT[i] != -1) {
            printf("Block %d -> %d\n", i, FAT[i]);
        }
    }
}

void printDirectory() {
    printf("Directory Entries:\n");
    for (int i = 0; i < DIRECTORY_SIZE; i++) {
        if (directory[i].startBlock != -1) {
            printf("Name: %s, Start Block: %d, Size: %d\n",
                directory[i].name, directory[i].startBlock, directory[i].size);
        }
    }
}
