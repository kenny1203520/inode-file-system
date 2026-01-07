# Inode-based File System (MiniFS)

這是一個功能完整的 Inode 基礎檔案系統實作，模擬 Unix-like 檔案系統的核心概念，並提供現代化的圖形介面管理器。

## ✨ 核心特性

### 檔案系統基礎

- **磁碟模擬**：使用 `disk.img` (16MB) 模擬物理磁碟
- **Inode 結構**：每個檔案/目錄由 64-byte 的 Inode 描述
- **目錄管理**：支援多層目錄結構的創建、刪除與巡覽
- **檔案操作**：完整的檔案 CRUD 操作（創建、讀取、寫入、刪除）
- **持久化**：所有資料寫入 `disk.img`，程式重啟後保持完整

### 進階功能

- **檔案複製/移動**：支援跨目錄的複製與移動操作
- **重新命名**：直觀的重新命名介面
- **檔案編輯器**：內建多行文字編輯器，支援 Ctrl+S 儲存
- **詳細資訊**：顯示 Inode 編號、區塊佔用、資料夾項目統計
- **右鍵選單**：情境感知的操作選單
- **資料夾大小計算**：遞迴計算資料夾總大小

---

## 🛠️ 編譯與建置 (Build)

本專案使用 C++ 編寫，並提供 Windows 批次檔以快速建置。

### 前置需求

- 已安裝 MinGW-w64 (g++) 編譯器，並已加入環境變數 path。

### 一鍵編譯

直接雙擊執行根目錄下的 `build.bat`，或在終端機輸入：

```cmd
.\build.bat
```

成功後將產生以下三個執行檔：

1.  **`minifs.exe`**: 自動化測試程式（執行預設的測試腳本）。
2.  **`shell.exe`**: 互動式命令列介面 (CLI)。
3.  **`gui.exe`**: 圖形化檔案管理器 (GUI)。

---

## 🚀 操作指南 (Operation Guide)

### 1. 自動化測試 (`minifs.exe`)

此程式會自動執行一系列測試，包含格式化、創建目錄、寫入檔案、刪除檔案與持久化驗證。

- **執行方法**: `.\minifs.exe`
- **用途**: 快速驗證系統核心功能是否正常。

### 2. 互動式 Shell (`shell.exe`)

提供類似 Linux 的命令列環境，讓您手動操作檔案系統。

- **執行方法**: `.\shell.exe`
- **可用指令**:

  **[規格書要求指令]**

  - `mkfs <disk>`: 建立新的檔案系統 (格式化)
  - `ls [path]`: 列出目錄內容
  - `mkdir [path]`: 建立目錄
  - `rmdir [path]`: 刪除空目錄
  - `touch [path]`: 建立新檔案
  - `rm [path]`: 刪除檔案
  - `cat [path]`: 顯示檔案內容
  - `append [path] "text"`: 將文字附加到檔尾
  - `stat [path]`: 顯示 Inode 資訊（大小、編號、區塊數）

  **[額外擴充功能]**

  - `cd [path]`: 切換當前工作目錄
  - `write [path] [content]`: 覆寫模式寫入檔案
  - `copy [src] [dst]`: 複製檔案到目標路徑
  - `move [src] [dst]`: 移動/重新命名檔案
  - `format`: `mkfs` 的別名
  - `info`: 顯示磁碟與 Inode 使用狀況
  - `exit`: 離開 Shell

**範例**:

```text
minifs:/> mkfs disk.img
minifs:/> mkdir docs
minifs:/> touch docs/readme.txt
minifs:/> append docs/readme.txt "Hello MiniFS!"
minifs:/> cat docs/readme.txt
Hello MiniFS!
```

### 3. 圖形化介面 (`gui.exe`)

提供 Windows 原生視窗介面，功能豐富的現代化檔案管理器。

- **執行方法**: `.\gui.exe`

- **核心功能**:

  - **檔案瀏覽**: 清晰顯示當前目錄的所有檔案與資料夾
  - **雙擊操作**:
    - 雙擊資料夾進入子目錄
    - 雙擊檔案直接開啟編輯器
  - **工具列按鈕**:
    - `<` (上一層): 快速返回上層目錄
    - `R` (重新整理): 更新目錄列表

- **右鍵選單** (情境感知):

  **選取檔案/資料夾時**:

  - `開啟` - 進入資料夾或編輯檔案
  - `複製` - 複製選取項目
  - `剪下` - 剪下以移動項目
  - `刪除` - 刪除選取項目
  - `重新命名` - 彈出對話框重新命名
  - `內容` - 查看詳細資訊

  **空白處右鍵**:

  - `新增` → `資料夾` - 建立新資料夾
  - `新增` → `檔案` - 建立新檔案
  - `貼上` - 貼上已複製/剪下的項目
  - `重新整理` - 更新列表

- **檔案編輯器**:

  - 多行文字編輯，自動換行
  - `Ctrl+S` 快速儲存
  - 關閉視窗時自動提示儲存
  - 支援最大 16KB 文字內容

- **詳細資訊視窗**:
  - 檔案：顯示 Inode 編號、佔用區塊數、磁碟空間
  - 資料夾：額外顯示項目統計（資料夾數、檔案數）與遞迴總大小

> 💡 **提示**: 詳細功能說明請參閱 [FEATURES.md](FEATURES.md)

---

## 📂 系統規格 (System Specification)

- **總容量**: 16 MB
- **區塊大小 (Block Size)**: 4096 bytes (4KB)
- **總區塊數**: 4096 個
- **Inode 總數**: 256 個
- **Inode 大小**: 64 bytes
- **最大檔案大小**: 16 KB (4 個直接區塊)
- **檔名限制**: 28 字元
- **支援平台**: Windows (使用 Win32 API)

## 📝 專案結構

```
inode-file-system/
├── build.bat           # 建置腳本 (一鍵編譯)
├── disk.img            # 虛擬磁碟映像檔 (執行後自動產生)
├── README.md           # 專案說明文件
├── FEATURES.md         # 詳細功能說明文檔
├── include/            # 標頭檔
│   ├── config.h        # 系統參數設定
│   ├── structures.h    # 資料結構定義 (Superblock, Inode, Dentry)
│   ├── Bitmap.h        # Bitmap 管理
│   ├── DiskEmulator.h  # 磁碟模擬器
│   ├── FileSystem.h    # 檔案系統介面
│   └── InodeManager.h  # Inode 管理器
├── src/                # 原始碼
│   ├── main.cpp        # 自動化測試主程式
│   ├── Shell.cpp       # 互動式 Shell 主程式
│   ├── GuiMain.cpp     # GUI 主程式 (Win32 API)
│   ├── FileSystem.cpp  # 檔案系統核心邏輯
│   ├── InodeManager.cpp# Inode 與區塊管理
│   ├── DiskEmulator.cpp# 磁碟讀寫模擬
│   └── Bitmap.cpp      # Bitmap 操作實作
└── minifs.exe          # 編譯產物：自動化測試程式
    shell.exe           # 編譯產物：命令列介面
    gui.exe             # 編譯產物：圖形化介面
```

## 🎯 使用場景

1. **學習檔案系統原理**: 理解 Inode、目錄項、區塊管理等核心概念
2. **實驗檔案操作**: 安全的沙盒環境測試檔案系統行為
3. **GUI 開發參考**: Win32 原生 GUI 與檔案系統互動的實作範例
4. **作業系統課程**: 適合作為教學或專題使用

## 📚 延伸閱讀

- [FEATURES.md](FEATURES.md) - 完整功能清單與使用說明
- `include/structures.h` - 詳細的資料結構定義
- `src/FileSystem.cpp` - 檔案系統核心實作邏輯

---

**開發環境**: C++ (MinGW-w64) | **GUI 框架**: Win32 API | **編譯器**: g++ 4.9.2+
