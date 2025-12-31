#include "FileSystem.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cstring>

FileSystem::FileSystem() : mounted_(false) {
}

FileSystem::~FileSystem() {
    unmount();
}

bool FileSystem::format(const std::string& disk_path, bool use_memory) {
    // 創建儲存後端
    if (use_memory) {
        disk_ = std::make_unique<MemoryDiskEmulator>(Config::TOTAL_BLOCKS, Config::BLOCK_SIZE);
    } else {
        auto file_disk = std::make_unique<FileDiskEmulator>(disk_path, Config::TOTAL_BLOCKS, Config::BLOCK_SIZE);
        if (!file_disk->initialize()) {
            return false;
        }
        disk_ = std::move(file_disk);
    }

    // 創建並初始化 superblock
    Superblock sb;
    if (!disk_->writeBlock(Config::SUPERBLOCK_BLOCK, &sb)) {
        std::cerr << "寫入 superblock 失敗" << std::endl;
        return false;
    }

    // 創建 bitmap
    inode_bitmap_ = std::make_unique<Bitmap>(disk_.get(), Config::INODE_BITMAP_START, 
                                              Config::INODE_BITMAP_BLOCKS, Config::TOTAL_INODES);
    block_bitmap_ = std::make_unique<Bitmap>(disk_.get(), Config::BLOCK_BITMAP_START, 
                                              Config::BLOCK_BITMAP_BLOCKS, 
                                              Config::TOTAL_BLOCKS - Config::DATA_BLOCKS_START);

    // 保存空的 bitmap
    inode_bitmap_->save();
    block_bitmap_->save();

    // 創建 inode manager
    inode_manager_ = std::make_unique<InodeManager>(disk_.get(), inode_bitmap_.get(), block_bitmap_.get());

    // 創建根目錄
    int root_inode = inode_manager_->allocateInode(FileType::DIRECTORY);
    if (root_inode != Config::ROOT_INODE) {
        std::cerr << "根目錄 inode 編號錯誤" << std::endl;
        return false;
    }

    // 初始化根目錄：添加 "." 和 ".." 項目
    Inode root;
    inode_manager_->readInode(Config::ROOT_INODE, root);

    // 分配第一個區塊給根目錄
    int block_num = inode_manager_->getBlockNumber(root, 0, true);
    if (block_num <= 0) {
        std::cerr << "無法為根目錄分配區塊" << std::endl;
        return false;
    }
    
    // 關鍵修復：立即寫回根目錄 inode
    inode_manager_->writeInode(Config::ROOT_INODE, root);

    // 創建目錄項目（使用完整區塊大小以避免未初始化內存問題）
    uint32_t entries_per_block = Config::BLOCK_SIZE / sizeof(DirectoryEntry);
    std::vector<DirectoryEntry> entries(entries_per_block);  // 分配完整區塊
    entries[0] = DirectoryEntry(Config::ROOT_INODE, ".");
    entries[1] = DirectoryEntry(Config::ROOT_INODE, "..");
    // 其餘項目已由默認構造函數初始化為 invalid

    // 寫入目錄項目
    disk_->writeBlock(block_num, entries.data());
    root.size = sizeof(DirectoryEntry) * 2;
    inode_manager_->writeInode(Config::ROOT_INODE, root);

    // 保存 bitmap
    inode_bitmap_->save();
    block_bitmap_->save();

    mounted_ = true;
    std::cout << "檔案系統格式化完成" << std::endl;
    return true;
}

bool FileSystem::mount(const std::string& disk_path) {
    // 打開現有的檔案系統
    disk_ = std::make_unique<FileDiskEmulator>(disk_path, Config::TOTAL_BLOCKS, Config::BLOCK_SIZE);

    // 讀取 superblock
    Superblock sb;
    if (!disk_->readBlock(Config::SUPERBLOCK_BLOCK, &sb)) {
        std::cerr << "讀取 superblock 失敗" << std::endl;
        return false;
    }

    // 驗證魔數
    if (sb.magic != Config::MAGIC_NUMBER) {
        std::cerr << "無效的檔案系統" << std::endl;
        return false;
    }

    // 載入 bitmap
    inode_bitmap_ = std::make_unique<Bitmap>(disk_.get(), Config::INODE_BITMAP_START, 
                                              Config::INODE_BITMAP_BLOCKS, Config::TOTAL_INODES);
    block_bitmap_ = std::make_unique<Bitmap>(disk_.get(), Config::BLOCK_BITMAP_START, 
                                              Config::BLOCK_BITMAP_BLOCKS, 
                                              Config::TOTAL_BLOCKS - Config::DATA_BLOCKS_START);

    if (!inode_bitmap_->load() || !block_bitmap_->load()) {
        std::cerr << "載入 bitmap 失敗" << std::endl;
        return false;
    }

    // 創建 inode manager
    inode_manager_ = std::make_unique<InodeManager>(disk_.get(), inode_bitmap_.get(), block_bitmap_.get());
    inode_manager_->loadSuperblock();

    mounted_ = true;
    std::cout << "檔案系統掛載成功" << std::endl;
    return true;
}

void FileSystem::unmount() {
    if (mounted_) {
        inode_bitmap_->save();
        block_bitmap_->save();
        mounted_ = false;
        std::cout << "檔案系統已卸載" << std::endl;
    }
}

std::vector<std::string> FileSystem::splitPath(const std::string& path) {
    std::vector<std::string> components;
    std::stringstream ss(path);
    std::string component;

    while (std::getline(ss, component, '/')) {
        if (!component.empty() && component != ".") {
            components.push_back(component);
        }
    }

    return components;
}

int FileSystem::resolvePath(const std::string& path) {
    if (!mounted_) {
        std::cerr << "檔案系統未掛載" << std::endl;
        return -1;
    }

    if (path == "/" || path.empty()) {
        return Config::ROOT_INODE;
    }

    std::vector<std::string> components = splitPath(path);
    uint32_t current_inode = Config::ROOT_INODE;

    for (const auto& comp : components) {
        int next_inode = findInDirectory(current_inode, comp);
        if (next_inode < 0) {
            return -1;  // 路徑不存在
        }
        current_inode = next_inode;
    }

    return current_inode;
}

int FileSystem::findInDirectory(uint32_t dir_inode_num, const std::string& name) {
    Inode dir_inode;
    if (!inode_manager_->readInode(dir_inode_num, dir_inode)) {
        return -1;
    }

    if (!dir_inode.isDirectory()) {
        return -1;
    }

    // 計算目錄項目數量
    uint32_t num_entries = dir_inode.size / sizeof(DirectoryEntry);
    uint32_t entries_per_block = Config::BLOCK_SIZE / sizeof(DirectoryEntry);

    for (uint32_t i = 0; i < num_entries; ++i) {
        uint32_t block_index = i / entries_per_block;
        uint32_t entry_offset = i % entries_per_block;

        int block_num = inode_manager_->getBlockNumber(dir_inode, block_index, false);
        if (block_num <= 0) continue;

        std::vector<DirectoryEntry> entries(entries_per_block);
        disk_->readBlock(block_num, entries.data());

        if (entries[entry_offset].valid && std::string(entries[entry_offset].name) == name) {
            return entries[entry_offset].inode_number;
        }
    }

    return -1;  // 未找到
}

bool FileSystem::addToDirectory(uint32_t dir_inode_num, const std::string& name, uint32_t inode_num) {
    Inode dir_inode;
    if (!inode_manager_->readInode(dir_inode_num, dir_inode)) {
        return false;
    }

    if (!dir_inode.isDirectory()) {
        return false;
    }

    // 檢查是否已存在
    if (findInDirectory(dir_inode_num, name) >= 0) {
        std::cerr << "項目已存在: " << name << std::endl;
        return false;
    }

    // 找到空閒位置或添加新項目
    uint32_t num_entries = dir_inode.size / sizeof(DirectoryEntry);
    uint32_t entries_per_block = Config::BLOCK_SIZE / sizeof(DirectoryEntry);
    
    uint32_t block_index = num_entries / entries_per_block;
    uint32_t entry_offset = num_entries % entries_per_block;

    int block_num = inode_manager_->getBlockNumber(dir_inode, block_index, true);
    if (block_num <= 0) {
        std::cerr << "無法分配目錄區塊" << std::endl;
        return false;
    }
    
    // 關鍵修復：如果分配了新區塊，inode 已被修改，需要立即寫回
    // 這確保 block_count 和區塊指針的更新被保存
    inode_manager_->writeInode(dir_inode_num, dir_inode);

    // 讀取區塊
    std::vector<DirectoryEntry> entries(entries_per_block);
    disk_->readBlock(block_num, entries.data());

    // 添加新項目
    entries[entry_offset] = DirectoryEntry(inode_num, name.c_str());

    // 寫回區塊
    disk_->writeBlock(block_num, entries.data());

    // 更新目錄大小
    dir_inode.size += sizeof(DirectoryEntry);
    inode_manager_->writeInode(dir_inode_num, dir_inode);

    return true;
}

bool FileSystem::removeFromDirectory(uint32_t dir_inode_num, const std::string& name) {
    Inode dir_inode;
    if (!inode_manager_->readInode(dir_inode_num, dir_inode)) {
        return false;
    }

    if (!dir_inode.isDirectory()) {
        return false;
    }

    uint32_t num_entries = dir_inode.size / sizeof(DirectoryEntry);
    uint32_t entries_per_block = Config::BLOCK_SIZE / sizeof(DirectoryEntry);

    for (uint32_t i = 0; i < num_entries; ++i) {
        uint32_t block_index = i / entries_per_block;
        uint32_t entry_offset = i % entries_per_block;

        int block_num = inode_manager_->getBlockNumber(dir_inode, block_index, false);
        if (block_num <= 0) continue;

        std::vector<DirectoryEntry> entries(entries_per_block);
        disk_->readBlock(block_num, entries.data());

        if (entries[entry_offset].valid && std::string(entries[entry_offset].name) == name) {
            // 標記為無效
            entries[entry_offset].valid = false;
            disk_->writeBlock(block_num, entries.data());
            return true;
        }
    }

    return false;
}

bool FileSystem::isDirectoryEmpty(uint32_t dir_inode_num) {
    Inode dir_inode;
    if (!inode_manager_->readInode(dir_inode_num, dir_inode)) {
        return false;
    }

    uint32_t num_entries = dir_inode.size / sizeof(DirectoryEntry);
    
    // 只有 "." 和 ".." 的目錄被視為空
    uint32_t valid_count = 0;
    uint32_t entries_per_block = Config::BLOCK_SIZE / sizeof(DirectoryEntry);

    for (uint32_t i = 0; i < num_entries; ++i) {
        uint32_t block_index = i / entries_per_block;
        uint32_t entry_offset = i % entries_per_block;

        int block_num = inode_manager_->getBlockNumber(dir_inode, block_index, false);
        if (block_num <= 0) continue;

        std::vector<DirectoryEntry> entries(entries_per_block);
        disk_->readBlock(block_num, entries.data());

        if (entries[entry_offset].valid) {
            std::string name = entries[entry_offset].name;
            if (name != "." && name != "..") {
                valid_count++;
            }
        }
    }

    return valid_count == 0;
}

bool FileSystem::create(const std::string& path, bool is_directory) {
    if (!mounted_) return false;

    // 分割路徑
    auto components = splitPath(path);
    if (components.empty()) return false;

    std::string filename = components.back();
    components.pop_back();

    // 找到父目錄
    uint32_t parent_inode = Config::ROOT_INODE;
    for (const auto& comp : components) {
        int next = findInDirectory(parent_inode, comp);
        if (next < 0) {
            std::cerr << "父目錄不存在: " << comp << std::endl;
            return false;
        }
        parent_inode = next;
    }

    // 分配新 inode
    FileType type = is_directory ? FileType::DIRECTORY : FileType::REGULAR;
    int new_inode = inode_manager_->allocateInode(type);
    if (new_inode < 0) {
        return false;
    }

    // 如果是目錄，初始化 "." 和 ".."
    if (is_directory) {
        Inode dir;
        inode_manager_->readInode(new_inode, dir);

        int block_num = inode_manager_->getBlockNumber(dir, 0, true);
        if (block_num <= 0) {
            inode_manager_->freeInode(new_inode);
            return false;
        }
        
        // 關鍵修復：寫回 inode 以保存區塊分配的更新
        inode_manager_->writeInode(new_inode, dir);

        // 創建目錄項目（使用完整區塊）
        uint32_t entries_per_block = Config::BLOCK_SIZE / sizeof(DirectoryEntry);
        std::vector<DirectoryEntry> entries(entries_per_block);
        entries[0] = DirectoryEntry(new_inode, ".");
        entries[1] = DirectoryEntry(parent_inode, "..");
        // 其餘項目由默認構造函數初始化為 invalid

        disk_->writeBlock(block_num, entries.data());
        dir.size = sizeof(DirectoryEntry) * 2;
        inode_manager_->writeInode(new_inode, dir);
    }

    // 添加到父目錄
    if (!addToDirectory(parent_inode, filename, new_inode)) {
        inode_manager_->freeInode(new_inode);
        return false;
    }

    inode_bitmap_->save();
    block_bitmap_->save();

    std::cout << (is_directory ? "目錄" : "檔案") << "創建成功: " << path << std::endl;
    return true;
}

bool FileSystem::mkdir(const std::string& path) {
    return create(path, true);
}

bool FileSystem::remove(const std::string& path) {
    if (!mounted_) return false;

    int inode_num = resolvePath(path);
    if (inode_num < 0) {
        std::cerr << "路徑不存在: " << path << std::endl;
        return false;
    }

    // 不能刪除根目錄
    if (inode_num == static_cast<int>(Config::ROOT_INODE)) {
        std::cerr << "無法刪除根目錄" << std::endl;
        return false;
    }

    Inode inode;
    inode_manager_->readInode(inode_num, inode);

    // 如果是目錄，檢查是否為空
    if (inode.isDirectory() && !isDirectoryEmpty(inode_num)) {
        std::cerr << "目錄不為空: " << path << std::endl;
        return false;
    }

    // 從父目錄移除
    auto components = splitPath(path);
    std::string filename = components.back();
    components.pop_back();

    uint32_t parent_inode = Config::ROOT_INODE;
    for (const auto& comp : components) {
        parent_inode = findInDirectory(parent_inode, comp);
    }

    removeFromDirectory(parent_inode, filename);

    // 釋放 inode
    inode_manager_->freeInode(inode_num);

    inode_bitmap_->save();
    block_bitmap_->save();

    std::cout << "刪除成功: " << path << std::endl;
    return true;
}

bool FileSystem::rmdir(const std::string& path) {
    return remove(path);
}

std::vector<std::string> FileSystem::list(const std::string& path) {
    std::vector<std::string> result;
    
    if (!mounted_) return result;

    int inode_num = resolvePath(path);
    if (inode_num < 0) {
        std::cerr << "路徑不存在: " << path << std::endl;
        return result;
    }

    Inode inode;
    if (!inode_manager_->readInode(inode_num, inode)) {
        return result;
    }

    if (!inode.isDirectory()) {
        std::cerr << "不是目錄: " << path << std::endl;
        return result;
    }

    uint32_t num_entries = inode.size / sizeof(DirectoryEntry);
    uint32_t entries_per_block = Config::BLOCK_SIZE / sizeof(DirectoryEntry);

    for (uint32_t i = 0; i < num_entries; ++i) {
        uint32_t block_index = i / entries_per_block;
        uint32_t entry_offset = i % entries_per_block;

        int block_num = inode_manager_->getBlockNumber(inode, block_index, false);
        if (block_num <= 0) continue;

        std::vector<DirectoryEntry> entries(entries_per_block);
        disk_->readBlock(block_num, entries.data());

        if (entries[entry_offset].valid) {
            result.push_back(entries[entry_offset].name);
        }
    }

    return result;
}

int FileSystem::read(const std::string& path, void* buffer, uint32_t size, uint32_t offset) {
    if (!mounted_) return -1;

    int inode_num = resolvePath(path);
    if (inode_num < 0) {
        std::cerr << "檔案不存在: " << path << std::endl;
        return -1;
    }

    Inode inode;
    if (!inode_manager_->readInode(inode_num, inode)) {
        return -1;
    }

    if (!inode.isFile()) {
        std::cerr << "不是檔案: " << path << std::endl;
        return -1;
    }

    // 調整讀取大小
    if (offset >= inode.size) return 0;
    if (offset + size > inode.size) {
        size = inode.size - offset;
    }

    uint32_t bytes_read = 0;
    uint8_t* buf = static_cast<uint8_t*>(buffer);

    while (bytes_read < size) {
        uint32_t block_index = (offset + bytes_read) / Config::BLOCK_SIZE;
        uint32_t block_offset = (offset + bytes_read) % Config::BLOCK_SIZE;
        uint32_t bytes_to_read = std::min(size - bytes_read, Config::BLOCK_SIZE - block_offset);

        int block_num = inode_manager_->getBlockNumber(inode, block_index, false);
        if (block_num <= 0) break;

        std::vector<uint8_t> block_buffer(Config::BLOCK_SIZE);
        disk_->readBlock(block_num, block_buffer.data());

        std::memcpy(buf + bytes_read, block_buffer.data() + block_offset, bytes_to_read);
        bytes_read += bytes_to_read;
    }

    return bytes_read;
}

int FileSystem::write(const std::string& path, const void* buffer, uint32_t size, uint32_t offset) {
    if (!mounted_) return -1;

    int inode_num = resolvePath(path);
    if (inode_num < 0) {
        std::cerr << "檔案不存在: " << path << std::endl;
        return -1;
    }

    Inode inode;
    if (!inode_manager_->readInode(inode_num, inode)) {
        return -1;
    }

    if (!inode.isFile()) {
        std::cerr << "不是檔案: " << path << std::endl;
        return -1;
    }

    uint32_t bytes_written = 0;
    const uint8_t* buf = static_cast<const uint8_t*>(buffer);

    while (bytes_written < size) {
        uint32_t block_index = (offset + bytes_written) / Config::BLOCK_SIZE;
        uint32_t block_offset = (offset + bytes_written) % Config::BLOCK_SIZE;
        uint32_t bytes_to_write = std::min(size - bytes_written, Config::BLOCK_SIZE - block_offset);

        int block_num = inode_manager_->getBlockNumber(inode, block_index, true);
        if (block_num <= 0) {
            std::cerr << "無法分配區塊" << std::endl;
            break;
        }
        
        // 關鍵修復：寫回 inode（如果分配了新區塊）
        inode_manager_->writeInode(inode_num, inode);

        std::vector<uint8_t> block_buffer(Config::BLOCK_SIZE);
        
        // 如果不是寫入整個區塊，需要先讀取
        if (block_offset != 0 || bytes_to_write != Config::BLOCK_SIZE) {
            disk_->readBlock(block_num, block_buffer.data());
        }

        std::memcpy(block_buffer.data() + block_offset, buf + bytes_written, bytes_to_write);
        disk_->writeBlock(block_num, block_buffer.data());

        bytes_written += bytes_to_write;
    }

    // 更新檔案大小
    if (offset + bytes_written > inode.size) {
        inode.size = offset + bytes_written;
        inode_manager_->writeInode(inode_num, inode);
    }

    block_bitmap_->save();

    return bytes_written;
}

bool FileSystem::stat(const std::string& path, Inode& inode) {
    if (!mounted_) return false;

    int inode_num = resolvePath(path);
    if (inode_num < 0) return false;

    return inode_manager_->readInode(inode_num, inode);
}

void FileSystem::printInfo() {
    if (!mounted_) {
        std::cout << "檔案系統未掛載" << std::endl;
        return;
    }

    const Superblock& sb = inode_manager_->getSuperblock();
    
    std::cout << "\n========== 檔案系統資訊 ==========" << std::endl;
    std::cout << "總區塊數: " << sb.total_blocks << std::endl;
    std::cout << "空閒區塊數: " << sb.free_blocks << std::endl;
    std::cout << "總 Inode 數: " << sb.total_inodes << std::endl;
    std::cout << "空閒 Inode 數: " << sb.free_inodes << std::endl;
    std::cout << "區塊大小: " << sb.block_size << " bytes" << std::endl;
    std::cout << "使用率: " << std::fixed << std::setprecision(2) 
              << (100.0 * (sb.total_blocks - sb.free_blocks) / sb.total_blocks) << "%" << std::endl;
    std::cout << "================================\n" << std::endl;
}
