#include "InodeManager.h"
#include <iostream>
#include <cstring>
#include <ctime>

InodeManager::InodeManager(DiskEmulator* disk, Bitmap* inode_bitmap, Bitmap* block_bitmap)
    : disk_(disk), inode_bitmap_(inode_bitmap), block_bitmap_(block_bitmap) {
}

bool InodeManager::loadSuperblock() {
    return disk_->readBlock(Config::SUPERBLOCK_BLOCK, &superblock_);
}

bool InodeManager::saveSuperblock() {
    return disk_->writeBlock(Config::SUPERBLOCK_BLOCK, &superblock_);
}

int InodeManager::allocateInode(FileType type) {
    int inode_num = inode_bitmap_->allocate();
    if (inode_num < 0) {
        std::cerr << "無法分配 inode：已滿" << std::endl;
        return -1;
    }

    // 初始化 inode
    Inode inode;
    inode.type = type;
    inode.size = 0;
    inode.link_count = 1;
    inode.block_count = 0;
    
    uint64_t current_time = static_cast<uint64_t>(std::time(nullptr));
    inode.atime = inode.mtime = inode.ctime = current_time;

    // 寫入 inode
    if (!writeInode(inode_num, inode)) {
        inode_bitmap_->deallocate(inode_num);
        return -1;
    }

    // 更新 superblock
    superblock_.free_inodes--;
    saveSuperblock();

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
    inode = Inode();
    writeInode(inode_num, inode);

    // 在 bitmap 中標記為空閒
    inode_bitmap_->deallocate(inode_num);

    // 更新 superblock
    superblock_.free_inodes++;
    saveSuperblock();

    return true;
}

bool InodeManager::readInode(uint32_t inode_num, Inode& inode) {
    if (inode_num >= Config::TOTAL_INODES) {
        std::cerr << "Inode 編號超出範圍: " << inode_num << std::endl;
        return false;
    }

    // 計算 inode 所在的區塊
    uint32_t inodes_per_block = Config::BLOCK_SIZE / Config::INODE_SIZE;
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

    // 計算 inode 所在的區塊
    uint32_t inodes_per_block = Config::BLOCK_SIZE / Config::INODE_SIZE;
    uint32_t block_num = Config::INODE_TABLE_START + (inode_num / inodes_per_block);
    uint32_t offset = (inode_num % inodes_per_block) * Config::INODE_SIZE;

    // 讀取區塊
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
    block_num += Config::DATA_BLOCKS_START;

    // 清空區塊
    std::vector<uint8_t> empty_block(Config::BLOCK_SIZE, 0);
    disk_->writeBlock(block_num, empty_block.data());

    // 更新 superblock
    superblock_.free_blocks--;
    saveSuperblock();

    return block_num;
}

bool InodeManager::freeBlock(uint32_t block_num) {
    if (block_num < Config::DATA_BLOCKS_START || block_num >= Config::TOTAL_BLOCKS) {
        return false;
    }

    // 轉換為 bitmap 索引
    uint32_t bitmap_index = block_num - Config::DATA_BLOCKS_START;
    block_bitmap_->deallocate(bitmap_index);

    // 更新 superblock
    superblock_.free_blocks++;
    saveSuperblock();

    return true;
}

int InodeManager::getBlockNumber(Inode& inode, uint32_t logical_block, bool allocate) {
    // 直接區塊
    if (logical_block < Config::DIRECT_BLOCKS) {
        if (inode.direct[logical_block] == 0 && allocate) {
            int new_block = allocateBlock();
            if (new_block < 0) return -1;
            inode.direct[logical_block] = new_block;
            inode.block_count++;
        }
        return inode.direct[logical_block];
    }

    logical_block -= Config::DIRECT_BLOCKS;

    // 單重間接區塊
    if (logical_block < Config::POINTERS_PER_BLOCK) {
        if (inode.single_indirect == 0 && allocate) {
            int new_block = allocateBlock();
            if (new_block < 0) return -1;
            inode.single_indirect = new_block;
            inode.block_count++;
        }
        if (inode.single_indirect == 0) return 0;
        return getIndirectBlock(inode.single_indirect, logical_block, allocate);
    }

    logical_block -= Config::POINTERS_PER_BLOCK;

    // 雙重間接區塊
    if (logical_block < Config::POINTERS_PER_BLOCK * Config::POINTERS_PER_BLOCK) {
        if (inode.double_indirect == 0 && allocate) {
            int new_block = allocateBlock();
            if (new_block < 0) return -1;
            inode.double_indirect = new_block;
            inode.block_count++;
        }
        if (inode.double_indirect == 0) return 0;
        return getDoubleIndirectBlock(inode.double_indirect, logical_block, allocate);
    }

    return -1;  // 超出範圍
}

int InodeManager::getIndirectBlock(uint32_t indirect_block, uint32_t index, bool allocate) {
    std::vector<uint32_t> pointers(Config::POINTERS_PER_BLOCK, 0);
    disk_->readBlock(indirect_block, pointers.data());

    if (pointers[index] == 0 && allocate) {
        int new_block = allocateBlock();
        if (new_block < 0) return -1;
        pointers[index] = new_block;
        disk_->writeBlock(indirect_block, pointers.data());
    }

    return pointers[index];
}

int InodeManager::getDoubleIndirectBlock(uint32_t double_indirect_block, uint32_t index, bool allocate) {
    uint32_t first_level = index / Config::POINTERS_PER_BLOCK;
    uint32_t second_level = index % Config::POINTERS_PER_BLOCK;

    std::vector<uint32_t> first_pointers(Config::POINTERS_PER_BLOCK, 0);
    disk_->readBlock(double_indirect_block, first_pointers.data());

    if (first_pointers[first_level] == 0 && allocate) {
        int new_block = allocateBlock();
        if (new_block < 0) return -1;
        first_pointers[first_level] = new_block;
        disk_->writeBlock(double_indirect_block, first_pointers.data());
    }

    if (first_pointers[first_level] == 0) return 0;
    return getIndirectBlock(first_pointers[first_level], second_level, allocate);
}

bool InodeManager::freeAllBlocks(Inode& inode) {
    // 釋放直接區塊
    for (uint32_t i = 0; i < Config::DIRECT_BLOCKS; ++i) {
        if (inode.direct[i] != 0) {
            freeBlock(inode.direct[i]);
        }
    }

    // 釋放單重間接區塊
    if (inode.single_indirect != 0) {
        std::vector<uint32_t> pointers(Config::POINTERS_PER_BLOCK);
        disk_->readBlock(inode.single_indirect, pointers.data());
        for (uint32_t ptr : pointers) {
            if (ptr != 0) freeBlock(ptr);
        }
        freeBlock(inode.single_indirect);
    }

    // 釋放雙重間接區塊
    if (inode.double_indirect != 0) {
        std::vector<uint32_t> first_pointers(Config::POINTERS_PER_BLOCK);
        disk_->readBlock(inode.double_indirect, first_pointers.data());
        for (uint32_t first_ptr : first_pointers) {
            if (first_ptr != 0) {
                std::vector<uint32_t> second_pointers(Config::POINTERS_PER_BLOCK);
                disk_->readBlock(first_ptr, second_pointers.data());
                for (uint32_t second_ptr : second_pointers) {
                    if (second_ptr != 0) freeBlock(second_ptr);
                }
                freeBlock(first_ptr);
            }
        }
        freeBlock(inode.double_indirect);
    }

    return true;
}
