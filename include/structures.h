#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <cstdint>
#include <cstring>
#include "config.h"

// 檔案類型枚舉 (Fixed size to uint32_t for struct alignment/simplicity if needed, but uint8 is fine if padded)
// Spec uses uint32_t for file_type in Inode struct
enum class FileType : uint32_t {
    FREE = 0,        // 未使用
    REGULAR = 1,     // 一般檔案
    DIRECTORY = 2    // 目錄
};

// Superblock 結構 - 儲存檔案系統的全局資訊
// Size MUST vary to be exactly BLOCK_SIZE (4096)
struct Superblock {
    uint32_t magic;                 // 魔數
    uint32_t total_blocks;          // 總區塊數 (4096)
    uint32_t total_inodes;          // 總 Inode 數 (256)
    uint32_t inode_bitmap_blk;      // Inode Bitmap 的起始區塊 (1)
    uint32_t block_bitmap_blk;      // Block Bitmap 的起始區塊 (2)
    uint32_t inode_table_blk;       // Inode Table 的起始區塊 (3)
    uint32_t data_blk_start;        // 資料區起始區塊 (7)
    
    // Padding to fill 4096 bytes
    // Used 7 * 4 = 28 bytes.
    uint8_t padding[Config::BLOCK_SIZE - 28];

    Superblock() {
        std::memset(this, 0, sizeof(Superblock));
        magic = Config::MAGIC_NUMBER;
        total_blocks = Config::TOTAL_BLOCKS;
        total_inodes = Config::TOTAL_INODES;
        inode_bitmap_blk = Config::INODE_BITMAP_BLOCK;
        block_bitmap_blk = Config::BLOCK_BITMAP_BLOCK;
        inode_table_blk = Config::INODE_TABLE_START;
        data_blk_start = Config::DATA_BLOCKS_START;
    }
};

// Inode 結構 - 64 Bytes fixed
struct Inode {
    uint32_t inode_num;     // Inode 編號 (0-255)
    FileType file_type;     // 0: 未使用, 1: 一般檔案, 2: 目錄 (uint32_t base)
    uint32_t size;          // 檔案實際大小 (Bytes)
    uint32_t direct_blks[Config::DIRECT_BLOCKS]; // 直接資料區塊指標 (4個) -> 16 bytes

    // Used so far: 4 + 4 + 4 + 16 = 28 bytes.
    // Need to fill 64 bytes.
    // Padding = 64 - 28 = 36 bytes.
    uint32_t padding[9];    // 9 * 4 = 36 bytes

    Inode() {
        std::memset(this, 0, sizeof(Inode));
        file_type = FileType::FREE;
    }

    bool isDirectory() const { return file_type == FileType::DIRECTORY; }
    bool isFile() const { return file_type == FileType::REGULAR; }
    bool isFree() const { return file_type == FileType::FREE; }
};

// 目錄項目結構 (Dentry) - 32 Bytes fixed
struct DirectoryEntry {
    uint32_t inode_number;             // 對應的 inode 編號. 0 indicates empty/invalid slot? 
                                       // NOTE: Inode 0 is usually Root. 
                                       // Spec says "If inode is 0, logic should consider empty".
                                       // But Root IS 0. We need to handle this.
                                       // Convention: Maybe use 0 for valid, but initialize unknown to -1? 
                                       // OR, simply: Root is special.
                                       // Actually, typical FS: Inode 0 is root. Inode numbers start at 1?
                                       // Spec says "Inode (0-255)". Root is Inode 0.
                                       // "In memory build, mark Inode 0 as Root".
                                       // "In Dentry, if inode_number is 0, is it empty?"
                                       // Spec says: "If inode_num == 0 represents empty slot", then Root cannot be referred to as 0?
                                       // WAIT. Spec says: "Root Inode (0)".
                                       // And Spec for Dentry: "inode_num: Corresponding Inode Num. If 0 means empty."
                                       // This is a conflict if Root is 0. 
                                       // Solution: Let's assume Inode 0 IS Root. And "Empty" is flagged by a special value or just name[0] == 0? 
                                       // Standard dirent often uses inode=0 to mean unused IF inode 0 is reserved/invalid.
                                       // But here Inode 0 is valid. 
                                       // Let's us checking `name[0] == '\0'` to determine if empty, or use a reserved inode number for free (like 0xFFFFFFFF).
                                       // HOWEVER, Spec says "inode_num 0 represents empty". This implies we CANNOT use Inode 0 for Root if we follow that part of spec literally.
                                       // BUT Spec ALSO says "Root Inode (0)".
                                       // Let's stick to: Inode 0 is Root. We use a recognizable invalid value for "Empty" slot, e.g. 0xFFFFFFFF (MAX_UINT), OR simply rely on name being empty.
                                       // Let's check the spec again: "Inode 0 (Root)... Dentry: inode_num... if 0 means empty."
                                       // Okay, I will change Inode numbers to 1-based for files, so 0 can be "NULL".
                                       // OR, just use -1 (0xFFFFFFFF) for empty.
                                       // Let's use 0xFFFFFFFF for INVALID_INODE.
                                       
    char name[Config::MAX_FILENAME_LENGTH];  // 28 bytes

    DirectoryEntry() {
        std::memset(this, 0, sizeof(DirectoryEntry));
        inode_number = 0xFFFFFFFF; // Mark as empty by default
    }

    DirectoryEntry(uint32_t inode, const char* filename) {
        std::memset(this, 0, sizeof(DirectoryEntry));
        inode_number = inode;
        std::strncpy(name, filename, Config::MAX_FILENAME_LENGTH - 1); // safe copy
        name[Config::MAX_FILENAME_LENGTH - 1] = '\0';
    }
};

// 確保結構大小正確
static_assert(sizeof(Superblock) == Config::BLOCK_SIZE, "Superblock must be exactly one block");
static_assert(sizeof(Inode) == Config::INODE_SIZE, "Inode must be exactly 64 bytes");
static_assert(sizeof(DirectoryEntry) == Config::DENTRY_SIZE, "Dentry must be exactly 32 bytes");

#endif // STRUCTURES_H
