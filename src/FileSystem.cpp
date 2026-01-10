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
    // 1. 初始化磁碟 (16MB)
    if (use_memory) {
        disk_ = std::make_unique<MemoryDiskEmulator>(Config::TOTAL_BLOCKS, Config::BLOCK_SIZE);
    } else {
        auto file_disk = std::make_unique<FileDiskEmulator>(disk_path, Config::TOTAL_BLOCKS, Config::BLOCK_SIZE);
        if (!file_disk->initialize()) {
            return false;
        }
        disk_ = std::move(file_disk);
    }

    // 2. 寫入 Superblock（區塊 0）
    Superblock sb;
    // 注意：Superblock 現在由建構子使用預設常數進行 POD 初始化
    if (!disk_->writeBlock(Config::SUPERBLOCK_BLOCK, &sb)) {
        std::cerr << "寫入 superblock 失敗" << std::endl;
        return false;
    }

    // 3. 初始化 Bitmap
    // Inode Bitmap 在區塊 1，Block Bitmap 在區塊 2
    inode_bitmap_ = std::make_unique<Bitmap>(disk_.get(), Config::INODE_BITMAP_BLOCK, 
                                              1, Config::TOTAL_INODES);
    block_bitmap_ = std::make_unique<Bitmap>(disk_.get(), Config::BLOCK_BITMAP_BLOCK, 
                                              1, Config::TOTAL_BLOCKS - Config::DATA_BLOCKS_START);

    // 初始狀態：全部為 0（空閒）
    // 等等，Block Bitmap 邏輯：
    // 如果我們將 0 映射到 DataBlockStart，那麼我們不需要在此 bitmap 中標記元數據區塊為已使用
    // 因為此 bitmap 只涵蓋資料區域。
    // 但是，主規格說：「初始化 Bitmaps... 將區塊 0~6 標記為 1（已使用）」。
    // 如果 Block Bitmap 涵蓋整個磁碟（0 到 4096），那麼我們將 0-6 標記為已使用。
    // 如果它只涵蓋資料區域，那麼 0 代表區塊 7。
    // 讓我們堅持「Bitmap 只追蹤資料區塊」以簡化邏輯（1 位 = 1 個可分配的資料區塊）。
    // 元數據區塊隱含地「已使用」，因為它們不在可分配範圍內。
    // 但為了嚴格遵循主規格：「區塊 2：Block Bitmap 追蹤 4096 個資料區塊使用情況。」
    // 「區塊 0~6... 標記為 1」。
    // 這意味著 bitmap 涵蓋 0 到 4095。
    // 讓我們調整邏輯：Bitmap 涵蓋 0..TOTAL_BLOCKS。
    // 因此傳遞給 Bitmap 建構子的 `num_blocks` 應該足以涵蓋 TOTAL_BLOCKS 區塊。
    // 4096 位 = 512 位元組。1 個區塊就足夠了。
    
    // 使用完整範圍支援重新初始化 Bitmaps：
    block_bitmap_ = std::make_unique<Bitmap>(disk_.get(), Config::BLOCK_BITMAP_BLOCK, 
                                              1, Config::TOTAL_BLOCKS); 
    
    // 手動標記區塊 0-6 為已使用
    for (uint32_t i = 0; i < Config::DATA_BLOCKS_START; ++i) {
        block_bitmap_->setBit(i);
    }

    // 標記根 Inode（0）為已使用
    inode_bitmap_->setBit(Config::ROOT_INODE);

    // 儲存已初始化的 bitmaps
    inode_bitmap_->save();
    block_bitmap_->save();

    // 4. 建立 Inode Manager
    inode_manager_ = std::make_unique<InodeManager>(disk_.get(), inode_bitmap_.get(), block_bitmap_.get());

    // 5. 初始化根 Inode
    // 我們手動寫入它，因為 allocateInode 可能會搜尋 0，而我們已經設定了位元 0。
    // 或者我們使用 allocateInode 邏輯，但需要確保它選擇 0。
    // 由於我們設定了位元 0，allocateInode 會選擇 1。
    // 所以讓我們手動初始化根 Inode。
    Inode root;
    root.inode_num = Config::ROOT_INODE;
    root.file_type = FileType::DIRECTORY;
    root.size = 2 * sizeof(DirectoryEntry); // . 和 ..
    for(int i=0; i<Config::DIRECT_BLOCKS; ++i) root.direct_blks[i] = 0;

    // 為根目錄分配資料區塊
    // 我們需要驗證 allocateBlock 返回哪個區塊。它應該是第一個空閒的。
    // 由於 0-6 已使用，它應該返回 7。
    int root_block = inode_manager_->allocateBlock();
    if (root_block != static_cast<int>(Config::DATA_BLOCKS_START)) {
        // 警告但繼續
        // std::cout << "根區塊分配在 " << root_block << std::endl;
    }
    root.direct_blks[0] = root_block;

    // 寫入根 Inode
    inode_manager_->writeInode(Config::ROOT_INODE, root);

    // 6. 初始化根目錄項目
    std::vector<DirectoryEntry> entries(Config::DIR_ENTRIES_PER_BLOCK);
    // 將所有項目初始化為無效
    for(auto& e : entries) e.inode_number = 0xFFFFFFFF; // 無效

    entries[0] = DirectoryEntry(Config::ROOT_INODE, ".");
    entries[1] = DirectoryEntry(Config::ROOT_INODE, "..");
    
    disk_->writeBlock(root_block, entries.data());

    // 最終儲存
    inode_bitmap_->save();
    block_bitmap_->save();

    mounted_ = true;
    std::cout << "檔案系統格式化完成 (16MB, 4KB blocks)" << std::endl;
    return true;
}

bool FileSystem::mount(const std::string& disk_path) {
    disk_ = std::make_unique<FileDiskEmulator>(disk_path, Config::TOTAL_BLOCKS, Config::BLOCK_SIZE);

    Superblock sb;
    if (!disk_->readBlock(Config::SUPERBLOCK_BLOCK, &sb)) {
        std::cerr << "讀取 superblock 失敗" << std::endl;
        return false;
    }

    if (sb.magic != Config::MAGIC_NUMBER) {
        std::cerr << "無效的檔案系統 (Magic Mismatch)" << std::endl;
        return false;
    }

    inode_bitmap_ = std::make_unique<Bitmap>(disk_.get(), Config::INODE_BITMAP_BLOCK, 
                                              1, Config::TOTAL_INODES);
    block_bitmap_ = std::make_unique<Bitmap>(disk_.get(), Config::BLOCK_BITMAP_BLOCK, 
                                              1, Config::TOTAL_BLOCKS);

    if (!inode_bitmap_->load() || !block_bitmap_->load()) {
        std::cerr << "載入 bitmap 失敗" << std::endl;
        return false;
    }

    inode_manager_ = std::make_unique<InodeManager>(disk_.get(), inode_bitmap_.get(), block_bitmap_.get());
    
    // 注意：如果我們已經讀取了 Superblock，InodeManager loadSuperblock 是多餘的，但保持同步
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

// 路徑分割輔助函數
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
    if (!mounted_) return -1;
    if (path == "/" || path.empty()) return Config::ROOT_INODE;

    std::vector<std::string> components = splitPath(path);
    uint32_t current_inode = Config::ROOT_INODE;

    for (const auto& comp : components) {
        int next_inode = findInDirectory(current_inode, comp);
        if (next_inode < 0) return -1;
        current_inode = next_inode;
    }
    return current_inode;
}

int FileSystem::findInDirectory(uint32_t dir_inode_num, const std::string& name) {
    Inode dir_inode;
    if (!inode_manager_->readInode(dir_inode_num, dir_inode)) return -1;
    if (!dir_inode.isDirectory()) return -1;

    // 掃描所有直接區塊
    for (int i = 0; i < Config::DIRECT_BLOCKS; ++i) {
        uint32_t block_num = dir_inode.direct_blks[i];
        if (block_num == 0) continue;

        std::vector<DirectoryEntry> entries(Config::DIR_ENTRIES_PER_BLOCK);
        disk_->readBlock(block_num, entries.data());

        for (const auto& entry : entries) {
            if (entry.inode_number != 0xFFFFFFFF && std::strncmp(entry.name, name.c_str(), Config::MAX_FILENAME_LENGTH) == 0) {
                 return entry.inode_number;
            }
        }
    }
    return -1;
}

bool FileSystem::addToDirectory(uint32_t dir_inode_num, const std::string& name, uint32_t inode_num) {
    if (findInDirectory(dir_inode_num, name) >= 0) {
        std::cerr << "File exists: " << name << std::endl;
        return false;
    }

    Inode dir_inode;
    inode_manager_->readInode(dir_inode_num, dir_inode);
    if (!dir_inode.isDirectory()) return false;

    // 尋找空的槽位
    for (int i = 0; i < Config::DIRECT_BLOCKS; ++i) {
        bool allocate = false;
        if (dir_inode.direct_blks[i] == 0) {
            // 分配新區塊
            int new_blk = inode_manager_->allocateBlock();
            if (new_blk < 0) return false;
            dir_inode.direct_blks[i] = new_blk;
            // 使用無效項目初始化新區塊
            std::vector<DirectoryEntry> empty_entries(Config::DIR_ENTRIES_PER_BLOCK); 
            // 預設建構子將 inode 設為 0xFFFFFFFF
            disk_->writeBlock(new_blk, empty_entries.data());
            
            inode_manager_->writeInode(dir_inode_num, dir_inode);
            allocate = true;
        }

        uint32_t block_num = dir_inode.direct_blks[i];
        std::vector<DirectoryEntry> entries(Config::DIR_ENTRIES_PER_BLOCK);
        disk_->readBlock(block_num, entries.data());

        for (auto& entry : entries) {
            if (entry.inode_number == 0xFFFFFFFF) {
                // 找到空槽位
                entry = DirectoryEntry(inode_num, name.c_str());
                disk_->writeBlock(block_num, entries.data());
                
                // 更新大小以反映內容？還是保持大致相容。
                // 規格說大小以位元組為單位。嚴格來說，大小應該涵蓋已使用的項目？
                // 還是只是邏輯大小？讓我們在需要時更新大小，但對於目錄，通常 size = 已使用區塊 * 4096 或直接忽略大小。
                // 或 size = 項目數量 * 32。
                // 讓我們為每個新增的項目增加 32 位元組？
                dir_inode.size += sizeof(DirectoryEntry);
                inode_manager_->writeInode(dir_inode_num, dir_inode);
                return true;
            }
        }
    }
    
    std::cerr << "Directory full" << std::endl;
    return false;
}

bool FileSystem::removeFromDirectory(uint32_t dir_inode_num, const std::string& name) {
    Inode dir_inode;
    inode_manager_->readInode(dir_inode_num, dir_inode);
    
    for (int i = 0; i < Config::DIRECT_BLOCKS; ++i) {
        uint32_t block_num = dir_inode.direct_blks[i];
        if (block_num == 0) continue;

        std::vector<DirectoryEntry> entries(Config::DIR_ENTRIES_PER_BLOCK);
        disk_->readBlock(block_num, entries.data());
        bool changed = false;

        for (auto& entry : entries) {
            if (entry.inode_number != 0xFFFFFFFF && std::strncmp(entry.name, name.c_str(), Config::MAX_FILENAME_LENGTH) == 0) {
                entry.inode_number = 0xFFFFFFFF; // 標記為無效
                changed = true;
                break;
            }
        }

        if (changed) {
            disk_->writeBlock(block_num, entries.data());
            dir_inode.size -= sizeof(DirectoryEntry);
            inode_manager_->writeInode(dir_inode_num, dir_inode);
            return true;
        }
    }
    return false;
}

bool FileSystem::isDirectoryEmpty(uint32_t dir_inode_num) {
    Inode dir_inode;
    inode_manager_->readInode(dir_inode_num, dir_inode);
    
    for (int i = 0; i < Config::DIRECT_BLOCKS; ++i) {
        uint32_t block_num = dir_inode.direct_blks[i];
        if (block_num == 0) continue;

        std::vector<DirectoryEntry> entries(Config::DIR_ENTRIES_PER_BLOCK);
        disk_->readBlock(block_num, entries.data());

        for (const auto& entry : entries) {
            if (entry.inode_number == 0xFFFFFFFF) continue;
            std::string name = entry.name;
            if (name != "." && name != "..") return false;
        }
    }
    return true;
}

bool FileSystem::create(const std::string& path, bool is_directory) {
    if (!mounted_) return false;
    auto components = splitPath(path);
    if (components.empty()) return false;
    
    std::string filename = components.back();
    components.pop_back();

    uint32_t parent = Config::ROOT_INODE;
    for (const auto& c : components) {
        int next = findInDirectory(parent, c);
        if (next < 0) {
            std::cerr << "Parent not found: " << c << std::endl;
            return false;
        }
        parent = next;
    }

    FileType type = is_directory ? FileType::DIRECTORY : FileType::REGULAR;
    int new_inode = inode_manager_->allocateInode(type);
    if (new_inode < 0) return false;

    if (is_directory) {
        // 初始化 . 和 ..
        if(!addToDirectory(new_inode, ".", new_inode) || 
           !addToDirectory(new_inode, "..", parent)) {
             // 回滾？
             return false;
        }
    }

    if (!addToDirectory(parent, filename, new_inode)) {
        // 回滾
        inode_manager_->freeInode(new_inode);
        return false;
    }

    inode_bitmap_->save();
    block_bitmap_->save();
    return true;
}

bool FileSystem::mkdir(const std::string& path) {
    return create(path, true);
}

bool FileSystem::remove(const std::string& path) {
    int inode_num = resolvePath(path);
    if (inode_num < 0) {
        std::cerr << "Not found" << std::endl;
        return false;
    }
    if (inode_num == (int)Config::ROOT_INODE) {
        std::cerr << "Cannot delete root" << std::endl;
        return false;
    }

    Inode inode;
    inode_manager_->readInode(inode_num, inode);
    if (inode.isDirectory() && !isDirectoryEmpty(inode_num)) {
        std::cerr << "Directory not empty" << std::endl;
        return false;
    }

    // 從父目錄取消連結
    auto components = splitPath(path);
    std::string filename = components.back();
    components.pop_back();
    uint32_t parent = Config::ROOT_INODE;
    for (const auto& c : components) parent = findInDirectory(parent, c);
    
    removeFromDirectory(parent, filename);
    inode_manager_->freeInode(inode_num);
    
    inode_bitmap_->save();
    block_bitmap_->save();
    return true;
}

bool FileSystem::rmdir(const std::string& path) {
    return remove(path);
}

std::vector<std::string> FileSystem::list(const std::string& path) {
    std::vector<std::string> result;
    int inode_num = resolvePath(path);
    if (inode_num < 0) return result;

    Inode inode;
    inode_manager_->readInode(inode_num, inode);
    if (!inode.isDirectory()) return result;

    for(int i=0; i<Config::DIRECT_BLOCKS; ++i) {
        uint32_t blk = inode.direct_blks[i];
        if (blk == 0) continue;
        std::vector<DirectoryEntry> entries(Config::DIR_ENTRIES_PER_BLOCK);
        disk_->readBlock(blk, entries.data());
        for(const auto& e : entries) {
            if (e.inode_number != 0xFFFFFFFF) {
                result.push_back(e.name);
            }
        }
    }
    return result;
}

// 檔案的讀取/寫入
int FileSystem::read(const std::string& path, void* buffer, uint32_t size, uint32_t offset) {
    int inode_num = resolvePath(path);
    if (inode_num < 0) return -1;
    Inode inode;
    inode_manager_->readInode(inode_num, inode);
    if (!inode.isFile()) return -1;

    if (offset >= inode.size) return 0;
    if (offset + size > inode.size) size = inode.size - offset;

    uint32_t bytes_read = 0;
    uint8_t* out = (uint8_t*)buffer;

    while(bytes_read < size) {
        uint32_t logical_idx = (offset + bytes_read) / Config::BLOCK_SIZE;
        uint32_t blk_off = (offset + bytes_read) % Config::BLOCK_SIZE;
        uint32_t to_read = std::min(size - bytes_read, Config::BLOCK_SIZE - blk_off);

        int phys_blk = inode_manager_->getBlockNumber(inode, logical_idx, false);
        if (phys_blk <= 0) break; // 如果大小正確，應該不會發生

        std::vector<uint8_t> block_buf(Config::BLOCK_SIZE);
        disk_->readBlock(phys_blk, block_buf.data());
        std::memcpy(out + bytes_read, block_buf.data() + blk_off, to_read);
        bytes_read += to_read;
    }
    return bytes_read;
}

int FileSystem::write(const std::string& path, const void* buffer, uint32_t size, uint32_t offset) {
    int inode_num = resolvePath(path);
    if (inode_num < 0) return -1;
    Inode inode;
    inode_manager_->readInode(inode_num, inode);
    if (!inode.isFile()) return -1;

    uint32_t original_size = inode.size;

    // 如果寫入長度為 0，視為清空檔案/截斷
    if (size == 0) {
        inode_manager_->freeAllBlocks(inode);
        inode.size = offset; // 通常為 0
        inode_manager_->writeInode(inode_num, inode);
        block_bitmap_->save();
        return 0;
    }

    uint32_t bytes_written = 0;
    const uint8_t* in = (const uint8_t*)buffer;

    while(bytes_written < size) {
        uint32_t logical_idx = (offset + bytes_written) / Config::BLOCK_SIZE;
        uint32_t blk_off = (offset + bytes_written) % Config::BLOCK_SIZE;
        uint32_t to_write = std::min(size - bytes_written, Config::BLOCK_SIZE - blk_off);

        // 自動分配
        int phys_blk = inode_manager_->getBlockNumber(inode, logical_idx, true);
        if (phys_blk <= 0) {
            std::cerr << "No space or file too large" << std::endl;
            break;
        }
        inode_manager_->writeInode(inode_num, inode); // 更新 direct_blks

        std::vector<uint8_t> block_buf(Config::BLOCK_SIZE);
        if (blk_off != 0 || to_write < Config::BLOCK_SIZE) {
            disk_->readBlock(phys_blk, block_buf.data());
        }
        std::memcpy(block_buf.data() + blk_off, in + bytes_written, to_write);
        disk_->writeBlock(phys_blk, block_buf.data());

        bytes_written += to_write;
    }

    uint32_t new_size = offset + bytes_written;

    // 釋放多餘的區塊（處理縮小或清空檔案的情況）
    if (new_size < original_size) {
        uint32_t required_blocks = (new_size + Config::BLOCK_SIZE - 1) / Config::BLOCK_SIZE;
        for (uint32_t i = required_blocks; i < Config::DIRECT_BLOCKS; ++i) {
            if (inode.direct_blks[i] != 0) {
                inode_manager_->freeBlock(inode.direct_blks[i]);
                inode.direct_blks[i] = 0;
            }
        }
    }

    if (new_size != inode.size) {
        inode.size = new_size;
        inode_manager_->writeInode(inode_num, inode);
    }
    block_bitmap_->save();
    return bytes_written;
}

bool FileSystem::stat(const std::string& path, Inode& inode) {
    int inode_num = resolvePath(path);
    if (inode_num < 0) return false;
    return inode_manager_->readInode(inode_num, inode);
}

void FileSystem::printInfo() {
    if (!mounted_) {
        std::cout << "Not mounted" << std::endl;
        return;
    }
    // const Superblock& sb = inode_manager_->getSuperblock();
    // 從結構中移除了 free_blocks/free_inodes 欄位。
    // 如有需要，使用 bitmaps。
    uint32_t free_inodes = inode_bitmap_->countFree();
    uint32_t free_blocks = block_bitmap_->countFree();
    
    std::cout << "System Info: 16MB Disk, 4KB x 4096 blocks." << std::endl;
    std::cout << "Free Inodes: " << free_inodes << "/" << Config::TOTAL_INODES << std::endl;
    std::cout << "Free Blocks: " << free_blocks << "/" << Config::TOTAL_BLOCKS << std::endl;
}

bool FileSystem::copy(const std::string& src_path, const std::string& dest_path) {
    if (!mounted_) return false;

    // 1. 檢查來源檔案是否存在
    int src_inode_num = resolvePath(src_path);
    if (src_inode_num < 0) {
        std::cerr << "來源檔案不存在: " << src_path << std::endl;
        return false;
    }

    Inode src_inode;
    if (!inode_manager_->readInode(src_inode_num, src_inode)) {
        return false;
    }

    // 2. 目前只支援檔案複製，不支援目錄複製
    if (src_inode.isDirectory()) {
        std::cerr << "目前不支援目錄複製" << std::endl;
        return false;
    }

    // 3. 檢查目標路徑是否已存在
    if (resolvePath(dest_path) >= 0) {
        std::cerr << "目標檔案已存在: " << dest_path << std::endl;
        return false;
    }

    // 4. 創建新檔案
    if (!create(dest_path, false)) {
        std::cerr << "無法創建目標檔案" << std::endl;
        return false;
    }

    // 5. 讀取來源檔案內容
    std::vector<char> buffer(src_inode.size);
    int bytes_read = read(src_path, buffer.data(), src_inode.size);
    if (bytes_read != (int)src_inode.size) {
        std::cerr << "讀取來源檔案失敗" << std::endl;
        remove(dest_path); // 清理
        return false;
    }

    // 6. 寫入目標檔案
    int bytes_written = write(dest_path, buffer.data(), src_inode.size);
    if (bytes_written != (int)src_inode.size) {
        std::cerr << "寫入目標檔案失敗" << std::endl;
        remove(dest_path); // 清理
        return false;
    }

    return true;
}

bool FileSystem::move(const std::string& src_path, const std::string& dest_path) {
    if (!mounted_) return false;

    // 1. 檢查來源是否存在
    int src_inode_num = resolvePath(src_path);
    if (src_inode_num < 0) {
        std::cerr << "來源不存在: " << src_path << std::endl;
        return false;
    }

    // 2. 檢查目標是否已存在
    if (resolvePath(dest_path) >= 0) {
        std::cerr << "目標已存在: " << dest_path << std::endl;
        return false;
    }

    // 3. 分割來源和目標路徑
    auto src_parts = splitPath(src_path);
    auto dest_parts = splitPath(dest_path);

    if (src_parts.empty() || dest_parts.empty()) {
        std::cerr << "路徑無效" << std::endl;
        return false;
    }

    std::string src_name = src_parts.back();
    std::string dest_name = dest_parts.back();

    // 4. 獲取來源父目錄
    std::string src_parent_path = "/";
    for (size_t i = 0; i < src_parts.size() - 1; ++i) {
        if (i > 0) src_parent_path += "/";
        src_parent_path += src_parts[i];
    }
    if (src_parts.size() == 1) src_parent_path = "/";

    int src_parent_num = resolvePath(src_parent_path);
    if (src_parent_num < 0) return false;

    // 5. 獲取目標父目錄
    std::string dest_parent_path = "/";
    for (size_t i = 0; i < dest_parts.size() - 1; ++i) {
        if (i > 0) dest_parent_path += "/";
        dest_parent_path += dest_parts[i];
    }
    if (dest_parts.size() == 1) dest_parent_path = "/";

    int dest_parent_num = resolvePath(dest_parent_path);
    if (dest_parent_num < 0) {
        std::cerr << "目標父目錄不存在: " << dest_parent_path << std::endl;
        return false;
    }

    // 6. 從來源父目錄移除
    if (!removeFromDirectory(src_parent_num, src_name)) {
        std::cerr << "無法從來源目錄移除" << std::endl;
        return false;
    }

    // 7. 加入目標父目錄
    if (!addToDirectory(dest_parent_num, dest_name, src_inode_num)) {
        std::cerr << "無法加入目標目錄" << std::endl;
        // 回復到來源目錄
        addToDirectory(src_parent_num, src_name, src_inode_num);
        return false;
    }

    return true;
}
