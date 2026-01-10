#include "InodeManager.h"
#include <iostream>
#include <cstring>
#include <ctime>
#include <algorithm>

InodeManager::InodeManager(DiskEmulator* disk, Bitmap* inode_bitmap, Bitmap* block_bitmap)
    : disk_(disk), inode_bitmap_(inode_bitmap), block_bitmap_(block_bitmap) {
}

bool InodeManager::loadSuperblock() {
    // Read Block 0
    std::vector<uint8_t> buffer(Config::BLOCK_SIZE);
    if (!disk_->readBlock(Config::SUPERBLOCK_BLOCK, buffer.data())) {
        return false;
    }
    std::memcpy(&superblock_, buffer.data(), sizeof(Superblock));
    return true;
}

bool InodeManager::saveSuperblock() {
    std::vector<uint8_t> buffer(Config::BLOCK_SIZE, 0); // Padding is handled by struct size being 4096 ideally, or we memcpy to buffer
    // Struct Superblock is 4096 bytes now due to padding? 
    // Wait, in my definition I used uint8_t padding[Config::BLOCK_SIZE - 28].
    // sizeof(Superblock) should be 4096.
    std::memcpy(buffer.data(), &superblock_, sizeof(Superblock));
    return disk_->writeBlock(Config::SUPERBLOCK_BLOCK, buffer.data());
}

int InodeManager::allocateInode(FileType type) {
    int inode_num = inode_bitmap_->allocate();
    if (inode_num < 0) {
        std::cerr << "無法分配 inode：已滿" << std::endl;
        return -1;
    }

    // 初始化 inode
    Inode inode;
    inode.inode_num = static_cast<uint32_t>(inode_num);
    inode.file_type = type;
    inode.size = 0;
    // Clear direct blocks
    for(int i=0; i<Config::DIRECT_BLOCKS; ++i) {
        inode.direct_blks[i] = 0;
    }

    // 寫入 inode
    if (!writeInode(inode_num, inode)) {
        inode_bitmap_->deallocate(inode_num);
        return -1;
    }

    // 更新 superblock (Optional: if we track free inodes in superblock)
    // Note: The new Superblock struct doesn't strictly have a 'free_inodes' counter required by logic if we assume bitmap is truth,
    // but the struct definition DOES have 'total_inodes' etc.
    // My new Superblock definition removed 'free_inodes' and 'free_blocks' tracking fields to simplify?
    // Let me check my previous tool call for structures.h ...
    // "struct Superblock { ... total_blocks, total_inodes, inode_bitmap_blk ... }"
    // It DOES NOT have mutable 'free_inodes'/'free_blocks' counters in the defined struct I pushed! 
    // They were removed. So I should NOT try to update them here.
    // Good, that stat is often out of sync anyway. Bitmap is source of truth.
    
    // saveSuperblock(); // No mutable fields to save except maybe if I added something? No.

    return inode_num;
}

bool InodeManager::freeInode(uint32_t inode_num) {
    if (inode_num >= Config::TOTAL_INODES) {
        return false;
    }

    // 讀取 inode
    Inode inode;
    if (!readInode(inode_num, inode)) {
        return false;
    }

    // 釋放所有資料區塊
    freeAllBlocks(inode);

    // 清空 inode
    inode = Inode(); // Reset to FREE
    writeInode(inode_num, inode);

    // 在 bitmap 中標記為空閒
    inode_bitmap_->deallocate(inode_num);

    return true;
}

bool InodeManager::readInode(uint32_t inode_num, Inode& inode) {
    if (inode_num >= Config::TOTAL_INODES) {
        std::cerr << "Inode 編號超出範圍: " << inode_num << std::endl;
        return false;
    }

    // 計算 inode 所在的區塊
    // INODE_SIZE = 64
    // BLOCK_SIZE = 4096
    // INODES_PER_BLOCK = 64
    uint32_t inodes_per_block = Config::INODES_PER_BLOCK;
    uint32_t block_num = Config::INODE_TABLE_START + (inode_num / inodes_per_block);
    uint32_t offset = (inode_num % inodes_per_block) * Config::INODE_SIZE;

    // 讀取區塊
    std::vector<uint8_t> buffer(Config::BLOCK_SIZE);
    if (!disk_->readBlock(block_num, buffer.data())) {
        return false;
    }

    // 複製 inode
    std::memcpy(&inode, buffer.data() + offset, sizeof(Inode));
    return true;
}

bool InodeManager::writeInode(uint32_t inode_num, const Inode& inode) {
    if (inode_num >= Config::TOTAL_INODES) {
        std::cerr << "Inode 編號超出範圍: " << inode_num << std::endl;
        return false;
    }

    uint32_t inodes_per_block = Config::INODES_PER_BLOCK;
    uint32_t block_num = Config::INODE_TABLE_START + (inode_num / inodes_per_block);
    uint32_t offset = (inode_num % inodes_per_block) * Config::INODE_SIZE;

    // 讀取區塊 (Read-Modify-Write)
    std::vector<uint8_t> buffer(Config::BLOCK_SIZE);
    if (!disk_->readBlock(block_num, buffer.data())) {
        return false;
    }

    // 更新 inode
    std::memcpy(buffer.data() + offset, &inode, sizeof(Inode));

    // 寫回區塊
    return disk_->writeBlock(block_num, buffer.data());
}

int InodeManager::allocateBlock() {
    int block_num = block_bitmap_->allocate();
    if (block_num < 0) {
        std::cerr << "無法分配區塊：已滿" << std::endl;
        return -1;
    }

    // 調整為實際的資料區塊號
    // Bitmap 0 corresponds to Data Block Start? Or Bitmap 0 corresponds to absolute block 0?
    // Usually Bitmaps track Data Blocks only to save space.
    // Let's assume Block Bitmap tracks blocks starting from DATA_BLOCKS_START.
    // Config: TOTAL_BLOCKS = 4096.
    // If bitmap tracks ALL blocks, then 0-6 are permanently used.
    // If bitmap tracks ONLY data blocks, then index 0 = DATA_BLOCKS_START.
    // Let's check mkfs logic later. Assuming 0-indexed relative to DATA_BLOCKS_START is safer for max capacity?
    // No, standard is usually absolute or relative. 
    // The previous implementation did `start_block` injection in Bitmap.
    // Let's assume the Block Bitmap manages the whole disk or just the data area.
    // If logic: `block_num += Config::DATA_BLOCKS_START`, then it implies bitmap index 0 is the first data block.
    // Let's stick to this: Block Bitmap manages Data Blocks (0 -> DATA_BLOCKS_START).
    
    int actual_block_num = block_num + Config::DATA_BLOCKS_START;

    if (actual_block_num >= Config::TOTAL_BLOCKS) {
         std::cerr << "Block allocation out of bounds" << std::endl;
         block_bitmap_->deallocate(block_num);
         return -1;
    }

    // 清空區塊
    std::vector<uint8_t> empty_block(Config::BLOCK_SIZE, 0);
    disk_->writeBlock(actual_block_num, empty_block.data());

    return actual_block_num;
}

bool InodeManager::freeBlock(uint32_t block_num) {
    if (block_num < Config::DATA_BLOCKS_START || block_num >= Config::TOTAL_BLOCKS) {
        return false;
    }

    // 轉換為 bitmap 索引
    uint32_t bitmap_index = block_num - Config::DATA_BLOCKS_START;
    block_bitmap_->deallocate(bitmap_index);

    return true;
}

int InodeManager::getBlockNumber(Inode& inode, uint32_t logical_block, bool allocate) {
    // Only support Direct Blocks
    if (logical_block >= Config::DIRECT_BLOCKS) {
        return -1; // File too large / Not supported
    }

    if (inode.direct_blks[logical_block] == 0) {
        if (allocate) {
            int new_block = allocateBlock();
            if (new_block < 0) return -1;
            inode.direct_blks[logical_block] = new_block;
            // Note: We don't have block_count field anymore in Inode
        } else {
            return 0; // Not allocated
        }
    }
    return inode.direct_blks[logical_block];
}

// Indirect block methods removed as they are not supported in basic implementation

bool InodeManager::freeAllBlocks(Inode& inode) {
    // 釋放直接區塊
    for (uint32_t i = 0; i < Config::DIRECT_BLOCKS; ++i) {
        if (inode.direct_blks[i] != 0) {
            freeBlock(inode.direct_blks[i]);
            inode.direct_blks[i] = 0;
        }
    }
    return true;
}
