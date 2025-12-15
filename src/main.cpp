#include <iostream>
#include <iomanip>
#include "FileSystem.h"

void printSeparator() {
    std::cout << "\n" << std::string(60, '=') << "\n" << std::endl;
}

void printFileContent(const std::string& path, FileSystem& fs) {
    Inode inode;
    if (fs.stat(path, inode)) {
        char buffer[1024];
        int bytes = fs.read(path, buffer, inode.size);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::cout << "檔案內容 [" << path << "]:\n" << buffer << std::endl;
        }
    }
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    Inode 檔案系統示範程式                     ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝\n" << std::endl;

    FileSystem fs;

    // ===== 1. 格式化檔案系統 =====
    printSeparator();
    std::cout << "步驟 1: 格式化檔案系統" << std::endl;
    if (!fs.format("filesystem.img", false)) {
        std::cerr << "格式化失敗" << std::endl;
        return 1;
    }
    fs.printInfo();

    // ===== 2. 創建目錄結構 =====
    printSeparator();
    std::cout << "步驟 2: 創建目錄結構" << std::endl;
    fs.mkdir("/home");
    fs.mkdir("/home/user");
    fs.mkdir("/home/user/documents");
    fs.mkdir("/tmp");
    
    std::cout << "\n根目錄內容:" << std::endl;
    auto root_files = fs.list("/");
    for (const auto& file : root_files) {
        std::cout << "  - " << file << std::endl;
    }

    std::cout << "\n/home 目錄內容:" << std::endl;
    auto home_files = fs.list("/home");
    for (const auto& file : home_files) {
        std::cout << "  - " << file << std::endl;
    }

    // ===== 3. 創建並寫入文件 =====
    printSeparator();
    std::cout << "步驟 3: 創建並寫入文件" << std::endl;
    
    fs.create("/home/user/hello.txt", false);
    std::string content1 = "Hello, Inode File System!\n這是一個測試文件。\n";
    fs.write("/home/user/hello.txt", content1.c_str(), content1.size());
    std::cout << "已寫入 " << content1.size() << " bytes 到 /home/user/hello.txt" << std::endl;

    fs.create("/home/user/documents/readme.txt", false);
    std::string content2 = "這是 README 文件\n";
    content2 += "支援的功能:\n";
    content2 += "1. 創建/刪除文件和目錄\n";
    content2 += "2. 讀取/寫入文件\n";
    content2 += "3. 多級索引支援大文件\n";
    content2 += "4. 可切換硬碟/記憶體儲存\n";
    fs.write("/home/user/documents/readme.txt", content2.c_str(), content2.size());
    std::cout << "已寫入 " << content2.size() << " bytes 到 /home/user/documents/readme.txt" << std::endl;

    // ===== 4. 讀取文件 =====
    printSeparator();
    std::cout << "步驟 4: 讀取文件內容" << std::endl;
    printFileContent("/home/user/hello.txt", fs);
    std::cout << std::endl;
    printFileContent("/home/user/documents/readme.txt", fs);

    // ===== 5. 測試大文件（多級索引） =====
    printSeparator();
    std::cout << "步驟 5: 測試大文件寫入（測試間接區塊）" << std::endl;
    fs.create("/tmp/bigfile.dat", false);
    
    // 寫入 60KB 的資料（超過 12 個直接區塊 = 48KB）
    const uint32_t big_size = 60 * 1024;
    std::vector<char> big_data(big_size);
    for (uint32_t i = 0; i < big_size; ++i) {
        big_data[i] = 'A' + (i % 26);
    }
    
    int written = fs.write("/tmp/bigfile.dat", big_data.data(), big_size);
    std::cout << "已寫入 " << written << " bytes 到 /tmp/bigfile.dat" << std::endl;
    
    // 讀回並驗證
    std::vector<char> read_buffer(big_size);
    int bytes_read = fs.read("/tmp/bigfile.dat", read_buffer.data(), big_size);
    std::cout << "讀取了 " << bytes_read << " bytes" << std::endl;
    
    bool verified = (written == bytes_read && 
                     std::memcmp(big_data.data(), read_buffer.data(), big_size) == 0);
    std::cout << "資料驗證: " << (verified ? "✓ 成功" : "✗ 失敗") << std::endl;

    // ===== 6. 列出目錄 =====
    printSeparator();
    std::cout << "步驟 6: 列出所有目錄內容" << std::endl;
    
    std::cout << "\n/home/user 目錄:" << std::endl;
    auto user_files = fs.list("/home/user");
    for (const auto& file : user_files) {
        Inode inode;
        std::string full_path = "/home/user/" + file;
        if (fs.stat(full_path, inode) && file != "." && file != "..") {
            std::string type = inode.isDirectory() ? "[DIR] " : "[FILE]";
            std::cout << "  " << type << " " << std::setw(20) << std::left << file 
                      << " (" << inode.size << " bytes)" << std::endl;
        }
    }

    std::cout << "\n/tmp 目錄:" << std::endl;
    auto tmp_files = fs.list("/tmp");
    for (const auto& file : tmp_files) {
        Inode inode;
        std::string full_path = "/tmp/" + file;
        if (fs.stat(full_path, inode) && file != "." && file != "..") {
            std::string type = inode.isDirectory() ? "[DIR] " : "[FILE]";
            std::cout << "  " << type << " " << std::setw(20) << std::left << file 
                      << " (" << inode.size << " bytes)" << std::endl;
        }
    }

    // ===== 7. 刪除文件 =====
    printSeparator();
    std::cout << "步驟 7: 刪除文件" << std::endl;
    fs.remove("/tmp/bigfile.dat");
    
    std::cout << "\n刪除後 /tmp 目錄:" << std::endl;
    tmp_files = fs.list("/tmp");
    for (const auto& file : tmp_files) {
        if (file != "." && file != "..") {
            std::cout << "  - " << file << std::endl;
        }
    }

    // ===== 8. 最終狀態 =====
    printSeparator();
    std::cout << "步驟 8: 檔案系統最終狀態" << std::endl;
    fs.printInfo();

    // ===== 9. 卸載 =====
    printSeparator();
    std::cout << "步驟 9: 卸載檔案系統" << std::endl;
    fs.unmount();

    // ===== 10. 重新掛載並驗證持久性 =====
    printSeparator();
    std::cout << "步驟 10: 重新掛載並驗證資料持久性" << std::endl;
    if (fs.mount("filesystem.img")) {
        std::cout << "\n驗證資料是否保存:" << std::endl;
        printFileContent("/home/user/hello.txt", fs);
        
        std::cout << "\n/home/user 目錄內容:" << std::endl;
        auto persisted_files = fs.list("/home/user");
        for (const auto& file : persisted_files) {
            if (file != "." && file != "..") {
                std::cout << "  - " << file << std::endl;
            }
        }
        
        fs.printInfo();
    }

    printSeparator();
    std::cout << "✓ 所有測試完成！" << std::endl;
    std::cout << "\n產生的檔案系統映像: filesystem.img" << std::endl;
    printSeparator();

    return 0;
}
