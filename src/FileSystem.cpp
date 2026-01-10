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
    // 1. Initialize Disk (16MB)
    if (use_memory) {
        disk_ = std::make_unique<MemoryDiskEmulator>(Config::TOTAL_BLOCKS, Config::BLOCK_SIZE);
    } else {
        auto file_disk = std::make_unique<FileDiskEmulator>(disk_path, Config::TOTAL_BLOCKS, Config::BLOCK_SIZE);
        if (!file_disk->initialize()) {
            return false;
        }
        disk_ = std::move(file_disk);
    }

    // 2. Write Superblock (Block 0)
    Superblock sb;
    // Note: Superblock is now POD initialized by constructor with default constants
    if (!disk_->writeBlock(Config::SUPERBLOCK_BLOCK, &sb)) {
        std::cerr << "寫入 superblock 失敗" << std::endl;
        return false;
    }

    // 3. Initialize Bitmaps
    // Inode Bitmap at Block 1, Block Bitmap at Block 2
    inode_bitmap_ = std::make_unique<Bitmap>(disk_.get(), Config::INODE_BITMAP_BLOCK, 
                                              1, Config::TOTAL_INODES);
    block_bitmap_ = std::make_unique<Bitmap>(disk_.get(), Config::BLOCK_BITMAP_BLOCK, 
                                              1, Config::TOTAL_BLOCKS - Config::DATA_BLOCKS_START);

    // Initial State: All 0 (Free)
    // Wait, Block Bitmap logic: 
    // If we map 0->DataBlockStart, then we don't need to mark metadata blocks as used in THIS bitmap 
    // because this bitmap ONLY covers the Data Area.
    // However, the Master Spec said: "Initialize Bitmaps... Mark Block 0~6 as 1 (Used)".
    // If the Block Bitmap covers the ENTIRE disk (0 to 4096), then we mark 0-6 used.
    // If it covers ONLY Data area, then 0 represents Block 7.
    // Let's stick to "Bitmap tracks Data Blocks Only" to simplify logic (1 bit = 1 allocatable data block).
    // The Metadata blocks are implicitly "used" because they are not in the allocatable range.
    // But to follow the Master Spec strictly: "Block 2: Block Bitmap tracks 4096 data blocks usage."
    // "Block 0~6... mark as 1".
    // This implies the bitmap covers 0 to 4095.
    // Let's ADJUST logic: Bitmap covers 0..TOTAL_BLOCKS.
    // So `num_blocks` passed to Bitmap constructor should be enough to cover TOTAL_BLOCKS blocks.
    // 4096 bits = 512 bytes. 1 block is enough.
    
    // RE-INIT Bitmaps with FULL range support:
    block_bitmap_ = std::make_unique<Bitmap>(disk_.get(), Config::BLOCK_BITMAP_BLOCK, 
                                              1, Config::TOTAL_BLOCKS); 
    
    // Manually mark 0-6 as used
    for (uint32_t i = 0; i < Config::DATA_BLOCKS_START; ++i) {
        block_bitmap_->setBit(i);
    }

    // Mark Root Inode (0) as used
    inode_bitmap_->setBit(Config::ROOT_INODE);

    // Save initialized bitmaps
    inode_bitmap_->save();
    block_bitmap_->save();

    // 4. Create Inode Manager
    inode_manager_ = std::make_unique<InodeManager>(disk_.get(), inode_bitmap_.get(), block_bitmap_.get());

    // 5. Initialize Root Inode
    // We manually write it because allocateInode might search for 0, and we already set bit 0.
    // Or we use allocateInode logic but need to ensure it picks 0.
    // Since we set bit 0, allocateInode will pick 1.
    // So let's manually init Root Inode.
    Inode root;
    root.inode_num = Config::ROOT_INODE;
    root.file_type = FileType::DIRECTORY;
    root.size = 2 * sizeof(DirectoryEntry); // . and ..
    for(int i=0; i<Config::DIRECT_BLOCKS; ++i) root.direct_blks[i] = 0;

    // Allocate data block for Root Directory
    // We need to verify which block allocateBlock returns. It should be first free one.
    // Since 0-6 used, it should return 7.
    int root_block = inode_manager_->allocateBlock();
    if (root_block != static_cast<int>(Config::DATA_BLOCKS_START)) {
        // Warning but proceed
        // std::cout << "Root block allocated at " << root_block << std::endl;
    }
    root.direct_blks[0] = root_block;

    // Write Root Inode
    inode_manager_->writeInode(Config::ROOT_INODE, root);

    // 6. Initialize Root Directory Entries
    std::vector<DirectoryEntry> entries(Config::DIR_ENTRIES_PER_BLOCK);
    // Init all to invalid
    for(auto& e : entries) e.inode_number = 0xFFFFFFFF; // Invalid

    entries[0] = DirectoryEntry(Config::ROOT_INODE, ".");
    entries[1] = DirectoryEntry(Config::ROOT_INODE, "..");
    
    disk_->writeBlock(root_block, entries.data());

    // Final Save
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
    
    // Note: InodeManager loadSuperblock is redundant if we already read it, but keeps sync
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

// Split path helper
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

    // Scan all direct blocks
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

    // Find empty slot
    for (int i = 0; i < Config::DIRECT_BLOCKS; ++i) {
        bool allocate = false;
        if (dir_inode.direct_blks[i] == 0) {
            // Alloc new block
            int new_blk = inode_manager_->allocateBlock();
            if (new_blk < 0) return false;
            dir_inode.direct_blks[i] = new_blk;
            // Init new block with invalid entries
            std::vector<DirectoryEntry> empty_entries(Config::DIR_ENTRIES_PER_BLOCK); 
            // Default constructor sets inode=0xFFFFFFFF
            disk_->writeBlock(new_blk, empty_entries.data());
            
            inode_manager_->writeInode(dir_inode_num, dir_inode);
            allocate = true;
        }

        uint32_t block_num = dir_inode.direct_blks[i];
        std::vector<DirectoryEntry> entries(Config::DIR_ENTRIES_PER_BLOCK);
        disk_->readBlock(block_num, entries.data());

        for (auto& entry : entries) {
            if (entry.inode_number == 0xFFFFFFFF) {
                // Found empty slot
                entry = DirectoryEntry(inode_num, name.c_str());
                disk_->writeBlock(block_num, entries.data());
                
                // Update size to reflect content? Or just stay roughly compatible. 
                // Spec says size is in bytes. Strictly speaking size should cover used entries?
                // Or just be logical size? Let's just update size if needed, but for directory usually size = used blocks * 4096 or just ignore size.
                // Or size = number of entries * 32. 
                // Let's increment size by 32 bytes for each entry added?
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
                entry.inode_number = 0xFFFFFFFF; // Mark invalid
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
        // Init . and ..
        if(!addToDirectory(new_inode, ".", new_inode) || 
           !addToDirectory(new_inode, "..", parent)) {
             // Rollback?
             return false;
        }
    }

    if (!addToDirectory(parent, filename, new_inode)) {
        // Rollback
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

    // Unlink from parent
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

// Read/Write for files
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
        if (phys_blk <= 0) break; // Should not happen if size is correct

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

    uint32_t bytes_written = 0;
    const uint8_t* in = (const uint8_t*)buffer;

    while(bytes_written < size) {
        uint32_t logical_idx = (offset + bytes_written) / Config::BLOCK_SIZE;
        uint32_t blk_off = (offset + bytes_written) % Config::BLOCK_SIZE;
        uint32_t to_write = std::min(size - bytes_written, Config::BLOCK_SIZE - blk_off);

        // Auto allocate
        int phys_blk = inode_manager_->getBlockNumber(inode, logical_idx, true);
        if (phys_blk <= 0) {
            std::cerr << "No space or file too large" << std::endl;
            break;
        }
        inode_manager_->writeInode(inode_num, inode); // update direct_blks

        std::vector<uint8_t> block_buf(Config::BLOCK_SIZE);
        if (blk_off != 0 || to_write < Config::BLOCK_SIZE) {
            disk_->readBlock(phys_blk, block_buf.data());
        }
        std::memcpy(block_buf.data() + blk_off, in + bytes_written, to_write);
        disk_->writeBlock(phys_blk, block_buf.data());

        bytes_written += to_write;
    }

    if (offset + bytes_written > inode.size) {
        inode.size = offset + bytes_written;
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
    // Fields free_blocks/free_inodes removed from struct. 
    // Use bitmaps if needed.
    uint32_t free_inodes = inode_bitmap_->countFree();
    uint32_t free_blocks = block_bitmap_->countFree();
    
    std::cout << "System Info: 16MB Disk, 4KB x 4096 blocks." << std::endl;
    std::cout << "Free Inodes: " << free_inodes << "/" << Config::TOTAL_INODES << std::endl;
    std::cout << "Free Blocks: " << free_blocks << "/" << Config::TOTAL_BLOCKS << std::endl;
}
