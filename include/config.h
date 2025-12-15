#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

// 檔案系統配置常數
namespace Config {
    // 魔數（用於識別檔案系統類型）
    constexpr uint32_t MAGIC_NUMBER = 0x494E4F44;  // "INOD" in hex

    // 區塊配置
    constexpr uint32_t BLOCK_SIZE = 4096;          // 4KB per block
    constexpr uint32_t TOTAL_BLOCKS = 1024;        // 總共 1024 個區塊 (4MB)
    
    // Inode 配置
    constexpr uint32_t TOTAL_INODES = 128;         // 最多 128 個檔案/目錄
    constexpr uint32_t INODE_SIZE = 128;           // 每個 inode 128 bytes
    constexpr uint32_t INODES_PER_BLOCK = BLOCK_SIZE / INODE_SIZE;  // 每塊 32 個 inodes

    // 區塊指標配置
    constexpr uint32_t DIRECT_BLOCKS = 12;         // 直接指標數量
    constexpr uint32_t POINTERS_PER_BLOCK = BLOCK_SIZE / sizeof(uint32_t);  // 1024 個指標/塊
    
    // 檔案系統佈局
    constexpr uint32_t SUPERBLOCK_BLOCK = 0;       // Superblock 位置
    constexpr uint32_t INODE_BITMAP_START = 1;     // Inode bitmap 起始塊
    constexpr uint32_t INODE_BITMAP_BLOCKS = 1;    // Inode bitmap 佔用塊數
    constexpr uint32_t BLOCK_BITMAP_START = 2;     // Block bitmap 起始塊
    constexpr uint32_t BLOCK_BITMAP_BLOCKS = 1;    // Block bitmap 佔用塊數
    constexpr uint32_t INODE_TABLE_START = 3;      // Inode table 起始塊
    constexpr uint32_t INODE_TABLE_BLOCKS = (TOTAL_INODES + INODES_PER_BLOCK - 1) / INODES_PER_BLOCK;  // 4 塊
    constexpr uint32_t DATA_BLOCKS_START = INODE_TABLE_START + INODE_TABLE_BLOCKS;  // 資料塊起始位置

    // 檔案名稱與路徑
    constexpr uint32_t MAX_FILENAME_LENGTH = 255;  // 最大檔名長度
    constexpr uint32_t MAX_PATH_LENGTH = 4096;     // 最大路徑長度

    // 目錄項目
    constexpr uint32_t DIR_ENTRIES_PER_BLOCK = BLOCK_SIZE / (MAX_FILENAME_LENGTH + sizeof(uint32_t) + 1);

    // 根目錄
    constexpr uint32_t ROOT_INODE = 0;             // 根目錄的 inode 編號
}

#endif // CONFIG_H
