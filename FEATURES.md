# MiniFS 檔案管理器 - 功能詳細說明

## 目錄

- [基本介面](#基本介面)
- [工具列功能](#工具列功能)
- [檔案操作](#檔案操作)
- [右鍵選單](#右鍵選單)
- [程式碼架構](#程式碼架構)

---

## 基本介面

### 1. 路徑標籤

**功能描述**：顯示當前所在目錄的完整路徑

**使用方式**：

- 位於視窗頂部，工具列按鈕右側
- 格式：`當前路徑: /path/to/directory`
- 自動更新當目錄變更時

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 控制項：`hPathLabel` (全域變數)
- 建立位置：行 705-706
- 更新函數：`RefreshList()` (行 285-336)

---

### 2. 檔案列表框

**功能描述**：顯示當前目錄下的所有檔案和資料夾

**使用方式**：

- 單擊選擇項目
- 雙擊進入資料夾或開啟檔案
- 顯示格式：
  - 資料夾：`[資料夾] 名稱`
  - 檔案：`名稱 (大小 bytes)`

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 控制項：`hList` (全域變數)
- 建立位置：行 708-710
- 刷新函數：`RefreshList()` (行 285-336)
- 雙擊處理：`OnDoubleClick()` (行 338-372)

---

### 3. 狀態列

**功能描述**：顯示當前目錄的統計資訊

**使用方式**：

- 位於檔案列表下方
- 顯示格式：`總計: X 個資料夾, Y 個檔案`
- 自動統計並更新

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 控制項：`hStatusLabel` (全域變數)
- 建立位置：行 713-714
- 更新位置：`RefreshList()` 函數內 (行 324-335)

---

## 工具列功能

### 4. 上一層按鈕 (<)

**功能描述**：返回上一層目錄

**使用方式**：

- 點擊工具列左側的 `<` 按鈕
- 或使用右鍵選單的「上一層」
- 在根目錄 `/` 時無作用

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 按鈕建立：行 702-703
- 事件處理：行 776-783 (ID_TOOLBAR_UP)
- 快捷鍵 ID：`ID_TOOLBAR_UP` (301)

**實作邏輯**：

```cpp
if (current_path != "/") {
    size_t last = current_path.find_last_of('/');
    if (last == 0) current_path = "/";
    else current_path = current_path.substr(0, last);
    RefreshList();
}
```

---

### 5. 重新整理按鈕 (R)

**功能描述**：刷新當前目錄的檔案列表

**使用方式**：

- 點擊工具列的 `R` 按鈕
- 或使用右鍵選單的「重新整理」
- 適用於檔案系統外部變更後同步顯示

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 按鈕建立：行 705-706
- 事件處理：行 785-787 (ID_TOOLBAR_REFRESH)
- 快捷鍵 ID：`ID_TOOLBAR_REFRESH` (302)

---

## 檔案操作

### 6. 新增資料夾

**功能描述**：在當前目錄下建立新的資料夾

**使用方式**：

1. 在空白處右鍵點擊
2. 選擇「新增資料夾」
3. 輸入資料夾名稱
4. 點擊確定

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 主函數：`CreateNew(bool is_dir)` (行 374-414)
- 右鍵選單事件：行 766 (ID_MENU_MKDIR)
- 呼叫方式：`CreateNew(true)`

**核心邏輯**：

```cpp
std::string newPath = (current_path == "/")
    ? "/" + name
    : current_path + "/" + name;
if (fs.mkdir(newPath)) {
    RefreshList();
}
```

---

### 7. 新增檔案

**功能描述**：在當前目錄下建立新的空白檔案

**使用方式**：

1. 在空白處右鍵點擊
2. 選擇「新增檔案」
3. 輸入檔案名稱
4. 點擊確定

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 主函數：`CreateNew(bool is_dir)` (行 374-414)
- 右鍵選單事件：行 767 (ID_MENU_TOUCH)
- 呼叫方式：`CreateNew(false)`

**核心邏輯**：

```cpp
std::string newPath = (current_path == "/")
    ? "/" + name
    : current_path + "/" + name;
if (fs.create(newPath, false)) {
    RefreshList();
}
```

---

### 8. 複製檔案

**功能描述**：將選擇的檔案或資料夾複製到剪貼簿

**使用方式**：

1. 選擇要複製的檔案或資料夾
2. 右鍵點擊選擇「複製」
3. 導航到目標目錄
4. 右鍵選擇「貼上」

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 複製函數：`OnCopy()` (行 555-560)
- 貼上函數：`OnPaste()` (行 562-598)
- 右鍵選單事件：行 756 (ID_MENU_COPY)

**全域變數**：

- `clipboard_path` (行 20)：儲存複製的路徑
- `is_move_operation` (行 21)：區分複製/移動模式

**核心邏輯**：

```cpp
// 複製
clipboard_path = GetSelectedPath();
is_move_operation = false;

// 貼上
if (!is_move_operation) {
    fs.copy(clipboard_path, destPath);
}
```

---

### 9. 剪下（移動）檔案

**功能描述**：移動檔案或資料夾到新位置

**使用方式**：

1. 選擇要移動的檔案或資料夾
2. 右鍵點擊選擇「剪下（移動）」
3. 導航到目標目錄
4. 右鍵選擇「貼上」

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 剪下函數：`OnMove()` (行 600-606)
- 貼上函數：`OnPaste()` (行 562-598)
- 右鍵選單事件：行 757 (ID_MENU_CUT)

**核心邏輯**：

```cpp
// 剪下
clipboard_path = GetSelectedPath();
is_move_operation = true;

// 貼上
if (is_move_operation) {
    fs.move(clipboard_path, destPath);
}
```

---

### 10. 貼上

**功能描述**：執行複製或移動操作

**使用方式**：

- 先執行「複製」或「剪下」
- 在目標目錄右鍵選擇「貼上」
- 若剪貼簿為空，選項會變灰色

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 函數：`OnPaste()` (行 562-598)
- 右鍵選單事件：行 758 (ID_MENU_PASTE)
- 選單啟用狀態：行 198, 205 (根據 `clipboard_path.empty()` 判斷)

**錯誤處理**：

- 檢查目標名稱是否已存在
- 檢查來源和目標是否相同
- 顯示成功或失敗訊息

---

### 11. 重新命名

**功能描述**：修改檔案或資料夾的名稱

**使用方式**：

1. 選擇要重新命名的項目
2. 右鍵選擇「重新命名」
3. 在對話框中輸入新名稱（會預填舊名稱）
4. 點擊確定或取消

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 主函數：`OnRename()` (行 643-692)
- 對話框處理：`RenameDialogProc()` (行 88-124)
- 顯示對話框：`ShowRenameDialog()` (行 127-171)
- 右鍵選單事件：行 759 (ID_MENU_RENAME)

**輸入驗證**：

- 檔名不能為空
- 檔名長度限制 27 字元
- 檢查新檔名是否已存在

**對話框元件**：

```cpp
// 輸入框
CreateWindow(L"EDIT", ...);
// 確定按鈕
CreateWindow(L"BUTTON", L"確定", ...);
// 取消按鈕
CreateWindow(L"BUTTON", L"取消", ...);
```

---

### 12. 刪除

**功能描述**：刪除選擇的檔案或資料夾

**使用方式**：

1. 選擇要刪除的項目
2. 右鍵選擇「刪除」
3. 確認刪除對話框中點擊「是」

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 函數：`DeleteSelected()` (行 608-641)
- 右鍵選單事件：行 760 (ID_MENU_DELETE)

**安全機制**：

- 顯示確認對話框（MB_YESNO）
- 顯示要刪除的項目路徑
- 目錄不為空時會失敗並提示

**核心邏輯**：

```cpp
int result = MessageBox(..., MB_YESNO | MB_ICONWARNING);
if (result == IDYES) {
    if (fs.remove(target)) {
        RefreshList();
    }
}
```

---

### 13. 內容

**功能描述**：顯示檔案或資料夾的詳細資訊

**使用方式**：

1. 選擇檔案或資料夾
2. 右鍵選擇「內容」
3. 查看資訊對話框

**顯示資訊**：

- 名稱
- 類型（檔案/資料夾）
- 大小（bytes，自動轉換為 KB/MB）
- 完整路徑

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 主函數：`ShowProperties()` (行 254-283)
- 輔助函數：`CalculateDirectorySize()` (行 54-83) - 遞迴計算資料夾大小
- 右鍵選單事件：行 762 (ID_MENU_PROPERTIES)

**大小計算邏輯**：

```cpp
// 檔案：直接返回大小
if (inode.file_type == FileType::REGULAR) {
    return inode.size;
}

// 資料夾：遞迴計算所有子項目
for (const auto& entry : entries) {
    totalSize += CalculateDirectorySize(fullPath);
}
```

---

## 右鍵選單

### 14. 右鍵選單系統

**功能描述**：根據上下文顯示不同的操作選單

**使用方式**：

- 在檔案/資料夾上右鍵：顯示項目操作選單
- 在空白處右鍵：顯示目錄操作選單

**選單內容**：

#### 選擇項目時：

1. 開啟
2. ---（分隔線）---
3. 複製
4. 剪下（移動）
5. 貼上（剪貼簿為空時變灰）
6. ---（分隔線）---
7. 重新命名
8. 刪除
9. ---（分隔線）---
10. 內容

#### 空白處時：

1. 新增資料夾
2. 新增檔案
3. ---（分隔線）---
4. 貼上（剪貼簿為空時變灰）
5. ---（分隔線）---
6. 上一層（根目錄時變灰）
7. 重新整理

**程式碼位置**：

- 檔案：`src/GuiMain.cpp`
- 函數：`ShowContextMenu(HWND hwnd, int x, int y)` (行 176-222)
- 視窗訊息：WM_CONTEXTMENU (行 719-737)
- 選單 ID 定義：行 39-46

**實作邏輯**：

```cpp
// 檢查是否有選擇項目
int selectedIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
bool hasSelection = (selectedIndex != LB_ERR);

if (hasSelection) {
    // 建立項目操作選單
    AppendMenu(hMenu, MF_STRING, ID_MENU_OPEN, L"開啟");
    // ...
} else {
    // 建立目錄操作選單
    AppendMenu(hMenu, MF_STRING, ID_MENU_MKDIR, L"新增資料夾");
    // ...
}
```

---

## 程式碼架構

### 主要檔案

- **GuiMain.cpp** (約 850 行)：GUI 主程式
- **FileSystem.h/cpp**：檔案系統核心 API
- **structures.h**：資料結構定義（Inode, Superblock 等）

### 全域變數

```cpp
// src/GuiMain.cpp
FileSystem fs;                    // 檔案系統實例
std::string current_path = "/";   // 當前路徑
std::string clipboard_path = "";  // 剪貼簿路徑
bool is_move_operation = false;   // 複製/移動旗標

// GUI 控制項
HWND hList;         // 檔案列表框
HWND hPathLabel;    // 路徑標籤
HWND hStatusLabel;  // 狀態列
```

### 控制項 ID 定義

```cpp
// 列表和按鈕 ID
#define ID_LIST 101
#define ID_BTN_UP 102
// ...

// 右鍵選單 ID
#define ID_MENU_OPEN 201
#define ID_MENU_COPY 202
#define ID_MENU_CUT 203
// ...

// 工具列按鈕 ID
#define ID_TOOLBAR_UP 301
#define ID_TOOLBAR_REFRESH 302
```

### 關鍵函數總覽

| 函數名稱                   | 行號範圍 | 功能描述                   |
| -------------------------- | -------- | -------------------------- |
| `toWString()`              | 86-95    | 字串轉換（UTF-8 → UTF-16） |
| `toString()`               | 97-106   | 字串轉換（UTF-16 → UTF-8） |
| `RenameDialogProc()`       | 109-147  | 重新命名對話框處理         |
| `ShowRenameDialog()`       | 150-194  | 顯示重新命名對話框         |
| `ShowContextMenu()`        | 197-243  | 顯示右鍵選單               |
| `CalculateDirectorySize()` | 54-83    | 遞迴計算資料夾大小         |
| `ShowProperties()`         | 254-283  | 顯示檔案/資料夾內容        |
| `RefreshList()`            | 285-336  | 刷新檔案列表               |
| `OnDoubleClick()`          | 338-372  | 處理雙擊事件               |
| `CreateNew()`              | 374-414  | 新增檔案/資料夾            |
| `GetSelectedPath()`        | 524-554  | 取得選擇項目的完整路徑     |
| `OnCopy()`                 | 556-561  | 複製操作                   |
| `OnPaste()`                | 563-599  | 貼上操作                   |
| `OnMove()`                 | 601-607  | 移動操作                   |
| `DeleteSelected()`         | 609-642  | 刪除操作                   |
| `OnRename()`               | 644-693  | 重新命名操作               |
| `WindowProc()`             | 695-807  | 主視窗訊息處理             |
| `WinMain()`                | 809-850  | 程式進入點                 |

### 事件處理流程

```
使用者操作
    ↓
WM_COMMAND / WM_CONTEXTMENU
    ↓
WindowProc() 路由
    ↓
對應的處理函數（On*()）
    ↓
呼叫 FileSystem API
    ↓
RefreshList() 更新介面
```

### FileSystem API 使用

```cpp
// 檔案操作
fs.create(path, is_directory)  // 建立檔案/資料夾
fs.remove(path)                // 刪除
fs.copy(src, dest)             // 複製
fs.move(src, dest)             // 移動/重命名

// 目錄操作
fs.list(path)                  // 列出目錄內容
fs.stat(path, inode)           // 取得 inode 資訊
fs.mkdir(path)                 // 建立目錄

// 系統操作
fs.format(disk_path)           // 格式化
fs.mount(disk_path)            // 掛載
fs.unmount()                   // 卸載
```

---

## 編譯與執行

### 編譯指令

```batch
.\build.bat
```

### 執行程式

```batch
.\gui.exe
```

### 相依檔案

- `disk.img`：虛擬磁碟映像檔（16 MB）
- 編譯器：MinGW g++ (C++17)
- 平台：Windows (Win32 API)

---

## 快速參考

### 快捷操作

- **雙擊**：進入資料夾
- **右鍵**：開啟選單
- **<** 按鈕：回上一層
- **R** 按鈕：重新整理

### 常見操作流程

#### 複製檔案

1. 右鍵源檔案 → 複製
2. 導航到目標資料夾
3. 右鍵空白處 → 貼上

#### 移動檔案

1. 右鍵源檔案 → 剪下
2. 導航到目標資料夾
3. 右鍵空白處 → 貼上

#### 重新命名

1. 右鍵檔案 → 重新命名
2. 輸入新名稱
3. 確定

#### 查看屬性

1. 右鍵檔案/資料夾
2. 選擇「內容」
3. 查看詳細資訊

---

## 版本資訊

**最後更新**：2026-01-07

**主要功能**：

- ✅ 檔案瀏覽
- ✅ 建立/刪除檔案和資料夾
- ✅ 複製/移動
- ✅ 重新命名
- ✅ 右鍵選單
- ✅ 檔案屬性查看
- ✅ 遞迴資料夾大小計算

**技術特點**：

- Win32 原生 GUI
- Inode 檔案系統
- 16 MB 虛擬磁碟
- 256 個 Inode
- 4KB 區塊大小
