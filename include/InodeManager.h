#ifndef INODE_MANAGER_H
#define INODE_MANAGER_H

#include <cstdint>
#include "structures.h"
#include "DiskEmulator.h"
#include "Bitmap.h"

// InodeManager 類別 - 管理 inode 的分配、釋放和區塊索引
class InodeManager {
private:
    DiskEmulator* disk_;
    Bitmap* inode_bitmap_;
    Bitmap* block_bitmap_;
    Superblock superblock_;

public:
    InodeManager(DiskEmulator* disk, Bitmap* inode_bitmap, Bitmap* block_bitmap);
    
    // 載入 superblock
    bool loadSuperblock();
    
    // 保存 superblock
    bool saveSuperblock();
    
    // 獲取 superblock
    const Superblock& getSuperblock() const { return superblock_; }
    
    // 分配一個 inode，返回 inode 編號
    int allocateInode(FileType type);
    
    // 釋放一個 inode 及其所有區塊
    bool freeInode(uint32_t inode_num);
    
    // 讀取 inode
    bool readInode(uint32_t inode_num, Inode& inode);
    
    // 寫入 inode
    bool writeInode(uint32_t inode_num, const Inode& inode);
    
    // 分配一個資料區塊
    int allocateBlock();
    
    // 釋放一個資料區塊
    bool freeBlock(uint32_t block_num);
    
    // 獲取檔案的第 n 個邏輯區塊對應的實際區塊號
    // 如果不存在且 allocate=true，則分配新區塊
    int getBlockNumber(Inode& inode, uint32_t logical_block, bool allocate = false);
    
    // 釋放 inode 的所有資料區塊
    bool freeAllBlocks(Inode& inode);

private:
    // Indirect block support removed
};

#endif // INODE_MANAGER_H
