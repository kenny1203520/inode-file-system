#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

// 檔案系統配置常數
namespace Config {
    // 魔數（用於識別檔案系統類型）
    constexpr uint32_t MAGIC_NUMBER = 0x5D3F;    // As per Master Spec

    // 區塊配置
    constexpr uint32_t BLOCK_SIZE = 4096;          // 4KB per block
    constexpr uint32_t TOTAL_BLOCKS = 4096;        // 4096 blocks * 4KB = 16MB
    
    // Inode 配置
    constexpr uint32_t TOTAL_INODES = 256;         // 256 Inodes
    constexpr uint32_t INODE_SIZE = 64;            // 64 bytes per inode
    constexpr uint32_t INODES_PER_BLOCK = BLOCK_SIZE / INODE_SIZE;  // 64 inodes per block
    
    // 檔案系統佈局 (Block IDs)
    constexpr uint32_t SUPERBLOCK_BLOCK = 0;       // Superblock
    constexpr uint32_t INODE_BITMAP_BLOCK = 1;     // Inode Bitmap
    constexpr uint32_t BLOCK_BITMAP_BLOCK = 2;     // Block Bitmap
    constexpr uint32_t INODE_TABLE_START = 3;      // Inode Table Start
    constexpr uint32_t INODE_TABLE_BLOCKS = 4;     // 256 inodes * 64 bytes = 16KB = 4 blocks
    constexpr uint32_t DATA_BLOCKS_START = 7;      // Data Blocks Start (0-6 used by metadata)

    // Data Block parameters
    // We only support direct pointers in the basic spec, but Master Spec mentions 4 direct blocks is small.
    // However, to strictly follow the struct definition in the Master Spec (64 bytes), we have space constraints.
    // Struct logic: 32 bytes for fields + padding.
    // Let's implement what fits in 64 bytes.
    // Master Spec suggests: 4 direct blocks. Or maybe more if we optimize.
    // Master Spec Struct:
    // uint32_t inode_num; (4)
    // uint32_t file_type; (4)
    // uint32_t size; (4)
    // uint32_t direct_blks[1]; (Wait, spec said "suggest implement 4", but the struct example showed `direct_blks[1]`... 
    // actually reading the example struct carefully:
    // uint32_t direct_blks[1]; // "suggest implement 4 direct block" - technically an array of size 1 is 4 bytes. 
    // But the text says "4 direct blocks". If we use 4 direct blocks, that takes 16 bytes.
    // 4+4+4+16 = 28 bytes.
    // 64 bytes total. 36 bytes padding.
    // So we can support 4 direct blocks easily.

    constexpr uint32_t DIRECT_BLOCKS = 4;          // 4 Direct Blocks

    // 檔案名稱與路徑
    constexpr uint32_t MAX_FILENAME_LENGTH = 28;   // 28 bytes (including null) to align with 32-byte Dentry
    constexpr uint32_t MAX_PATH_LENGTH = 4096;
    constexpr uint32_t DENTRY_SIZE = 32;           // Fixed 32 bytes
    constexpr uint32_t DIR_ENTRIES_PER_BLOCK = BLOCK_SIZE / DENTRY_SIZE; // 128 entries

    // 根目錄
    constexpr uint32_t ROOT_INODE = 0;             // Inode 0 is root
}

#endif // CONFIG_H
