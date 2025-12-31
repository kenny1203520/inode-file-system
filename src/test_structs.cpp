#include <iostream>
#include "structures.h"
#include "config.h"

int main() {
    std::cout << "=== 結構大小檢查 ===" << std::endl;
    std::cout << "sizeof(DirectoryEntry) = " << sizeof(DirectoryEntry) << " bytes" << std::endl;
    std::cout << "sizeof(Superblock) = " << sizeof(Superblock) << " bytes" << std::endl;
    std::cout << "sizeof(Inode) = " << sizeof(Inode) << " bytes" << std::endl;
    std::cout << "Config::BLOCK_SIZE = " << Config::BLOCK_SIZE << " bytes" << std::endl;
    std::cout << "Config::MAX_FILENAME_LENGTH = " << Config::MAX_FILENAME_LENGTH << std::endl;
    
    uint32_t entries_per_block = Config::BLOCK_SIZE / sizeof(DirectoryEntry);
    std::cout << "\nentries_per_block = " << entries_per_block << std::endl;
    std::cout << "bytes used per block = " << entries_per_block * sizeof(DirectoryEntry) << std::endl;
    std::cout << "wasted bytes = " << (Config::BLOCK_SIZE - entries_per_block * sizeof(DirectoryEntry)) << std::endl;
    
    std::cout << "\n=== DirectoryEntry 測試 ===" << std::endl;
    DirectoryEntry de1;
    std::cout << "默認構造: valid=" << de1.valid << ", inode=" << de1.inode_number << ", name=\"" << de1.name << "\"" << std::endl;
    
    DirectoryEntry de2(5, "test.txt");
    std::cout << "有參構造: valid=" << de2.valid << ", inode=" << de2.inode_number << ", name=\"" << de2.name << "\"" << std::endl;
    
    return 0;
}
