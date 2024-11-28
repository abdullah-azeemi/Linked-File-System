#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<openssl/aes.h>


#define DISK_SIZE (64 * 1024 * 1024) // 64 MB
#define BLOCK_SIZE 1024             // 1 KB
#define DIRECTORY_SIZE 128
#define MAX_FILE_NAME 64
#define FAT_SIZE (DISK_SIZE / BLOCK_SIZE) // 65536 entries
#define MAX_FILE_SIZE (128 * BLOCK_SIZE) // 128 blocks = 128 KB
#define MAX_KEY_SIZE 64
#define MAX_FILENAME_LENGTH 64

struct DirectoryEntry {
    char name[MAX_FILE_NAME];
    int startBlock;    
    int size;           
    bool isDirectory;  
    int parentDirIndex; 
};



// Global variables
extern int FAT[FAT_SIZE];
extern DirectoryEntry directory[DIRECTORY_SIZE];

// Function prototypes
void createDiskFile(const char* filename);
void formatDisk(const char* diskName);
void writeBlock(FILE* disk, int blockNum, const void* data);
void readBlock(FILE* disk, int blockNum, void* buffer);

int createFile(const char* name, int size);
void deleteFile(const char* name);
void consoleInterface(const char* diskName);
void exitProgram(FILE* disk);
void loadFileSystemState(const char* diskName);
int deleteFileOrDirectory(const char* name);

#endif // FILESYSTEM_H
