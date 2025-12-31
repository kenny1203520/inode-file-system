#include <iostream>
#include <cstring>
#include "FileSystem.h"

// 測試單個操作並顯示詳細調試資訊
int main() {
    std::cout << "=== 調試測試程式 ===" << std::endl;
    
    FileSystem fs;
    
    // 測試 1: 格式化
    std::cout << "\n[測試 1] 格式化檔案系統..." << std::endl;
    if (!fs.format("debug_fs.img", false)) {
        std::cerr << "✗ 格式化失敗！" << std::endl;
        return 1;
    }
    std::cout << "✓ 格式化成功" << std::endl;
    
    // 測試 2: 列出根目錄
    std::cout << "\n[測試 2] 列出根目錄..." << std::endl;
    auto files = fs.list("/");
    std::cout << "根目錄包含 " << files.size() << " 個項目：";
    for (const auto& f : files) {
        std::cout << " [" << f << "]";
    }
    std::cout << std::endl;
    
    // 測試 3: 顯示統計資訊
    std::cout << "\n[測試 3] 檔案系統資訊" << std::endl;
    fs.printInfo();
    
    // 測試 4: 創建第一個目錄
    std::cout << "\n[測試 4] 創建 /test 目錄..." << std::endl;
    std::cout << "  呼叫 fs.mkdir(\"/test\")..." << std::endl;
    if (!fs.mkdir("/test")) {
        std::cerr << "✗ 創建失敗！程式即將退出。" << std::endl;
        std::cerr << "  這就是主程式崩潰的地方。" << std::endl;
        return 1;
    }
    std::cout << "✓ 創建成功" << std::endl;
    
    // 測試 5: 再次列出根目錄
    std::cout << "\n[測試 5] 再次列出根目錄（應包含 test）..." << std::endl;
    files = fs.list("/");
    std::cout << "根目錄包含 " << files.size() << " 個項目：";
    for (const auto& f : files) {
        std::cout << " [" << f << "]";
    }
    std::cout << std::endl;
    
    // 測試 6: 創建嵌套目錄
    std::cout << "\n[測試 6] 創建 /test/sub 目錄..." << std::endl;
    if (!fs.mkdir("/test/sub")) {
        std::cerr << "✗ 創建失敗！" << std::endl;
        return 1;
    }
    std::cout << "✓ 創建成功" << std::endl;
    
    // 測試 7: 創建檔案
    std::cout << "\n[測試 7] 創建 /test/file.txt..." << std::endl;
    if (!fs.create("/test/file.txt", false)) {
        std::cerr << "✗ 創建失敗！" << std::endl;
        return 1;
    }
    std::cout << "✓ 創建成功" << std::endl;
    
    // 測試 8: 寫入資料
    std::cout << "\n[測試 8] 寫入資料到 /test/file.txt..." << std::endl;
    const char* data = "Hello, Debug!";
    int written = fs.write("/test/file.txt", data, strlen(data));
    std::cout << "✓ 寫入了 " << written << " bytes" << std::endl;
    
    // 測試 9: 讀取資料
    std::cout << "\n[測試 9] 讀取資料..." << std::endl;
    char buffer[100] = {0};
    int bytes_read = fs.read("/test/file.txt", buffer, sizeof(buffer));
    std::cout << "✓ 讀取了 " << bytes_read << " bytes: \"" << buffer << "\"" << std::endl;
    
    // 測試 10: 最終統計
    std::cout << "\n[測試 10] 最終檔案系統狀態" << std::endl;
    fs.printInfo();
    
    std::cout << "\n✓✓✓ 所有測試通過！✓✓✓" << std::endl;
    
    return 0;
}
