#include "DiskEmulator.h"
#include <iostream>
#include <cstring>

// ===== FileDiskEmulator 實作 =====

FileDiskEmulator::FileDiskEmulator(const std::string& path, uint32_t total_blocks, uint32_t block_size)
    : disk_path_(path), total_blocks_(total_blocks), block_size_(block_size) {
}

FileDiskEmulator::~FileDiskEmulator() {
    if (file_.is_open()) {
        file_.close();
    }
}

bool FileDiskEmulator::initialize() {
    // 創建並初始化磁碟檔案
    file_.open(disk_path_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file_.is_open()) {
        std::cerr << "無法創建磁碟檔案: " << disk_path_ << std::endl;
        return false;
    }

    // 寫入空塊以初始化檔案大小
    std::vector<uint8_t> empty_block(block_size_, 0);
    for (uint32_t i = 0; i < total_blocks_; ++i) {
        file_.write(reinterpret_cast<const char*>(empty_block.data()), block_size_);
    }
    
    file_.flush();
    std::cout << "磁碟檔案初始化完成: " << disk_path_ 
              << " (大小: " << (total_blocks_ * block_size_ / 1024) << " KB)" << std::endl;
    return true;
}

bool FileDiskEmulator::readBlock(uint32_t block_num, void* buffer) {
    if (block_num >= total_blocks_) {
        std::cerr << "讀取區塊超出範圍: " << block_num << std::endl;
        return false;
    }

    if (!file_.is_open()) {
        file_.open(disk_path_, std::ios::in | std::ios::out | std::ios::binary);
        if (!file_.is_open()) {
            std::cerr << "無法開啟磁碟檔案進行讀取" << std::endl;
            return false;
        }
    }

    // 定位到區塊位置
    file_.seekg(static_cast<std::streamoff>(block_num) * block_size_, std::ios::beg);
    if (file_.fail()) {
        std::cerr << "定位失敗: block " << block_num << std::endl;
        return false;
    }

    // 讀取區塊
    file_.read(reinterpret_cast<char*>(buffer), block_size_);
    if (file_.fail() && !file_.eof()) {
        std::cerr << "讀取區塊失敗: " << block_num << std::endl;
        return false;
    }

    return true;
}

bool FileDiskEmulator::writeBlock(uint32_t block_num, const void* buffer) {
    if (block_num >= total_blocks_) {
        std::cerr << "寫入區塊超出範圍: " << block_num << std::endl;
        return false;
    }

    if (!file_.is_open()) {
        file_.open(disk_path_, std::ios::in | std::ios::out | std::ios::binary);
        if (!file_.is_open()) {
            std::cerr << "無法開啟磁碟檔案進行寫入" << std::endl;
            return false;
        }
    }

    // 定位到區塊位置
    file_.seekp(static_cast<std::streamoff>(block_num) * block_size_, std::ios::beg);
    if (file_.fail()) {
        std::cerr << "定位失敗: block " << block_num << std::endl;
        return false;
    }

    // 寫入區塊
    file_.write(reinterpret_cast<const char*>(buffer), block_size_);
    if (file_.fail()) {
        std::cerr << "寫入區塊失敗: " << block_num << std::endl;
        return false;
    }

    file_.flush();
    return true;
}

// ===== MemoryDiskEmulator 實作 =====

MemoryDiskEmulator::MemoryDiskEmulator(uint32_t total_blocks, uint32_t block_size)
    : total_blocks_(total_blocks), block_size_(block_size) {
    // 分配記憶體空間
    memory_.resize(static_cast<size_t>(total_blocks) * block_size, 0);
    std::cout << "記憶體磁碟初始化完成 (大小: " 
              << (total_blocks * block_size / 1024) << " KB)" << std::endl;
}

bool MemoryDiskEmulator::readBlock(uint32_t block_num, void* buffer) {
    if (block_num >= total_blocks_) {
        std::cerr << "讀取區塊超出範圍: " << block_num << std::endl;
        return false;
    }

    size_t offset = static_cast<size_t>(block_num) * block_size_;
    std::memcpy(buffer, memory_.data() + offset, block_size_);
    return true;
}

bool MemoryDiskEmulator::writeBlock(uint32_t block_num, const void* buffer) {
    if (block_num >= total_blocks_) {
        std::cerr << "寫入區塊超出範圍: " << block_num << std::endl;
        return false;
    }

    size_t offset = static_cast<size_t>(block_num) * block_size_;
    std::memcpy(memory_.data() + offset, buffer, block_size_);
    return true;
}
