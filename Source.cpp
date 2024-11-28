#include "filesystem.h"
#include<iostream>
using namespace std;

int main() {
    const char* diskName = "disk.bin";
    loadFileSystemState(diskName);
    consoleInterface(diskName);
    
    return 0;
}
