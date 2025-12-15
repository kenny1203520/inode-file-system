# Inode 檔案系統實作

一個完整的基於 inode 的檔案系統實作，使用 C++ 開發。

## 專案特色

- ✅ 完整的 inode 結構實作
- ✅ 多級索引（直接、單重間接、雙重間接區塊）
- ✅ 支援目錄與檔案操作
- ✅ 硬碟/記憶體雙後端儲存（可切換）
- ✅ 持久化儲存
- ✅ 完整的檔案 I/O 操作

## 系統架構

```
├── include/              # 標頭檔
│   ├── config.h         # 系統配置
│   ├── structures.h     # 核心資料結構
│   ├── DiskEmulator.h   # 儲存抽象層
│   ├── Bitmap.h         # 位圖管理
│   ├── InodeManager.h   # Inode 管理
│   └── FileSystem.h     # 檔案系統 API
├── src/                 # 源檔案
│   ├── DiskEmulator.cpp
│   ├── Bitmap.cpp
│   ├── InodeManager.cpp
│   ├── FileSystem.cpp
│   └── main.cpp         # 示範程式
└── CMakeLists.txt       # CMake 構建配置
```

## 檔案系統規格

- **區塊大小**: 4KB
- **總區塊數**: 1024 (4MB)
- **Inode 數量**: 128
- **支援最大檔案大小**: 約 4GB（透過雙重間接區塊）

### 磁碟佈局

```
Block 0:     Superblock
Blocks 1-2:  Inode Bitmap
Blocks 3-4:  Block Bitmap  
Blocks 5-8:  Inode Table
Blocks 9+:   Data Blocks
```

## 編譯與執行

### Windows (使用 CMake)

```bash
# 創建 build 目錄
mkdir build
cd build

# 生成專案
cmake ..

# 編譯（根據生成器選擇）
# Visual Studio:
cmake --build . --config Release

# MinGW:
mingw32-make

# 執行
.\Release\inode_fs_demo.exe
# 或
.\inode_fs_demo.exe
```

### Linux / macOS

```bash
mkdir build
cd build
cmake ..
make
./inode_fs_demo
```

### 直接使用 g++ 編譯

```bash
g++ -std=c++17 -I include \
    src/DiskEmulator.cpp \
    src/Bitmap.cpp \
    src/InodeManager.cpp \
    src/FileSystem.cpp \
    src/main.cpp \
    -o inode_fs_demo
```

## 功能演示

示範程式 (`main.cpp`) 展示了以下功能：

1. **格式化檔案系統** - 初始化新的檔案系統
2. **創建目錄結構** - 建立多層目錄
3. **創建並寫入檔案** - 寫入文字檔案
4. **讀取檔案** - 讀取並顯示檔案內容
5. **大檔案測試** - 測試多級索引（寫入 60KB 檔案）
6. **列出目錄** - 顯示目錄內容
7. **刪除檔案** - 移除檔案並釋放空間
8. **持久化測試** - 卸載後重新掛載，驗證資料保存

## API 使用範例

```cpp
#include "FileSystem.h"

int main() {
    FileSystem fs;
    
    // 格式化新檔案系統
    fs.format("my_disk.img");
    
    // 創建目錄
    fs.mkdir("/home");
    fs.mkdir("/home/user");
    
    // 創建檔案
    fs.create("/home/user/test.txt", false);
    
    // 寫入資料
    std::string data = "Hello, World!";
    fs.write("/home/user/test.txt", data.c_str(), data.size());
    
    // 讀取資料
    char buffer[100];
    int bytes = fs.read("/home/user/test.txt", buffer, sizeof(buffer));
    
    // 列出目錄
    auto files = fs.list("/home/user");
    
    // 顯示檔案系統資訊
    fs.printInfo();
    
    return 0;
}
```

## 切換儲存後端

專案設計了抽象儲存層，可輕鬆切換：

### 使用硬碟儲存（預設）

```cpp
fs.format("filesystem.img", false);  // false = 使用檔案
```

### 使用記憶體儲存

```cpp
fs.format("", true);  // true = 使用記憶體
```

## 核心資料結構

### Superblock

儲存檔案系統全局資訊：
- 魔數（識別碼）
- 總區塊數 / 空閒區塊數
- 總 inode 數 / 空閒 inode 數
- 各區域起始位置

### Inode

儲存檔案/目錄元數據：
- 檔案類型（一般檔案/目錄）
- 檔案大小
- 12 個直接區塊指標
- 單重/雙重間接區塊指標
- 時間戳記

### DirectoryEntry

目錄項目：
- Inode 編號
- 檔案/目錄名稱

## 多級索引說明

支援大檔案的關鍵機制：

- **直接區塊 (12個)**: 支援最多 48KB (12 × 4KB)
- **單重間接**: 額外 4MB (1024 × 4KB)
- **雙重間接**: 額外 4GB (1024 × 1024 × 4KB)

## 作業說明

此專案為作業系統課程的實作作業，展示了：

1. inode 檔案系統的核心概念
2. 區塊管理與分配
3. 多級索引機制
4. 目錄結構與路徑解析
5. 檔案 I/O 操作
6. 儲存抽象化設計

## 未來擴展

可考慮添加的功能：

- [ ] 權限管理（User/Group/Other）
- [ ] 符號連結
- [ ] 硬連結支援
- [ ] 檔案系統修復工具
- [ ] 三重間接區塊
- [ ] 日誌功能

## 授權

此專案為教育目的開發。

## 作者

銘傳大學 - 作業系統課程實作

---

**注意**: 執行程式後會在當前目錄生成 `filesystem.img` 檔案（約 4MB），這是模擬的磁碟映像檔。
