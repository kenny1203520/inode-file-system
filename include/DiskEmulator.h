#ifndef DISK_EMULATOR_H
#define DISK_EMULATOR_H

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>

// 抽象存儲介面 - 允許輕鬆切換硬碟/記憶體實作
class DiskEmulator {
public:
    virtual ~DiskEmulator() = default;
    
    // 讀取一個區塊
    virtual bool readBlock(uint32_t block_num, void* buffer) = 0;
    
    // 寫入一個區塊
    virtual bool writeBlock(uint32_t block_num, const void* buffer) = 0;
    
    // 獲取總區塊數
    virtual uint32_t getTotalBlocks() const = 0;
    
    // 獲取區塊大小
    virtual uint32_t getBlockSize() const = 0;
};

// 檔案型存儲實作（使用實際硬碟檔案模擬）
class FileDiskEmulator : public DiskEmulator {
private:
    std::string disk_path_;
    uint32_t total_blocks_;
    uint32_t block_size_;
    mutable std::fstream file_;

public:
    FileDiskEmulator(const std::string& path, uint32_t total_blocks, uint32_t block_size);
    ~FileDiskEmulator() override;
    
    bool readBlock(uint32_t block_num, void* buffer) override;
    bool writeBlock(uint32_t block_num, const void* buffer) override;
    uint32_t getTotalBlocks() const override { return total_blocks_; }
    uint32_t getBlockSize() const override { return block_size_; }
    
    // 初始化磁碟檔案
    bool initialize();
};

// 記憶體型存儲實作（純記憶體模擬，用於未來切換）
class MemoryDiskEmulator : public DiskEmulator {
private:
    std::vector<uint8_t> memory_;
    uint32_t total_blocks_;
    uint32_t block_size_;

public:
    MemoryDiskEmulator(uint32_t total_blocks, uint32_t block_size);
    ~MemoryDiskEmulator() override = default;
    
    bool readBlock(uint32_t block_num, void* buffer) override;
    bool writeBlock(uint32_t block_num, const void* buffer) override;
    uint32_t getTotalBlocks() const override { return total_blocks_; }
    uint32_t getBlockSize() const override { return block_size_; }
};

#endif // DISK_EMULATOR_H
