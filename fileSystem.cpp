#define _CRT_SECURE_NO_WARNINGS
#include "filesystem.h"
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include<string>

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
void createDirectory(const char* name, int parentDirIndex = -1) {
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
    directory[dirIndex].startBlock = dirIndex;  
    directory[dirIndex].size = 0; 
    directory[dirIndex].isDirectory = true;
    directory[dirIndex].parentDirIndex = parentDirIndex;

    if (parentDirIndex != -1) {
        directory[parentDirIndex].size++;
    }

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

    cout << "Blocks required for file: " << blocksRequired << endl;  // Debug print

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
    directory[dirIndex].isDirectory = 0; // Mark as file

    cout << "File '" << name << "' created in entry " << dirIndex
        << " with start block " << startBlock << endl;  // Detailed debug

    return 0;
}
void listFilesAndDirectories() {
    cout << "Files and Directories:\n";
    for (int i = 0; i < DIRECTORY_SIZE; i++) {
        if (directory[i].startBlock != -1 && directory[i].name != "") {
            if (directory[i].isDirectory) {
                cout << " <DIR> " << directory[i].name << ", size : "<< directory[i].size << ", StartBlock: " << directory[i].startBlock;
                cout << endl;
            }
            else if (directory[i].size != 0 && directory[i].isDirectory == 0) {
                cout << " <FILE> " << directory[i].name << ", size : " << directory[i].size << ", StartBlock: " << directory[i].startBlock;
                cout << endl;
            }
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
void encryptPartition(const char* diskName, const char* key) {
    FILE* disk = fopen(diskName, "r+b");
    if (!disk) {
        perror("Error opening disk file");
        return;
    }

    char buffer[BLOCK_SIZE];
    for (int i = 0; i < FAT_SIZE; i++) {
        fseek(disk, i * BLOCK_SIZE, SEEK_SET);
        fread(buffer, 1, BLOCK_SIZE, disk);
        for (int j = 0; j < BLOCK_SIZE; j++) {
            buffer[j] ^= key[j % strlen(key)];
        }
        fseek(disk, i * BLOCK_SIZE, SEEK_SET);
        fwrite(buffer, 1, BLOCK_SIZE, disk);
    }
    fclose(disk);
    printf("Partition encrypted successfully.\n");
}

void decryptPartition(const char* diskName, const char* key) {
    encryptPartition(diskName, key);
    printf("Partition decrypted successfully.\n");
}
void deleteFile(const char* name) {
    for (int i = 0; i < DIRECTORY_SIZE; i++) {
        if (strcmp(directory[i].name, name) == 0) {
       
            int block = directory[i].startBlock;
            while (block != -1) {
                int nextBlock = FAT[block];
                FAT[block] = -1;
                block = nextBlock;
            }
            directory[i].startBlock = -1;
            cout << "File '" << name << "' deleted successfully.\n";
            return;
        }
    }
    cout << "Error: File not found.\n";
}

void consoleInterface(const char* diskName) {
    FILE* disk = fopen(diskName, "r+b");
    if (!disk) {
        perror("Error opening disk file");
        exit(EXIT_FAILURE);
    }
    loadFileSystemState(disk);


    while (true) {
        cout << "\nFile System Menu:\n";
        cout << "1. Create File\n";
        cout << "2. Delete File\n";
        cout << "3. Write to File\n";
        cout << "4. Read File\n";
        cout << "5. Truncate File\n";
        cout << "6. List Files\n";
        cout << "7. Format Disk\n";
        cout << "8. Create Directory\n";
        cout << "9. Delete Directory\n";
        cout << "10. List Files & Directories\n";
        cout << "11. Encrypt Partition\n";
        cout << "12. Decrypt Partition\n";
        cout << "13. Exit\n";
        cout << "Enter your choice: ";

        int choice;
        cin >> choice;

        switch (choice) {
        case 1: {
            char name[MAX_FILE_NAME];
            int size;
            cout << "Enter file name: ";
            cin >> name;
            cout << "Enter file size (bytes): ";
            cin >> size;
            createFile(name, size);
            break;
        }
        case 2: {
            char name[MAX_FILE_NAME];
            cout << "Enter file name to delete: ";
            cin >> name;
            deleteFile(name);
            break;
        }
        case 3: {
            char name[MAX_FILE_NAME];
            cout << "Enter file name to write to: ";
            cin >> name;
            char data[MAX_FILE_SIZE];
            cout << "Enter data to write: ";
            cin.ignore();
            cin.getline(data, MAX_FILE_SIZE);
            writeFile(name, data, strlen(data));
            break;
        }
        case 4: {
            char name[MAX_FILE_NAME];
            cout << "Enter file name to read: ";
            cin >> name;
            readFile(name);
            break;
        }
        case 5: {
            char name[MAX_FILE_NAME];
            int newSize;
            cout << "Enter file name to truncate: ";
            cin >> name;
            cout << "Enter new size (bytes): ";
            cin >> newSize;
            truncateFile(name, newSize);
            break;
        }
        case 6:
        case 10: // Combine both cases for list
            listFilesAndDirectories();
            break;
        case 7:
            formatDisk(diskName);
            break;
        case 8: {
            char name[MAX_FILE_NAME];
            cout << "Enter directory name: ";
            cin >> name;
            createDirectory(name);
            break;
        }
        case 9: {
            char name[MAX_FILE_NAME];
            cout << "Enter directory name: ";
            cin >> name;
            deleteDirectory(name);
            break;
        }
        case 11: {
            char key[BLOCK_SIZE];
            cout << "Enter encryption key: ";
            cin >> key;
            encryptPartition(diskName, key);
            break;
        }
        case 12: {
            char key[BLOCK_SIZE];
            cout << "Enter decryption key: ";
            cin >> key;
            decryptPartition(diskName, key);
            break;
        }
        case 13:
            saveFileSystemState(disk);
            fclose(disk);
            cout << "Exiting the file system. Goodbye!\n";
            return;
        default:
            cout << "Invalid choice. Please try again.\n";
            break;
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


