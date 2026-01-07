#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <ctime>
#include "FileSystem.h"

// 連結函式庫: -luser32 -lgdi32 -lcomctl32

FileSystem fs;
std::string current_path = "/";
HWND hList;
HWND hPathLabel;
HWND hStatusLabel;
HWND hBtnMkdir, hBtnTouch, hBtnDelete, hBtnUp;
HWND hBtnCopy, hBtnMove, hBtnRename, hBtnRefresh;
std::string clipboard_path = ""; // 用於複製/移動操作
bool is_move_operation = false; // true 表示移動，false 表示複製
std::wstring g_rename_buffer; // 用於重命名對話框的輸入
std::string g_edit_file_path = ""; // 正在編輯的檔案路徑
HWND g_edit_window = NULL; // 編輯視窗句柄

// 控制項 ID 定義
#define ID_LIST 101
#define ID_BTN_UP 102
#define ID_BTN_MKDIR 103
#define ID_BTN_TOUCH 104
#define ID_BTN_DELETE 105
#define ID_BTN_COPY 106
#define ID_BTN_MOVE 107
#define ID_BTN_RENAME 108
#define ID_BTN_REFRESH 109
#define ID_BTN_REAL_RENAME 110

// 右鍵選單項目 ID
#define ID_MENU_OPEN 201
#define ID_MENU_COPY 202
#define ID_MENU_CUT 203
#define ID_MENU_PASTE 204
#define ID_MENU_RENAME 205
#define ID_MENU_DELETE 206
#define ID_MENU_REFRESH 207
#define ID_MENU_MKDIR 208
#define ID_MENU_TOUCH 209
#define ID_MENU_UP 210
#define ID_MENU_PROPERTIES 211
#define ID_MENU_EDIT 212

// 工具列按鈕 ID
#define ID_TOOLBAR_UP 301
#define ID_TOOLBAR_REFRESH 302

// 函數前向聲明
std::string GetSelectedPath();
void RefreshList();

// 遞迴計算資料夾總大小
uint32_t CalculateDirectorySize(const std::string& path) {
    Inode inode;
    if (!fs.stat(path, inode)) {
        return 0;
    }
    
    // 如果是檔案，直接返回大小
    if (inode.file_type == FileType::REGULAR) {
        return inode.size;
    }
    
    // 如果是資料夾，遞迴計算所有子項目
    if (inode.file_type == FileType::DIRECTORY) {
        uint32_t totalSize = 0;
        std::vector<std::string> entries = fs.list(path);
        
        for (const auto& entry : entries) {
            // 跳過 . 和 ..
            if (entry == "." || entry == "..") continue;
            
            std::string fullPath = (path == "/") ? "/" + entry : path + "/" + entry;
            totalSize += CalculateDirectorySize(fullPath);
        }
        
        return totalSize;
    }
    
    return 0;
}

// 輔助函數：將 std::string 轉換為 std::wstring
std::wstring toWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// 將 wstring 轉換為 string
std::string toString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// 編輯視窗的視窗處理程序
LRESULT CALLBACK EditWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit = NULL;
    
    switch (uMsg) {
    case WM_CREATE:
        {
            // 建立編輯框
            hEdit = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
                0, 0, 800, 600,
                hwnd, (HMENU)1, NULL, NULL
            );
            
            // 設定字型
            HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
            SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            // 讀取檔案內容
            if (!g_edit_file_path.empty()) {
                char buffer[16384]; // 最大 16KB
                int bytesRead = fs.read(g_edit_file_path, buffer, sizeof(buffer));
                
                if (bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    std::wstring content = toWString(std::string(buffer, bytesRead));
                    SetWindowText(hEdit, content.c_str());
                } else if (bytesRead == 0) {
                    SetWindowText(hEdit, L"");
                } else {
                    MessageBox(hwnd, L"讀取檔案失敗", L"錯誤", MB_OK | MB_ICONERROR);
                }
            }
        }
        return 0;
        
    case WM_SIZE:
        if (hEdit) {
            RECT rect;
            GetClientRect(hwnd, &rect);
            MoveWindow(hEdit, 0, 0, rect.right, rect.bottom, TRUE);
        }
        return 0;
        
    case WM_KEYDOWN:
        // 處理 Ctrl+S
        if (wParam == 'S' && GetKeyState(VK_CONTROL) < 0) {
            // 儲存檔案
            int textLen = GetWindowTextLength(hEdit);
            if (textLen > 16384 - 1) {
                MessageBox(hwnd, L"檔案內容超過最大大小限制 (16KB)", L"錯誤", MB_OK | MB_ICONERROR);
                return 0;
            }
            
            wchar_t* wbuffer = new wchar_t[textLen + 1];
            GetWindowText(hEdit, wbuffer, textLen + 1);
            std::string content = toString(std::wstring(wbuffer));
            delete[] wbuffer;
            
            int bytesWritten = fs.write(g_edit_file_path, content.c_str(), content.length());
            
            if (bytesWritten >= 0) {
                MessageBox(hwnd, L"儲存成功", L"成功", MB_OK | MB_ICONINFORMATION);
                SetWindowText(hwnd, (L"編輯檔案 - " + toWString(g_edit_file_path) + L" (已儲存)").c_str());
            } else {
                MessageBox(hwnd, L"儲存失敗", L"錯誤", MB_OK | MB_ICONERROR);
            }
            return 0;
        }
        break;
        
    case WM_CLOSE:
        {
            int result = MessageBox(hwnd, L"是否儲存變更？", L"關閉編輯器", MB_YESNOCANCEL | MB_ICONQUESTION);
            if (result == IDCANCEL) {
                return 0;
            } else if (result == IDYES) {
                // 儲存檔案
                int textLen = GetWindowTextLength(hEdit);
                if (textLen <= 16384 - 1) {
                    wchar_t* wbuffer = new wchar_t[textLen + 1];
                    GetWindowText(hEdit, wbuffer, textLen + 1);
                    std::string content = toString(std::wstring(wbuffer));
                    delete[] wbuffer;
                    
                    fs.write(g_edit_file_path, content.c_str(), content.length());
                }
            }
            DestroyWindow(hwnd);
            g_edit_window = NULL;
            g_edit_file_path.clear();
        }
        return 0;
        
    case WM_DESTROY:
        g_edit_window = NULL;
        g_edit_file_path.clear();
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 開啟檔案編輯視窗
void OpenFileEditor(const std::string& filePath) {
    // 檢查是否為檔案
    Inode inode;
    if (!fs.stat(filePath, inode)) {
        MessageBox(NULL, L"無法取得檔案資訊", L"錯誤", MB_OK | MB_ICONERROR);
        return;
    }
    
    if (inode.file_type != FileType::REGULAR) {
        MessageBox(NULL, L"只能編輯檔案", L"錯誤", MB_OK | MB_ICONERROR);
        return;
    }
    
    // 如果已有編輯視窗開啟，先關閉
    if (g_edit_window != NULL) {
        SetForegroundWindow(g_edit_window);
        MessageBox(NULL, L"已有編輯視窗開啟", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    g_edit_file_path = filePath;
    
    // 註冊視窗類別
    static bool classRegistered = false;
    const wchar_t EDIT_CLASS[] = L"MiniFS_EditWindow";
    
    if (!classRegistered) {
        WNDCLASS wc = { };
        wc.lpfnWndProc = EditWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = EDIT_CLASS;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        RegisterClass(&wc);
        classRegistered = true;
    }
    
    // 建立編輯視窗
    g_edit_window = CreateWindowEx(
        0,
        EDIT_CLASS,
        (L"編輯檔案 - " + toWString(filePath)).c_str(),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 820, 650,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    
    if (!g_edit_window) {
        MessageBox(NULL, L"無法建立編輯視窗", L"錯誤", MB_OK | MB_ICONERROR);
        g_edit_file_path.clear();
    }
}

// 重命名對話框視窗處理程序
LRESULT CALLBACK RenameDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit = NULL;
    
    switch (uMsg) {
    case WM_CREATE:
        {
            // 建立標籤
            CreateWindow(L"STATIC", L"請輸入新名稱：",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, 10, 280, 20, hwnd, NULL, NULL, NULL);
            
            // 建立輸入框
            hEdit = CreateWindow(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                10, 35, 280, 25, hwnd, (HMENU)1001, NULL, NULL);
            
            // 設定初始文字
            SetWindowText(hEdit, g_rename_buffer.c_str());
            SetFocus(hEdit);
            SendMessage(hEdit, EM_SETSEL, 0, -1); // 全選文字
            
            // 建立按鈕
            CreateWindow(L"BUTTON", L"確定",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                80, 70, 70, 30, hwnd, (HMENU)IDOK, NULL, NULL);
            
            CreateWindow(L"BUTTON", L"取消",
                WS_CHILD | WS_VISIBLE,
                160, 70, 70, 30, hwnd, (HMENU)IDCANCEL, NULL, NULL);
        }
        return 0;
        
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            // 取得輸入的文字
            wchar_t buffer[256];
            GetWindowText(hEdit, buffer, 256);
            g_rename_buffer = buffer;
            DestroyWindow(hwnd);
            return 0;
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            g_rename_buffer.clear();
            DestroyWindow(hwnd);
            return 0;
        }
        break;
        
    case WM_CLOSE:
        g_rename_buffer.clear();
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 顯示重命名對話框
bool ShowRenameDialog(HWND parent, const std::wstring& oldName, std::wstring& newName) {
    static bool classRegistered = false;
    const wchar_t DIALOG_CLASS[] = L"RenameDialogClass";
    
    if (!classRegistered) {
        WNDCLASS wc = { };
        wc.lpfnWndProc = RenameDialogProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = DIALOG_CLASS;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClass(&wc);
        classRegistered = true;
    }
    
    g_rename_buffer = oldName;
    
    HWND hDialog = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        DIALOG_CLASS,
        L"重命名",
        WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 320, 150,
        parent, NULL, GetModuleHandle(NULL), NULL
    );
    
    if (!hDialog) return false;
    
    // 簡單的模擬對話框迴圈
    EnableWindow(parent, FALSE);
    
    MSG msg;
    bool result = false;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsWindow(hDialog)) break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        
        // 檢查對話框是否關閉
        if (!IsWindow(hDialog)) {
            if (!g_rename_buffer.empty()) {
                newName = g_rename_buffer;
                result = true;
            }
            break;
        }
    }
    
    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);
    
    return result;
}

// 顯示右鍵選單
void ShowContextMenu(HWND hwnd, int x, int y) {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;
    
    // 檢查是否有選擇項目
    int selectedIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
    bool hasSelection = (selectedIndex != LB_ERR);
    
    if (hasSelection) {
        // 有選擇項目時的選單
        AppendMenu(hMenu, MF_STRING, ID_MENU_OPEN, L"開啟");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        
        // 檢查是否為檔案，只有檔案才顯示編輯選項
        std::string selectedPath = GetSelectedPath();
        Inode inode;
        bool isFile = false;
        if (fs.stat(selectedPath, inode)) {
            isFile = (inode.file_type == FileType::REGULAR);
        }
        
        if (isFile) {
            AppendMenu(hMenu, MF_STRING, ID_MENU_EDIT, L"編輯");
            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        }
        
        AppendMenu(hMenu, MF_STRING, ID_MENU_COPY, L"複製");
        AppendMenu(hMenu, MF_STRING, ID_MENU_CUT, L"剪下（移動）");
        AppendMenu(hMenu, MF_STRING | (clipboard_path.empty() ? MF_GRAYED : 0), ID_MENU_PASTE, L"貼上");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING, ID_MENU_RENAME, L"重新命名");
        AppendMenu(hMenu, MF_STRING, ID_MENU_DELETE, L"刪除");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING, ID_MENU_PROPERTIES, L"內容");
    } else {
        // 無選擇時的選單
        // 建立「新增」子選單
        HMENU hNewMenu = CreatePopupMenu();
        AppendMenu(hNewMenu, MF_STRING, ID_MENU_MKDIR, L"資料夾");
        AppendMenu(hNewMenu, MF_STRING, ID_MENU_TOUCH, L"檔案");
        
        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hNewMenu, L"新增");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING | (clipboard_path.empty() ? MF_GRAYED : 0), ID_MENU_PASTE, L"貼上");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING | (current_path == "/" ? MF_GRAYED : 0), ID_MENU_UP, L"上一層");
        AppendMenu(hMenu, MF_STRING, ID_MENU_REFRESH, L"重新整理");
    }
    
    // 顯示選單並等待用戶選擇
    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
                   x, y, 0, hwnd, NULL);
    
    DestroyMenu(hMenu);
}

// 顯示檔案/資料夾屬性
void ShowProperties() {
    std::string target = GetSelectedPath();
    if (target.empty()) {
        MessageBox(NULL, L"請選擇一個檔案或資料夾", L"內容", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    Inode inode;
    if (!fs.stat(target, inode)) {
        MessageBox(NULL, L"無法取得檔案資訊", L"錯誤", MB_OK | MB_ICONERROR);
        return;
    }
    
    // 提取檔名
    std::string name = target;
    size_t lastSlash = target.find_last_of('/');
    if (lastSlash != std::string::npos && lastSlash < target.length() - 1) {
        name = target.substr(lastSlash + 1);
    }
    
    // 建立屬性訊息
    std::wstring message;
    message += L"名稱: " + toWString(name) + L"\n";
    message += L"類型: " + std::wstring((inode.file_type == FileType::DIRECTORY) ? L"資料夾" : L"檔案") + L"\n";
    
    // Inode 資訊
    message += L"Inode 編號: " + std::to_wstring(inode.inode_num) + L"\n";
    
    // 計算佔用區塊數量
    int blockCount = 0;
    for (int i = 0; i < Config::DIRECT_BLOCKS; i++) {
        if (inode.direct_blks[i] != 0) {
            blockCount++;
        }
    }
    message += L"佔用區塊: " + std::to_wstring(blockCount) + L" 個\n";
    
    // 磁碟佔用空間
    uint32_t diskSpace = blockCount * Config::BLOCK_SIZE;
    message += L"磁碟空間: " + std::to_wstring(diskSpace) + L" bytes";
    if (diskSpace >= 1024) {
        double kb = diskSpace / 1024.0;
        wchar_t buf[50];
        swprintf(buf, 50, L" (%.2f KB)", kb);
        message += buf;
    }
    message += L"\n";
    
    // 資料夾項目數量
    if (inode.file_type == FileType::DIRECTORY) {
        std::vector<std::string> entries = fs.list(target);
        int itemCount = 0;
        int folderCount = 0;
        int fileCount = 0;
        
        for (const auto& entry : entries) {
            if (entry == "." || entry == "..") continue;
            itemCount++;
            
            std::string fullPath = (target == "/") ? "/" + entry : target + "/" + entry;
            Inode childInode;
            if (fs.stat(fullPath, childInode)) {
                if (childInode.file_type == FileType::DIRECTORY) {
                    folderCount++;
                } else {
                    fileCount++;
                }
            }
        }
        
        message += L"包含: " + std::to_wstring(itemCount) + L" 個項目 (" 
                   + std::to_wstring(folderCount) + L" 個資料夾, " 
                   + std::to_wstring(fileCount) + L" 個檔案)\n";
    }
    
    message += L"\n";
    
    // 計算實際大小（資料夾會遞迴計算總大小）
    uint32_t actualSize = (inode.file_type == FileType::DIRECTORY) 
                          ? CalculateDirectorySize(target) 
                          : inode.size;
    
    message += L"大小: " + std::to_wstring(actualSize) + L" bytes\n";
    
    // 轉換為 KB/MB
    if (actualSize >= 1024 * 1024) {
        double mb = actualSize / (1024.0 * 1024.0);
        wchar_t buf[50];
        swprintf(buf, 50, L"      (%.2f MB)\n", mb);
        message += buf;
    } else if (actualSize >= 1024) {
        double kb = actualSize / 1024.0;
        wchar_t buf[50];
        swprintf(buf, 50, L"      (%.2f KB)\n", kb);
        message += buf;
    }
    
    message += L"\n路徑: " + toWString(target);
    
    // 顯示屬性對話框
    MessageBox(NULL, message.c_str(), L"內容", MB_OK | MB_ICONINFORMATION);
}

void RefreshList() {
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    
    // 如果不是根目錄，加入 ".." 項目
    if (current_path != "/") {
        SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"[..]");
    }

    int fileCount = 0, dirCount = 0;
    uint32_t totalSize = 0;

    auto files = fs.list(current_path);
    for (const auto& f : files) {
        if (f == "." || f == "..") continue;
        
        std::string full_path = current_path;
        if (full_path.back() != '/') full_path += "/";
        full_path += f;

        Inode inode;
        if (fs.stat(full_path, inode)) {
            std::string display;
            if (inode.isDirectory()) {
                display = "[" + f + "]";
                dirCount++;
            } else {
                char sizeStr[32];
                if (inode.size < 1024) {
                    snprintf(sizeStr, sizeof(sizeStr), "%u B", inode.size);
                } else if (inode.size < 1024 * 1024) {
                    snprintf(sizeStr, sizeof(sizeStr), "%.1f KB", inode.size / 1024.0);
                } else {
                    snprintf(sizeStr, sizeof(sizeStr), "%.1f MB", inode.size / (1024.0 * 1024.0));
                }
                display = f + std::string(30 - f.length(), ' ') + sizeStr;
                fileCount++;
                totalSize += inode.size;
            }
            SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)toWString(display).c_str());
        }
    }
    
    SetWindowText(hPathLabel, toWString("當前路徑: " + current_path).c_str());
    
    char statusStr[128];
    snprintf(statusStr, sizeof(statusStr), "總計: %d 個資料夾, %d 個檔案 | 大小: %.2f KB", 
             dirCount, fileCount, totalSize / 1024.0);
    SetWindowText(hStatusLabel, toWString(statusStr).c_str());
}

void Navigate(const std::string& new_path) {
    Inode inode;
    if (fs.stat(new_path, inode) && inode.isDirectory()) {
        current_path = new_path;
        RefreshList();
    } else {
        MessageBox(NULL, L"Cannot navigate: Not a directory or not found.", L"Error", MB_OK);
    }
}

// 檢視器視窗處理程序
LRESULT CALLBACK ViewerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_SIZE:
        {
            // 調整編輯控制項大小以適應客戶區
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            HWND hEdit = GetDlgItem(hwnd, 1001); // 編輯控制項的 ID 為 1001
            SetWindowPos(hEdit, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);
        }
        return 0;
    case WM_CLOSE:
        // 不結束應用程式，只關閉此檢視器視窗
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ShowFileViewer(const std::string& filename, const std::string& content) {
    static bool classRegistered = false;
    const wchar_t VIEWER_CLASS[] = L"MiniFS_Viewer_Class";

    if (!classRegistered) {
        WNDCLASS wc = { };
        wc.lpfnWndProc = ViewerWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = VIEWER_CLASS;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClass(&wc);
        classRegistered = true;
    }

    std::wstring wTitle = toWString("Viewer: " + filename);
    HWND hViewer = CreateWindowEx(
        0, VIEWER_CLASS, wTitle.c_str(),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, // WS_OVERLAPPEDWINDOW gives resize, min/max, close
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (hViewer) {
        // 建立唯讀編輯控制項來顯示內容
        // 將內容轉換為 CRLF 格式以符合 Windows 編輯控制項需求
        // 標準 Windows 編輯控制項偏好 \r\n 換行符號
        // 快速處理確保 \n 轉換為 \r\n
        std::string safeContent;
        safeContent.reserve(content.size() + content.size() / 10);
        for (size_t i = 0; i < content.size(); ++i) {
            if (content[i] == '\n' && (i == 0 || content[i-1] != '\r')) {
                safeContent += "\r\n";
            } else {
                safeContent += content[i];
            }
        }

        std::wstring wContent = toWString(safeContent);

        CreateWindow(
            L"EDIT", wContent.c_str(),
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | 
            ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
            0, 0, 800, 600, // 初始大小，將由 WM_SIZE 調整
            hViewer, (HMENU)1001, GetModuleHandle(NULL), NULL
        );
        
        // 觸發初始調整以正確適配大小
        RECT rc;
        GetClientRect(hViewer, &rc);
        HWND hEdit = GetDlgItem(hViewer, 1001);
        MoveWindow(hEdit, 0, 0, rc.right, rc.bottom, TRUE);
        
        // 設定等寬字型以便查看程式碼/文字
        HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                                 DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
}

void OnDoubleClick() {
    int idx = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
    if (idx != LB_ERR) {
        wchar_t buffer[256];
        SendMessage(hList, LB_GETTEXT, idx, (LPARAM)buffer);
        std::wstring item = buffer;
        std::string name = toString(item);

        if (name == "[..]") {
             // 返回上一層目錄
             if (current_path == "/") return;
             size_t last = current_path.find_last_of('/');
             if (last == 0) current_path = "/";
             else current_path = current_path.substr(0, last);
             RefreshList();
             return;
        }

        // 檢查是目錄還是檔案
        bool isDir = (name.find('[') != std::string::npos && name.find(']') != std::string::npos);
        std::string actualName;
        if (isDir) {
            size_t start = name.find('[') + 1;
            size_t end = name.find(']');
            actualName = name.substr(start, end - start);
        } else {
            // 提取檔名（在大小資訊之前，以空格分隔）
            size_t spacePos = name.find("  ");
            actualName = (spacePos != std::string::npos) ? name.substr(0, spacePos) : name;
            while (!actualName.empty() && actualName.back() == ' ') actualName.pop_back();
        }
        
        std::string target = (current_path == "/") ? "/" + actualName : current_path + "/" + actualName;

        if (isDir) {
            Navigate(target);
        } else {
            // 開啟檔案編輯器
            OpenFileEditor(target);
        }
    }
}

void CreateNew(bool isDir) {
    char buf[100] = {0};
    // 純 Win32 輸入對話框較複雜，直接使用預設名稱
    // 建立 "NewFolder" 或 "NewFile"
    std::string base = isDir ? "NewFolder" : "NewFile";
    std::string name = base;
    
    // 檢查名稱衝突的簡單迴圈
    int i = 1;
    while(true) {
        std::string target = (current_path == "/") ? "/" + name : current_path + "/" + name;
        Inode inode;
        if (!fs.stat(target, inode)) break; // 找到可用的名稱
        name = base + std::to_string(i++);
    }
    
    std::string target = (current_path == "/") ? "/" + name : current_path + "/" + name;
    if (isDir) fs.mkdir(target);
    else fs.create(target, false);
    
    RefreshList();
}

std::string GetSelectedPath() {
    int idx = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
    if (idx == LB_ERR) return "";
    
    wchar_t buffer[256];
    SendMessage(hList, LB_GETTEXT, idx, (LPARAM)buffer);
    std::wstring item = buffer;
    std::string name = toString(item);
    
    if (name == "[..]" || name.empty()) return "";
    
    // 提取名稱，跳過大小資訊
    bool isDir = (name.find('[') != std::string::npos && name.find(']') != std::string::npos);
    std::string actualName;
    if (isDir) {
        size_t start = name.find('[') + 1;
        size_t end = name.find(']');
        actualName = name.substr(start, end - start);
    } else {
        // 尋找第一組空格序列（30個以上）
        size_t spacePos = name.find("  ");
        actualName = (spacePos != std::string::npos) ? name.substr(0, spacePos) : name;
        // 移除尾隨空格
        while (!actualName.empty() && actualName.back() == ' ') actualName.pop_back();
    }
    
    return (current_path == "/") ? "/" + actualName : current_path + "/" + actualName;
}

void OnCopy() {
    clipboard_path = GetSelectedPath();
    if (!clipboard_path.empty()) {
        is_move_operation = false;
        MessageBox(NULL, toWString("已複製: " + clipboard_path + "\n\n請導航到目標資料夾後點擊 [貼上] 按鈕").c_str(), L"複製", MB_OK | MB_ICONINFORMATION);
    }
}

void OnPaste() {
    if (clipboard_path.empty()) {
        MessageBox(NULL, L"剪貼簿是空的", L"貼上", MB_OK | MB_ICONWARNING);
        return;
    }
    
    // 從剪貼簿路徑中提取檔名
    size_t lastSlash = clipboard_path.find_last_of('/');
    std::string filename = (lastSlash != std::string::npos) ? clipboard_path.substr(lastSlash + 1) : clipboard_path;
    std::string dest = (current_path == "/") ? "/" + filename : current_path + "/" + filename;
    
    if (is_move_operation) {
        // 移動操作：不允許重名
        if (fs.move(clipboard_path, dest)) {
            MessageBox(NULL, L"移動成功", L"成功", MB_OK | MB_ICONINFORMATION);
            clipboard_path = "";
            is_move_operation = false;
            RefreshList();
        } else {
            MessageBox(NULL, L"移動失敗 (可能目標已存在或來源不存在)", L"錯誤", MB_OK | MB_ICONERROR);
        }
    } else {
        // 複製操作：自動處理重名
        int counter = 1;
        while (true) {
            Inode inode;
            if (!fs.stat(dest, inode)) break;
            
            size_t dotPos = filename.find_last_of('.');
            if (dotPos != std::string::npos) {
                std::string name = filename.substr(0, dotPos);
                std::string ext = filename.substr(dotPos);
                dest = (current_path == "/") ? "/" + name + "_copy" + std::to_string(counter) + ext 
                                              : current_path + "/" + name + "_copy" + std::to_string(counter) + ext;
            } else {
                dest = (current_path == "/") ? "/" + filename + "_copy" + std::to_string(counter)
                                              : current_path + "/" + filename + "_copy" + std::to_string(counter);
            }
            counter++;
        }
        
        if (fs.copy(clipboard_path, dest)) {
            MessageBox(NULL, L"複製成功", L"成功", MB_OK | MB_ICONINFORMATION);
            RefreshList();
        } else {
            MessageBox(NULL, L"複製失敗", L"錯誤", MB_OK | MB_ICONERROR);
        }
    }
}

void OnMove() {
    std::string src = GetSelectedPath();
    if (src.empty()) return;
    
    clipboard_path = src;
    is_move_operation = true;
    MessageBox(NULL, toWString("準備移動: " + src + "\n\n請導航到目標資料夾後點擊 [貼上] 按鈕").c_str(), 
               L"移動", MB_OK | MB_ICONINFORMATION);
}

void OnMoveExecute() {
    if (clipboard_path.empty()) {
        MessageBox(NULL, L"沒有選擇要移動的項目", L"移動", MB_OK | MB_ICONWARNING);
        return;
    }
    
    size_t lastSlash = clipboard_path.find_last_of('/');
    std::string filename = (lastSlash != std::string::npos) ? clipboard_path.substr(lastSlash + 1) : clipboard_path;
    std::string dest = (current_path == "/") ? "/" + filename : current_path + "/" + filename;
    
    if (fs.move(clipboard_path, dest)) {
        MessageBox(NULL, L"移動成功", L"成功", MB_OK | MB_ICONINFORMATION);
        clipboard_path = "";
        RefreshList();
    } else {
        MessageBox(NULL, L"移動失敗 (可能目標已存在)", L"錯誤", MB_OK | MB_ICONERROR);
    }
}

void OnRename() {
    std::string src = GetSelectedPath();
    if (src.empty()) {
        MessageBox(NULL, L"請先選擇要重命名的檔案或資料夾", L"重命名", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // 提取舊檔名
    size_t lastSlash = src.find_last_of('/');
    std::string oldName = (lastSlash != std::string::npos) ? src.substr(lastSlash + 1) : src;
    std::string parentPath = (lastSlash == 0) ? "/" : (lastSlash != std::string::npos ? src.substr(0, lastSlash) : "/");
    
    // 顯示重命名對話框
    std::wstring wOldName = toWString(oldName);
    std::wstring wNewName;
    
    HWND hMainWnd = FindWindow(L"MiniFS_GUI_Class", NULL);
    if (!ShowRenameDialog(hMainWnd, wOldName, wNewName)) {
        return; // 使用者取消
    }
    
    std::string newName = toString(wNewName);
    
    // 檢查新檔名是否為空或與舊檔名相同
    if (newName.empty()) {
        MessageBox(NULL, L"檔名不能為空", L"錯誤", MB_OK | MB_ICONERROR);
        return;
    }
    
    if (newName == oldName) {
        return; // 沒有變更
    }
    
    // 檢查檔名長度
    if (newName.length() > 27) { // MAX_FILENAME_LENGTH - 1
        MessageBox(NULL, L"檔名太長（最多 27 個字元）", L"錯誤", MB_OK | MB_ICONERROR);
        return;
    }
    
    // 建構新路徑
    std::string dest = (parentPath == "/") ? "/" + newName : parentPath + "/" + newName;
    
    // 執行重命名（使用 move 函數）
    if (fs.move(src, dest)) {
        MessageBox(NULL, L"重命名成功", L"成功", MB_OK | MB_ICONINFORMATION);
        RefreshList();
    } else {
        MessageBox(NULL, L"重命名失敗（目標名稱可能已存在）", L"錯誤", MB_OK | MB_ICONERROR);
    }
}

void DeleteSelected() {
    std::string target = GetSelectedPath();
    if (target.empty()) return;
    
    int result = MessageBox(NULL, toWString("確定要刪除嗎？\n\n" + target).c_str(), 
                           L"確認刪除", MB_YESNO | MB_ICONWARNING);
    
    if (result == IDYES) {
        if (fs.remove(target)) {
            RefreshList();
            MessageBox(NULL, L"刪除成功", L"成功", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBox(NULL, L"刪除失敗 (目錄不為空?)", L"錯誤", MB_OK | MB_ICONERROR);
        }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        {
            // 工具列按鈕
            CreateWindow(L"BUTTON", L"<",  // 上一層
                WS_CHILD | WS_VISIBLE, 10, 10, 40, 25, hwnd, (HMENU)ID_TOOLBAR_UP, NULL, NULL);
            
            CreateWindow(L"BUTTON", L"R",  // 重新整理 (Refresh)
                WS_CHILD | WS_VISIBLE, 55, 10, 40, 25, hwnd, (HMENU)ID_TOOLBAR_REFRESH, NULL, NULL);
            
            // 路徑標籤
            hPathLabel = CreateWindow(L"STATIC", L"當前路徑: /", 
                WS_CHILD | WS_VISIBLE | SS_LEFT, 105, 12, 585, 20, hwnd, NULL, NULL, NULL);
            
            // 檔案列表框（擴大至底部）
            hList = CreateWindow(L"LISTBOX", NULL, 
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 
                10, 40, 680, 500, hwnd, (HMENU)ID_LIST, NULL, NULL);
            
            // 狀態列
            hStatusLabel = CreateWindow(L"STATIC", L"總計: 0 個資料夾, 0 個檔案", 
                WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 550, 680, 20, hwnd, NULL, NULL, NULL);
            
            RefreshList();
        }
        break;

    case WM_CONTEXTMENU:
        {
            HWND hTarget = (HWND)wParam;
            // 只處理檔案列表的右鍵選單
            if (hTarget == hList || hTarget == hwnd) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                
                // 如果是鍵盤觸發（lParam == -1），使用當前游標位置
                if (lParam == (LPARAM)-1) {
                    POINT pt;
                    GetCursorPos(&pt);
                    x = pt.x;
                    y = pt.y;
                }
                
                ShowContextMenu(hwnd, x, y);
            }
        }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_LIST && HIWORD(wParam) == LBN_DBLCLK) {
            OnDoubleClick();
        }
        // 處理右鍵選單命令
        else if (LOWORD(wParam) == ID_MENU_OPEN) OnDoubleClick();
        else if (LOWORD(wParam) == ID_MENU_EDIT) {
            std::string path = GetSelectedPath();
            if (!path.empty()) {
                OpenFileEditor(path);
            }
        }
        else if (LOWORD(wParam) == ID_MENU_COPY) OnCopy();
        else if (LOWORD(wParam) == ID_MENU_CUT) OnMove();
        else if (LOWORD(wParam) == ID_MENU_PASTE) OnPaste();
        else if (LOWORD(wParam) == ID_MENU_RENAME) OnRename();
        else if (LOWORD(wParam) == ID_MENU_DELETE) DeleteSelected();
        else if (LOWORD(wParam) == ID_MENU_REFRESH) RefreshList();
        else if (LOWORD(wParam) == ID_MENU_PROPERTIES) ShowProperties();
        else if (LOWORD(wParam) == ID_MENU_MKDIR) CreateNew(true);
        else if (LOWORD(wParam) == ID_MENU_TOUCH) CreateNew(false);
        else if (LOWORD(wParam) == ID_MENU_UP) {
            if (current_path != "/") {
                size_t last = current_path.find_last_of('/');
                if (last == 0) current_path = "/";
                else current_path = current_path.substr(0, last);
                RefreshList();
            }
        }
        // 處理工具列按鈕
        else if (LOWORD(wParam) == ID_TOOLBAR_UP) {
            if (current_path != "/") {
                size_t last = current_path.find_last_of('/');
                if (last == 0) current_path = "/";
                else current_path = current_path.substr(0, last);
                RefreshList();
            }
        }
        else if (LOWORD(wParam) == ID_TOOLBAR_REFRESH) {
            RefreshList();
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
            EndPaint(hwnd, &ps);
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR pCmdLine, int nCmdShow) {
    if (!fs.mount("disk.img")) {
        if (!fs.format("disk.img", false)) {
            MessageBox(NULL, L"Failed to initialize disk.img", L"Error", MB_ICONERROR);
            return 1;
        }
    }

    const wchar_t CLASS_NAME[] = L"MiniFS_GUI_Class";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"MiniFS 檔案管理器 - Inode 檔案系統",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 720, 620,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    fs.unmount();
    return 0;
}
