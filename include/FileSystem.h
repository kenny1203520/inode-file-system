#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <string>
#include <vector>
#include <memory>
#include "structures.h"
#include "DiskEmulator.h"
#include "Bitmap.h"
#include "InodeManager.h"

// FileSystem 類別 - 高層檔案系統 API
class FileSystem {
private:
    std::unique_ptr<DiskEmulator> disk_;
    std::unique_ptr<Bitmap> inode_bitmap_;
    std::unique_ptr<Bitmap> block_bitmap_;
    std::unique_ptr<InodeManager> inode_manager_;
    bool mounted_;

public:
    FileSystem();
    ~FileSystem();

    // 格式化檔案系統
    bool format(const std::string& disk_path, bool use_memory = false);

    // 掛載檔案系統
    bool mount(const std::string& disk_path);

    // 卸載檔案系統
    void unmount();

    // 創建檔案或目錄
    bool create(const std::string& path, bool is_directory);

    // 刪除檔案或目錄
    bool remove(const std::string& path);

    // 讀取檔案
    int read(const std::string& path, void* buffer, uint32_t size, uint32_t offset = 0);

    // 寫入檔案
    int write(const std::string& path, const void* buffer, uint32_t size, uint32_t offset = 0);

    // 列出目錄內容
    std::vector<std::string> list(const std::string& path);

    // 創建目錄
    bool mkdir(const std::string& path);

    // 刪除目錄
    bool rmdir(const std::string& path);

    // 獲取檔案資訊
    bool stat(const std::string& path, Inode& inode);

    // 複製檔案或目錄
    bool copy(const std::string& src_path, const std::string& dest_path);

    // 移動/重命名檔案或目錄
    bool move(const std::string& src_path, const std::string& dest_path);

    // 顯示檔案系統資訊
    void printInfo();

private:
    // 路徑解析：將路徑轉換為 inode 編號
    int resolvePath(const std::string& path);

    // 從目錄中查找項目
    int findInDirectory(uint32_t dir_inode_num, const std::string& name);

    // 在目錄中添加項目
    bool addToDirectory(uint32_t dir_inode_num, const std::string& name, uint32_t inode_num);

    // 從目錄中移除項目
    bool removeFromDirectory(uint32_t dir_inode_num, const std::string& name);

    // 檢查目錄是否為空
    bool isDirectoryEmpty(uint32_t dir_inode_num);

    // 分割路徑
    std::vector<std::string> splitPath(const std::string& path);
};

#endif // FILE_SYSTEM_H
