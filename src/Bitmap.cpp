#include "Bitmap.h"
#include <iostream>
#include <cstring>

Bitmap::Bitmap(DiskEmulator* disk, uint32_t start_block, uint32_t num_blocks, uint32_t total_bits)
    : disk_(disk), start_block_(start_block), num_blocks_(num_blocks), total_bits_(total_bits) {
    // 計算需要的位元組數
    uint32_t bytes_needed = (total_bits + 7) / 8;
    data_.resize(bytes_needed, 0);
}

bool Bitmap::load() {
    // 從磁碟載入 bitmap
    std::vector<uint8_t> block_buffer(disk_->getBlockSize());
    
    uint32_t bytes_to_read = data_.size();
    uint32_t bytes_read = 0;
    
    for (uint32_t i = 0; i < num_blocks_ && bytes_read < bytes_to_read; ++i) {
        if (!disk_->readBlock(start_block_ + i, block_buffer.data())) {
            std::cerr << "載入 bitmap 失敗: block " << (start_block_ + i) << std::endl;
            return false;
        }
        
        uint32_t bytes_to_copy = std::min(disk_->getBlockSize(), bytes_to_read - bytes_read);
        std::memcpy(data_.data() + bytes_read, block_buffer.data(), bytes_to_copy);
        bytes_read += bytes_to_copy;
    }
    
    return true;
}

bool Bitmap::save() {
    // 將 bitmap 保存到磁碟
    std::vector<uint8_t> block_buffer(disk_->getBlockSize(), 0);
    
    uint32_t bytes_to_write = data_.size();
    uint32_t bytes_written = 0;
    
    for (uint32_t i = 0; i < num_blocks_ && bytes_written < bytes_to_write; ++i) {
        uint32_t bytes_to_copy = std::min(disk_->getBlockSize(), bytes_to_write - bytes_written);
        std::memcpy(block_buffer.data(), data_.data() + bytes_written, bytes_to_copy);
        
        if (!disk_->writeBlock(start_block_ + i, block_buffer.data())) {
            std::cerr << "保存 bitmap 失敗: block " << (start_block_ + i) << std::endl;
            return false;
        }
        
        bytes_written += bytes_to_copy;
    }
    
    return true;
}

int Bitmap::allocate() {
    // 找到第一個空閒位元（0）並分配它
    for (uint32_t i = 0; i < total_bits_; ++i) {
        if (isFree(i)) {
            setBit(i);
            return static_cast<int>(i);
        }
    }
    return -1;  // 沒有空閒位元
}

bool Bitmap::deallocate(uint32_t index) {
    if (index >= total_bits_) {
        return false;
    }
    return clearBit(index);
}

bool Bitmap::isFree(uint32_t index) const {
    if (index >= total_bits_) {
        return false;
    }
    
    uint32_t byte_index = index / 8;
    uint32_t bit_offset = index % 8;
    
    return !(data_[byte_index] & (1 << bit_offset));
}

bool Bitmap::setBit(uint32_t index) {
    if (index >= total_bits_) {
        return false;
    }
    
    uint32_t byte_index = index / 8;
    uint32_t bit_offset = index % 8;
    
    data_[byte_index] |= (1 << bit_offset);
    return true;
}

bool Bitmap::clearBit(uint32_t index) {
    if (index >= total_bits_) {
        return false;
    }
    
    uint32_t byte_index = index / 8;
    uint32_t bit_offset = index % 8;
    
    data_[byte_index] &= ~(1 << bit_offset);
    return true;
}

uint32_t Bitmap::countFree() const {
    uint32_t count = 0;
    for (uint32_t i = 0; i < total_bits_; ++i) {
        if (isFree(i)) {
            ++count;
        }
    }
    return count;
}
