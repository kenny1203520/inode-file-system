#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <cstdint>
#include <cstring>
#include "config.h"

// 檔案類型枚舉
enum class FileType : uint8_t {
    FREE = 0,        // 未使用
    REGULAR = 1,     // 一般檔案
    DIRECTORY = 2    // 目錄
};

// Superblock 結構 - 儲存檔案系統的全局資訊
struct Superblock {
    uint32_t magic;                    // 魔數，用於識別檔案系統
    uint32_t total_blocks;             // 總區塊數
    uint32_t total_inodes;             // 總 inode 數
    uint32_t free_blocks;              // 可用區塊數
    uint32_t free_inodes;              // 可用 inode 數
    uint32_t block_size;               // 區塊大小
    uint32_t inode_size;               // Inode 大小
    uint32_t inode_bitmap_blocks;      // Inode bitmap 佔用的塊數
    uint32_t block_bitmap_blocks;      // Block bitmap 佔用的塊數
    uint32_t inode_table_start;        // Inode table 起始塊號
    uint32_t data_blocks_start;        // 資料塊起始塊號
    uint8_t padding[Config::BLOCK_SIZE - 44];  // 填充到 4KB

    Superblock() {
        std::memset(this, 0, sizeof(Superblock));
        magic = Config::MAGIC_NUMBER;
        total_blocks = Config::TOTAL_BLOCKS;
        total_inodes = Config::TOTAL_INODES;
        free_blocks = Config::TOTAL_BLOCKS - Config::DATA_BLOCKS_START;
        free_inodes = Config::TOTAL_INODES;
        block_size = Config::BLOCK_SIZE;
        inode_size = Config::INODE_SIZE;
        inode_bitmap_blocks = Config::INODE_BITMAP_BLOCKS;
        block_bitmap_blocks = Config::BLOCK_BITMAP_BLOCKS;
        inode_table_start = Config::INODE_TABLE_START;
        data_blocks_start = Config::DATA_BLOCKS_START;
    }
};

// Inode 結構 - 儲存檔案或目錄的元數據
struct Inode {
    FileType type;                     // 檔案類型
    uint32_t size;                     // 檔案大小（bytes）
    uint32_t link_count;               // 硬連結計數
    uint32_t block_count;              // 使用的區塊數
    uint32_t direct[Config::DIRECT_BLOCKS];      // 直接區塊指標（12個）
    uint32_t single_indirect;          // 單重間接區塊指標
    uint32_t double_indirect;          // 雙重間接區塊指標
    uint64_t atime;                    // 最後訪問時間
    uint64_t mtime;                    // 最後修改時間
    uint64_t ctime;                    // 創建時間
    uint8_t padding[128 - 1 - 4*4 - 4*12 - 4*2 - 8*3];  // 填充到 128 bytes

    Inode() {
        std::memset(this, 0, sizeof(Inode));
        type = FileType::FREE;
    }

    // 檢查是否為目錄
    bool isDirectory() const {
        return type == FileType::DIRECTORY;
    }

    // 檢查是否為檔案
    bool isFile() const {
        return type == FileType::REGULAR;
    }

    // 檢查是否空閒
    bool isFree() const {
        return type == FileType::FREE;
    }
};

// 目錄項目結構
struct DirectoryEntry {
    uint32_t inode_number;             // 對應的 inode 編號
    char name[Config::MAX_FILENAME_LENGTH + 1];  // 檔案/目錄名稱（null-terminated）
    bool valid;                        // 是否有效

    DirectoryEntry() {
        std::memset(this, 0, sizeof(DirectoryEntry));
        valid = false;
        inode_number = 0;
    }

    DirectoryEntry(uint32_t inode, const char* filename) {
        std::memset(this, 0, sizeof(DirectoryEntry));
        inode_number = inode;
        std::strncpy(name, filename, Config::MAX_FILENAME_LENGTH);
        name[Config::MAX_FILENAME_LENGTH] = '\0';
        valid = true;
    }
};

// 確保結構大小正確
static_assert(sizeof(Superblock) == Config::BLOCK_SIZE, "Superblock must be exactly one block");
static_assert(sizeof(Inode) == Config::INODE_SIZE, "Inode must be exactly INODE_SIZE bytes");

#endif // STRUCTURES_H
