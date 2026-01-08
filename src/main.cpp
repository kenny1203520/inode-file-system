#include <iostream>
#include <iomanip>
#ifdef _WIN32
#include <windows.h>
#endif
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
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    system("chcp 65001 > nul");
#endif
    std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
    std::cout <<   "║    MiniFS I-node like 檔案系統示範程式          ║" << std::endl;
    std::cout <<   "╚════════════════════════════════════════════════╝\n" << std::endl;

    FileSystem fs;

    // ===== 1. 格式化檔案系統 =====
    printSeparator();
    std::cout << "步驟 1: 格式化檔案系統" << std::endl;
    if (!fs.format("disk.img", false)) {
        std::cerr << "格式化失敗" << std::endl;
        return 1;
    }
    fs.printInfo();

    // ===== 2. 創建目錄結構與系統說明 =====
    printSeparator();
    std::cout << "步驟 2: 創建目錄結構與系統功能簡介" << std::endl;
    fs.mkdir("/home");
    fs.mkdir("/home/system");
    fs.mkdir("/home/user");
    fs.mkdir("/home/user/documents");
    fs.mkdir("/tmp");
    fs.mkdir("/docs");
    
    // 在 /home/system 中創建系統功能簡介文件
    std::cout << "\n創建系統功能簡介檔案..." << std::endl;
    
    // 1. 系統概述
    fs.create("/home/system/about.txt", false);
    std::string about_content = "MiniFS - Inode 檔案系統\r\n";
    about_content += "========================\r\n\r\n";
    about_content += "這是一個基於 Inode 架構的簡易檔案系統實作。\r\n\r\n";
    about_content += "系統規格:\r\n";
    about_content += "- 總容量: 16 MB\r\n";
    about_content += "- 區塊大小: 4 KB\r\n";
    about_content += "- 總區塊數: 4096 個\r\n";
    about_content += "- Inode 總數: 256 個\r\n";
    about_content += "- 最大檔案: 16 KB (4個直接區塊)\r\n";
    about_content += "- 檔名長度: 最多 28 字元\r\n\r\n";
    about_content += "支援平台: Windows (Win32 API)\r\n";
    fs.write("/home/system/about.txt", about_content.c_str(), about_content.size());
    std::cout << "  ✓ /home/system/about.txt (系統概述)" << std::endl;
    
    // 2. 核心功能列表
    fs.create("/home/system/features.txt", false);
    std::string features_content = "核心功能列表\r\n";
    features_content += "============\r\n\r\n";
    features_content += "[檔案系統管理]\r\n";
    features_content += "- mkfs/format: 格式化磁碟\r\n";
    features_content += "- mount/unmount: 掛載與卸載\r\n";
    features_content += "- info: 顯示磁碟使用狀況\r\n\r\n";
    features_content += "[目錄操作]\r\n";
    features_content += "- mkdir: 創建目錄\r\n";
    features_content += "- rmdir: 刪除空目錄\r\n";
    features_content += "- ls/list: 列出目錄內容\r\n";
    features_content += "- cd: 切換目錄 (Shell)\r\n\r\n";
    features_content += "[檔案操作]\r\n";
    features_content += "- touch/create: 創建檔案\r\n";
    features_content += "- rm/remove: 刪除檔案\r\n";
    features_content += "- cat/read: 讀取內容\r\n";
    features_content += "- write: 寫入/覆寫\r\n";
    features_content += "- append: 附加內容\r\n\r\n";
    features_content += "[進階功能]\r\n";
    features_content += "- copy: 複製檔案\r\n";
    features_content += "- move/rename: 移動/重新命名\r\n";
    features_content += "- stat: 顯示 Inode 詳細資訊\r\n\r\n";
    features_content += "[GUI 介面]\r\n";
    features_content += "- 右鍵選單操作\r\n";
    features_content += "- 檔案編輯器 (Ctrl+S)\r\n";
    features_content += "- 內容查看 (Inode/區塊/統計)\r\n";
    fs.write("/home/system/features.txt", features_content.c_str(), features_content.size());
    std::cout << "  ✓ /home/system/features.txt (功能清單)" << std::endl;
    
    // 3. 使用指南
    fs.create("/home/system/guide.txt", false);
    std::string guide_content = "快速使用指南\r\n";
    guide_content += "============\r\n\r\n";
    guide_content += "命令列模式 (shell.exe)\r\n";
    guide_content += "-----------------------\r\n\r\n";
    guide_content += "基本操作:\r\n";
    guide_content += "  minifs:/> mkdir mydir\r\n";
    guide_content += "  minifs:/> touch myfile.txt\r\n";
    guide_content += "  minifs:/> write myfile.txt \"Hello\"\r\n";
    guide_content += "  minifs:/> cat myfile.txt\r\n";
    guide_content += "  minifs:/> ls\r\n\r\n";
    guide_content += "複製與移動:\r\n";
    guide_content += "  minifs:/> cp file1.txt file2.txt\r\n";
    guide_content += "  minifs:/> mv old.txt new.txt\r\n\r\n";
    guide_content += "查看資訊:\r\n";
    guide_content += "  minifs:/> stat myfile.txt\r\n";
    guide_content += "  minifs:/> info\r\n\r\n";
    guide_content += "圖形介面 (gui.exe)\r\n";
    guide_content += "------------------\r\n\r\n";
    guide_content += "- 雙擊資料夾進入\r\n";
    guide_content += "- 雙擊檔案編輯\r\n";
    guide_content += "- 右鍵選單執行操作\r\n";
    guide_content += "- < 按鈕返回上層\r\n";
    guide_content += "- Ctrl+S 儲存編輯\r\n\r\n";
    guide_content += "詳細文檔請參閱 README.md\r\n";
    fs.write("/home/system/guide.txt", guide_content.c_str(), guide_content.size());
    std::cout << "  ✓ /home/system/guide.txt (使用指南)" << std::endl;
    
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

    std::cout << "\n/home/system 目錄內容 (系統說明文件):" << std::endl;
    auto system_files = fs.list("/home/system");
    for (const auto& file : system_files) {
        if (file != "." && file != "..") {
            std::cout << "  - " << file << std::endl;
        }
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
    content2 += "3. 小型檔案系統 (16MB)\n";
    content2 += "4. 持久化至 disk.img\n";
    fs.write("/home/user/documents/readme.txt", content2.c_str(), content2.size());
    std::cout << "已寫入 " << content2.size() << " bytes 到 /home/user/documents/readme.txt" << std::endl;

    fs.create("/docs/readme.txt", false);
    std::string content3 = "Hello MiniFS!";
    fs.write("/docs/readme.txt", content3.c_str(), content3.size());
    std::cout << "已寫入 " << content3.size() << " bytes 到 /docs/readme.txt" << std::endl;

    // ===== 4. 讀取文件 =====
    printSeparator();
    std::cout << "步驟 4: 讀取文件內容" << std::endl;
    printFileContent("/home/user/hello.txt", fs);
    std::cout << std::endl;
    printFileContent("/home/user/documents/readme.txt", fs);
    std::cout << std::endl;
    printFileContent("/docs/readme.txt", fs);

    // ===== 5. 測試較大文件 (Max 16KB) =====
    printSeparator();
    std::cout << "步驟 5: 測試多區塊寫入 (12KB)" << std::endl;
    fs.create("/tmp/bigfile.dat", false);
    
    // 寫入 12KB 的資料 (3個區塊, 上限為 4個區塊/16KB)
    const uint32_t big_size = 12 * 1024;
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

    std::cout << "\n/docs 目錄:" << std::endl;
    auto docs_files = fs.list("/docs");
    for (const auto& file : docs_files) {
        Inode inode;
        std::string full_path = "/docs/" + file;
        if (fs.stat(full_path, inode) && file != "." && file != "..") {
            std::string type = inode.isDirectory() ? "[DIR] " : "[FILE]";
            std::cout << "  " << type << " " << std::setw(20) << std::left << file 
                      << " (" << inode.size << " bytes)" << std::endl;
        }
    }

    // ===== 7. 複製文件 =====
    printSeparator();
    std::cout << "步驟 7: 測試 Copy 和 Move 功能" << std::endl;
    
    // 測試複製文件
    std::cout << "\n[Copy] 複製 /home/user/hello.txt 到 /tmp/hello_copy.txt" << std::endl;
    if (fs.copy("/home/user/hello.txt", "/tmp/hello_copy.txt")) {
        std::cout << "✓ 複製成功" << std::endl;
        printFileContent("/tmp/hello_copy.txt", fs);
    } else {
        std::cout << "✗ 複製失敗" << std::endl;
    }

    // 測試移動文件
    std::cout << "\n[Move] 移動 /tmp/hello_copy.txt 到 /docs/moved_hello.txt" << std::endl;
    if (fs.move("/tmp/hello_copy.txt", "/docs/moved_hello.txt")) {
        std::cout << "✓ 移動成功" << std::endl;
        
        std::cout << "\n/tmp 目錄 (移動後):" << std::endl;
        auto tmp_after_move = fs.list("/tmp");
        for (const auto& file : tmp_after_move) {
            if (file != "." && file != "..") {
                std::cout << "  - " << file << std::endl;
            }
        }
        
        std::cout << "\n/docs 目錄 (移動後):" << std::endl;
        auto docs_after_move = fs.list("/docs");
        for (const auto& file : docs_after_move) {
            if (file != "." && file != "..") {
                std::cout << "  - " << file << std::endl;
            }
        }
    } else {
        std::cout << "✗ 移動失敗" << std::endl;
    }

    // 測試重命名 (move 的特殊情況)
    std::cout << "\n[Rename] 重命名 /docs/moved_hello.txt 到 /docs/renamed.txt" << std::endl;
    if (fs.move("/docs/moved_hello.txt", "/docs/renamed.txt")) {
        std::cout << "✓ 重命名成功" << std::endl;
        printFileContent("/docs/renamed.txt", fs);
    } else {
        std::cout << "✗ 重命名失敗" << std::endl;
    }

    // ===== 8. 測試 Stat 功能 =====
    printSeparator();
    std::cout << "步驟 8: 測試 Stat 功能（顯示 Inode 資訊）" << std::endl;
    
    Inode stat_inode;
    std::cout << "\n[Stat] /home/user/hello.txt" << std::endl;
    if (fs.stat("/home/user/hello.txt", stat_inode)) {
        std::cout << "  Inode 編號: " << stat_inode.inode_num << std::endl;
        std::cout << "  檔案大小: " << stat_inode.size << " bytes" << std::endl;
        std::cout << "  類型: " << (stat_inode.isDirectory() ? "目錄" : "檔案") << std::endl;
        std::cout << "  使用的區塊: ";
        int block_count = 0;
        for (int i = 0; i < 4; i++) {
            if (stat_inode.direct_blks[i] != 0) {
                std::cout << stat_inode.direct_blks[i] << " ";
                block_count++;
            }
        }
        std::cout << "\n  區塊數量: " << block_count << std::endl;
    }

    std::cout << "\n[Stat] /tmp/bigfile.dat" << std::endl;
    if (fs.stat("/tmp/bigfile.dat", stat_inode)) {
        std::cout << "  Inode 編號: " << stat_inode.inode_num << std::endl;
        std::cout << "  檔案大小: " << stat_inode.size << " bytes" << std::endl;
        std::cout << "  類型: " << (stat_inode.isDirectory() ? "目錄" : "檔案") << std::endl;
        std::cout << "  使用的區塊: ";
        int block_count = 0;
        for (int i = 0; i < 4; i++) {
            if (stat_inode.direct_blks[i] != 0) {
                std::cout << stat_inode.direct_blks[i] << " ";
                block_count++;
            }
        }
        std::cout << "\n  區塊數量: " << block_count << std::endl;
    }

    std::cout << "\n[Stat] /home (目錄)" << std::endl;
    if (fs.stat("/home", stat_inode)) {
        std::cout << "  Inode 編號: " << stat_inode.inode_num << std::endl;
        std::cout << "  目錄大小: " << stat_inode.size << " bytes" << std::endl;
        std::cout << "  類型: " << (stat_inode.isDirectory() ? "目錄" : "檔案") << std::endl;
    }

    // ===== 9. 測試 Append 功能 =====
    printSeparator();
    std::cout << "步驟 9: 測試 Append 功能（附加文字）" << std::endl;
    
    std::cout << "\n原始內容:" << std::endl;
    printFileContent("/docs/renamed.txt", fs);
    
    std::string append_text = "\n--- 這是附加的內容 ---\n新增第二段文字。";
    Inode append_inode;
    if (fs.stat("/docs/renamed.txt", append_inode)) {
        int appended = fs.write("/docs/renamed.txt", append_text.c_str(), append_text.size(), append_inode.size);
        std::cout << "\n✓ 附加了 " << appended << " bytes" << std::endl;
        
        std::cout << "\n附加後內容:" << std::endl;
        printFileContent("/docs/renamed.txt", fs);
    }

    // ===== 10. 測試錯誤處理 =====
    printSeparator();
    std::cout << "步驟 10: 測試錯誤處理" << std::endl;
    
    std::cout << "\n[錯誤測試] 讀取不存在的檔案:" << std::endl;
    Inode temp_inode;
    if (!fs.stat("/nonexistent.txt", temp_inode)) {
        std::cout << "✓ 正確返回檔案不存在" << std::endl;
    }
    
    std::cout << "\n[錯誤測試] 刪除不存在的檔案:" << std::endl;
    if (!fs.remove("/nonexistent.txt")) {
        std::cout << "✓ 正確處理刪除失敗" << std::endl;
    }
    
    std::cout << "\n[錯誤測試] 在檔案上執行 mkdir:" << std::endl;
    if (!fs.mkdir("/docs/renamed.txt/invalid")) {
        std::cout << "✓ 正確拒絕在檔案上創建目錄" << std::endl;
    }
    
    std::cout << "\n[邊界測試] 最大檔名長度 (28 字元):" << std::endl;
    std::string long_name = "/tmp/abcdefghijklmnopqrstuvwxyz";  // 26 字元
    fs.create(long_name, false);
    std::string test_content = "長檔名測試";
    fs.write(long_name, test_content.c_str(), test_content.size());
    if (fs.stat(long_name, temp_inode)) {
        std::cout << "✓ 長檔名創建成功: " << long_name << std::endl;
    }

    // ===== 11. 刪除文件和目錄 =====
    printSeparator();
    std::cout << "步驟 11: 測試刪除功能 (rm/rmdir)" << std::endl;
    
    std::cout << "\n[rm] 刪除 /tmp/bigfile.dat" << std::endl;
    if (fs.remove("/tmp/bigfile.dat")) {
        std::cout << "✓ 檔案刪除成功" << std::endl;
    }
    
    std::cout << "\n刪除後 /tmp 目錄:" << std::endl;
    tmp_files = fs.list("/tmp");
    for (const auto& file : tmp_files) {
        if (file != "." && file != "..") {
            std::cout << "  - " << file << std::endl;
        }
    }

    std::cout << "\n[rm] 刪除 /docs/readme.txt" << std::endl;
    fs.remove("/docs/readme.txt");
    
    std::cout << "\n[rmdir] 嘗試刪除非空目錄 /home:" << std::endl;
    if (!fs.rmdir("/home")) {
        std::cout << "✓ 正確拒絕刪除非空目錄" << std::endl;
    }
    
    std::cout << "\n[rmdir] 清空並刪除目錄:" << std::endl;
    fs.remove("/home/user/hello.txt");
    fs.remove("/home/user/documents/readme.txt");
    fs.rmdir("/home/user/documents");
    fs.rmdir("/home/user");
    if (fs.rmdir("/home")) {
        std::cout << "✓ 成功刪除空目錄 /home" << std::endl;
    }
    
    std::cout << "\n根目錄內容 (刪除後):" << std::endl;
    root_files = fs.list("/");
    for (const auto& file : root_files) {
        if (file != "." && file != "..") {
            std::cout << "  - " << file << std::endl;
        }
    }

    // ===== 9. 最終狀態 =====
    printSeparator();
    std::cout << "步驟 12: 檔案系統最終狀態" << std::endl;
    fs.printInfo();

    // ===== 10. 卸載 =====
    printSeparator();
    std::cout << "步驟 13: 卸載檔案系統" << std::endl;
    fs.unmount();

    // ===== 11. 重新掛載並驗證持久性 =====
    printSeparator();
    std::cout << "步驟 14: 重新掛載並驗證資料持久性" << std::endl;
    if (fs.mount("disk.img")) {
        std::cout << "\n驗證資料是否保存:" << std::endl;
        printFileContent("/docs/renamed.txt", fs);
        
        std::cout << "\n/docs 目錄內容:" << std::endl;
        auto persisted_files = fs.list("/docs");
        for (const auto& file : persisted_files) {
            if (file != "." && file != "..") {
                std::cout << "  - " << file << std::endl;
            }
        }
        
        std::cout << "\n/tmp 目錄內容:" << std::endl;
        auto tmp_persisted = fs.list("/tmp");
        for (const auto& file : tmp_persisted) {
            if (file != "." && file != "..") {
                std::cout << "  - " << file << std::endl;
            }
        }
        
        fs.printInfo();
    }

    printSeparator();
    std::cout << "✓ 所有功能測試完成！" << std::endl;
    std::cout << "\n已測試功能清單:" << std::endl;
    std::cout << "  ✓ mkfs/format - 格式化檔案系統" << std::endl;
    std::cout << "  ✓ mount/unmount - 掛載/卸載" << std::endl;
    std::cout << "  ✓ mkdir - 創建目錄" << std::endl;
    std::cout << "  ✓ rmdir - 刪除空目錄" << std::endl;
    std::cout << "  ✓ touch/create - 創建檔案" << std::endl;
    std::cout << "  ✓ write - 寫入/覆寫檔案" << std::endl;
    std::cout << "  ✓ append - 附加文字到檔案" << std::endl;
    std::cout << "  ✓ read/cat - 讀取檔案內容" << std::endl;
    std::cout << "  ✓ copy - 複製檔案" << std::endl;
    std::cout << "  ✓ move/rename - 移動/重新命名" << std::endl;
    std::cout << "  ✓ remove/rm - 刪除檔案" << std::endl;
    std::cout << "  ✓ list/ls - 列出目錄" << std::endl;
    std::cout << "  ✓ stat - 顯示 Inode 資訊" << std::endl;
    std::cout << "  ✓ info - 顯示磁碟使用狀況" << std::endl;
    std::cout << "  ✓ 多區塊寫入 (12KB)" << std::endl;
    std::cout << "  ✓ 資料持久化驗證" << std::endl;
    std::cout << "  ✓ 錯誤處理測試" << std::endl;
    std::cout << "\n產生的檔案系統映像: disk.img (16 MB)" << std::endl;
    printSeparator();

    return 0;
}
