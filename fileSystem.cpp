#define _CRT_SECURE_NO_WARNINGS
#include "filesystem.h"
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>

using namespace std;

int FAT[FAT_SIZE];
DirectoryEntry directory[DIRECTORY_SIZE];

void formatDisk(const char* diskName) {
    FILE* disk = fopen(diskName, "r+b");
    if (!disk) {
        perror("Error opening disk file");
        exit(EXIT_FAILURE);
    }

    memset(FAT, -1, sizeof(FAT));
    memset(directory, 0, sizeof(directory));
    for (int i = 0; i < DIRECTORY_SIZE; i++) {
        directory[i].startBlock = -1;
    }

    writeBlock(disk, 0, FAT);
    writeBlock(disk, 1, directory);

    fclose(disk);
    cout << "Disk formatted successfully.\n";
}

void createDirectory(const char* name) {
    int dirIndex = -1;
    for (int i = 0; i < DIRECTORY_SIZE; i++) {
        if (directory[i].startBlock == -1) {
            dirIndex = i;
            break;
        }
    }
    if (dirIndex == -1) {
        cout << "Error: Directory is full.\n";
        return;
    }

    strcpy(directory[dirIndex].name, name);
    directory[dirIndex].startBlock = -1; // No blocks needed for directory
    directory[dirIndex].size = 0;
    directory[dirIndex].isDirectory = 1;

    cout << "Directory '" << name << "' created successfully.\n";
}

void saveFileSystemState(FILE* disk) {
    writeBlock(disk, 0, FAT);         // Save FAT to block 0
    writeBlock(disk, 1, directory);  // Save Directory to block 1
}

void loadFileSystemState(FILE* disk) {
    readBlock(disk, 0, FAT);         // Load FAT from block 0
    readBlock(disk, 1, directory);   // Load Directory from block 1
}

void deleteDirectory(const char* name) {
    for (int i = 0; i < DIRECTORY_SIZE; i++) {
        if (strcmp(directory[i].name, name) == 0 && directory[i].isDirectory) {
            directory[i].startBlock = -1;
            memset(directory[i].name, 0, sizeof(directory[i].name));
            cout << "Directory '" << name << "' deleted successfully.\n";
            return;
        }
    }
    cout << "Error: Directory not found.\n";
}

int deleteFileOrDirectory(const char* name) {
    for (int i = 0; i < DIRECTORY_SIZE; i++) {
        if (strcmp(directory[i].name, name) == 0) {
            if (directory[i].isDirectory) {
                cout << "Directory '" << name << "' deleted successfully.\n";
            }
            else {
                int block = directory[i].startBlock;
                while (block != -1) {
                    int nextBlock = FAT[block];
                    FAT[block] = -1;
                    block = nextBlock;
                }
                cout << "File '" << name << "' deleted successfully.\n";
            }
            directory[i].startBlock = -1;
            return 0;
        }
    }
    cout << "Error: File or directory not found.\n";
    return -1;
}

void createPartition(const char* diskName) {
    createDiskFile(diskName);
    formatDisk(diskName);
    cout << "Partition '" << diskName << "' created and formatted.\n";
}

int createFile(const char* name, int size) {
    if (size > MAX_FILE_SIZE) {
        cout << "Error: File size exceeds the maximum limit.\n";
        return -1;
    }

    int dirIndex = -1;
    for (int i = 0; i < DIRECTORY_SIZE; i++) {
        if (directory[i].startBlock == -1) {
            dirIndex = i;
            break;
        }
    }
    if (dirIndex == -1) {
        cout << "Error: Directory is full.\n";
        return -1;
    }

    int blocksRequired = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int prevBlock = -1, startBlock = -1;
    for (int i = 0; i < FAT_SIZE && blocksRequired > 0; i++) {
        if (FAT[i] == -1) {
            if (startBlock == -1) startBlock = i;
            if (prevBlock != -1) FAT[prevBlock] = i;
            prevBlock = i;
            blocksRequired--;
        }
    }
    if (blocksRequired > 0) {
        cout << "Error: Not enough disk space.\n";
        return -1;
    }
    FAT[prevBlock] = -1;

    strcpy(directory[dirIndex].name, name);
    directory[dirIndex].startBlock = startBlock;
    directory[dirIndex].size = size;
    directory[dirIndex].isDirectory = 0;

    cout << "File '" << name << "' created successfully.\n";
    return 0;
}

void listFilesAndDirectories() {
    cout << "Files and Directories:\n";
    for (int i = 0; i < DIRECTORY_SIZE; i++) {
        if (directory[i].startBlock != -1 && directory[i].size != 0) {
            cout << directory[i].name << " ("
                << (directory[i].isDirectory ? "Directory" : "File")
                << ", " << directory[i].size << " bytes)\n";
        }
    }
}

int writeFile(const char* name, const char* data, int dataSize) {
    for (int i = 0; i < DIRECTORY_SIZE; i++) {
        if (strcmp(directory[i].name, name) == 0) {
            if (dataSize > directory[i].size) {
                cout << "Error: Data size exceeds file size.\n";
                return -1;
            }

            int block = directory[i].startBlock;
            FILE* disk = fopen("disk.bin", "r+b");
            int offset = 0;

            while (block != -1 && offset < dataSize) {
                writeBlock(disk, block, data + offset);
                offset += BLOCK_SIZE;
                block = FAT[block];
            }

            fclose(disk);
            cout << "Data written to file '" << name << "' successfully.\n";
            return 0;
        }
    }
    cout << "Error: File not found.\n";
    return -1;
}




int readFile(const char* name) {
    for (int i = 0; i < DIRECTORY_SIZE; i++) {
        if (strcmp(directory[i].name, name) == 0) {
            char buffer[BLOCK_SIZE];
            int block = directory[i].startBlock;
            FILE* disk = fopen("disk.bin", "rb");

            printf("Reading file '%s':\n", name);
            while (block != -1) {
                readBlock(disk, block, buffer);
                printf("%.*s", BLOCK_SIZE, buffer); 
                block = FAT[block];
            }

            fclose(disk);
            printf("\n");
            return 0;
        }
    }
    printf("Error: File not found.\n");
    return -1;
}
int truncateFile(const char* name, int newSize) {
    for (int i = 0; i < DIRECTORY_SIZE; i++) {
        if (strcmp(directory[i].name, name) == 0) {
            if (newSize > directory[i].size) {
                printf("Error: New size exceeds original file size.\n");
                return -1;
            }

            int blocksToKeep = (newSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
            int block = directory[i].startBlock;

            while (block != -1 && blocksToKeep > 1) {
                block = FAT[block];
                blocksToKeep--;
            }

            // Free remaining blocks
            int toFree = FAT[block];
            FAT[block] = -1;
            while (toFree != -1) {
                int next = FAT[toFree];
                FAT[toFree] = -1;
                toFree = next;
            }

            directory[i].size = newSize;
            printf("File '%s' truncated to %d bytes.\n", name, newSize);
            return 0;
        }
    }
    printf("Error: File not found.\n");
    return -1;
}



int deleteFile(const char* name) {
    for (int i = 0; i < DIRECTORY_SIZE; i++) {
        if (strcmp(directory[i].name, name) == 0) {
            // Free blocks in FAT
            int block = directory[i].startBlock;
            while (block != -1) {
                int nextBlock = FAT[block];
                FAT[block] = -1;
                block = nextBlock;
            }

            // Remove directory entry
            directory[i].startBlock = -1;
            printf("File '%s' deleted successfully.\n", name);
            return 0;
        }
    }
    printf("Error: File not found.\n");
    return -1;
}
void consoleInterface(const char* diskName) {
    FILE* disk = fopen(diskName, "r+b"); 
    if (!disk) {
        perror("Error opening disk file");
        exit(EXIT_FAILURE);
    }
    while (1) {
        printf("File System Menu:\n");
        printf("1. Create File\n");
        printf("2. Delete File\n");
        printf("3. Write to File\n");
        printf("4. Read File\n");
        printf("5. Truncate File\n");
        printf("6. List Files\n");
        printf("7. Format Disk\n");
        printf("8. Create Directory\n");
        printf("9. Encrypt Partition\n");
        printf("10. Decrypt Partition\n");
        printf("11. Exit\n");
        printf("Enter your choice: ");

        int choice;
        scanf("%d", &choice);

        switch (choice) {
        case 1: {
            char name[MAX_FILE_NAME];
            int size;
            printf("Enter file name: ");
            cin >> name;
            printf("Enter file size (bytes): ");
            cin >> size;
            createFile(name, size);
            break;
        }
        case 2: {
            char name[MAX_FILE_NAME];
            printf("Enter file name to delete: ");
            cin >> name;
            deleteFile(name);
            break;
        }
        case 3: {
            char name[MAX_FILE_NAME];
            printf("Enter file name to write to: ");
            cin >> name;
            char data[MAX_FILE_SIZE];
            printf("Enter data to write: ");
            cin.ignore();
            cin.getline(data, MAX_FILE_SIZE);
            writeFile(name, data, strlen(data));
            break;
        }
        case 4: {
            char name[MAX_FILE_NAME];
            printf("Enter file name to read: ");
            cin >> name;
            readFile(name);
            break;
        }
        case 5: {
            char name[MAX_FILE_NAME];
            int newSize;
            printf("Enter file name to truncate: ");
            cin >> name;
            printf("Enter new size (bytes): ");
            cin >> newSize;
            truncateFile(name, newSize);
            break;
        }
        case 6:
            listFilesAndDirectories();
            break;
        case 7:
            formatDisk(diskName);
            break;
        case 11:
            exitProgram(disk);
            return;
        default:
            printf("Invalid choice.\n");
        }
    }
}
void exitProgram(FILE* disk) {
    // Save the state before exiting
    if (disk != NULL) {
        saveFileSystemState(disk);
        fclose(disk);  // Close the disk file
    }
    exit(0);  // Exit the program
}

void loadFileSystemState(const char* diskName) {
    FILE* disk = fopen(diskName, "rb");
    if (!disk) {
        perror("Error opening disk file");
        exit(EXIT_FAILURE);
    }

    // Read FAT and Directory Table from disk
    readBlock(disk, 0, FAT);
    readBlock(disk, 1, directory);

    fclose(disk);
    printf("File system state loaded successfully.\n");
}




