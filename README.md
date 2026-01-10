# Inode-based File System (MiniFS)

這是一個簡單的 Inode 基礎檔案系統實作，旨在模擬 Unix-like 檔案系統的核心概念。
本專案包含以下特性：
*   **磁碟模擬**：使用 `disk.img` (16MB) 模擬物理磁碟。
*   **Inode 結構**：每個檔案/目錄由一個 64-byte 的 Inode 描述。
*   **目錄管理**：支援目錄的創建、刪除與嵌套。
*   **檔案操作**：支援檔案的創建、讀取、寫入與刪除。
*   **持久化**：所有數據皆寫入 `disk.img`，程式重啟後資料依然存在。

---

## 🛠️ 編譯與建置 (Build)

本專案使用 C++ 編寫，並提供 Windows 批次檔以快速建置。

### 前置需求
*   已安裝 MinGW-w64 (g++) 編譯器，並已加入環境變數 path。

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
*   **執行方法**: `.\minifs.exe`
*   **用途**: 快速驗證系統核心功能是否正常。

### 2. 互動式 Shell (`shell.exe`)
提供類似 Linux 的命令列環境，讓您手動操作檔案系統。
*   **執行方法**: `.\shell.exe`
*   **可用指令**:

    **[規格書要求指令]**
    *   `mkfs <disk>`: 建立新的檔案系統 (格式化)
    *   `ls [path]`: 列出目錄內容
    *   `mkdir [path]`: 建立目錄
    *   `rmdir [path]`: 刪除空目錄
    *   `touch [path]`: 建立新檔案
    *   `rm [path]`: 刪除檔案
    *   `cat [path]`: 顯示檔案內容
    *   `append [path] "text"`: 將文字附加到檔尾
    *   `stat [path]`: 顯示 Inode 資訊（大小、編號、區塊數）

    **[額外擴充功能]**
    *   `cd [path]`: 切換當前工作目錄 (Shell 模擬導航)
    *   `write [path] [content]`: 覆寫模式寫入檔案
    *   `gui`: (需執行 `gui.exe`) 啟動視窗介面管理
    *   `format`: `mkfs` 的別名
    *   `info`: 顯示磁碟與 Inode 使用狀況
    *   `exit`: 離開 Shell

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
提供 Windows 原生視窗介面，直觀管理檔案。
*   **執行方法**: `.\gui.exe`
*   **功能**:
    *   **瀏覽**: 顯示當前目錄下的檔案與資料夾。
    *   **導航**: 雙擊 `[Folder]` 進入目錄，雙擊 `[..]` 或點擊 `Up` 按鈕返回上一層。
    *   **查看**: 雙擊檔案可彈出視窗顯示內容。
    *   **操作**:
        *   `New Folder`: 在當前目錄創建新資料夾 (命名為 `NewFolderX`)。
        *   `New File`: 在當前目錄創建新檔案 (命名為 `NewFileX`)。
        *   `Delete`: 刪除選取的檔案或目錄。

---

## 📂 系統規格 (System Specification)

*   **總容量**: 16 MB
*   **區塊大小 (Block Size)**: 4096 bytes (4KB)
*   **Inode 大小**: 64 bytes
*   **最大檔案大小**: 16 KB (4 個直接區塊)
*   **檔名限制**: 28 字元

## 📝 專案結構

```
inode-file-system/
├── build.bat           # 建置腳本 (一鍵編譯)
├── disk.img            # 虛擬磁碟映像檔 (程式執行後產生)
├── include/            # 標頭檔
│   ├── config.h        # 系統參數設定
│   ├── structures.h    # 資料結構定義 (Superblock, Inode, Dentry)
│   └── ...
├── src/                # 原始碼
│   ├── main.cpp        # 自動化測試主程式
│   ├── Shell.cpp       # 互動式 Shell 主程式
│   ├── GuiMain.cpp     # GUI 主程式
│   ├── FileSystem.cpp  # 檔案系統核心邏輯
│   ├── InodeManager.cpp# Inode 與區塊管理
│   ├── DiskEmulator.cpp# 磁碟讀寫模擬
│   └── Bitmap.cpp      # Bitmap 管理
└── README.md           # 說明文件
```
