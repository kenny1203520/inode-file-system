#ifndef BITMAP_H
#define BITMAP_H

#include <cstdint>
#include <vector>
#include "DiskEmulator.h"

// Bitmap 類別 - 管理空閒區塊或 inode 的分配
class Bitmap {
private:
    DiskEmulator* disk_;
    uint32_t start_block_;      // bitmap 起始區塊號
    uint32_t num_blocks_;       // bitmap 佔用的區塊數
    uint32_t total_bits_;       // 總位元數
    std::vector<uint8_t> data_; // bitmap 資料（在記憶體中）

public:
    // 建構子
    Bitmap(DiskEmulator* disk, uint32_t start_block, uint32_t num_blocks, uint32_t total_bits);
    
    // 載入 bitmap 從磁碟
    bool load();
    
    // 保存 bitmap 到磁碟
    bool save();
    
    // 分配一個空閒位元，返回位元索引，-1 表示失敗
    int allocate();
    
    // 釋放一個位元
    bool deallocate(uint32_t index);
    
    // 檢查某個位元是否空閒
    bool isFree(uint32_t index) const;
    
    // 設置某個位元
    bool setBit(uint32_t index);
    
    // 清除某個位元
    bool clearBit(uint32_t index);
    
    // 獲取空閒位元數量
    uint32_t countFree() const;
};

#endif // BITMAP_H
