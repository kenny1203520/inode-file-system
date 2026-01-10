#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include "FileSystem.h"

// Link with: -luser32 -lgdi32 -lcomctl32

FileSystem fs;
std::string current_path = "/";
HWND hList;
HWND hPathLabel;
HWND hBtnMkdir, hBtnTouch, hBtnDelete, hBtnUp;

// IDs
#define ID_LIST 101
#define ID_BTN_UP 102
#define ID_BTN_MKDIR 103
#define ID_BTN_TOUCH 104
#define ID_BTN_DELETE 105

// Helper to convert std::string to std::wstring
std::wstring toWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Convert wstring to string
std::string toString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

void RefreshList() {
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    
    // Add ".." if not root
    if (current_path != "/") {
        SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)L"[..]");
    }

    auto files = fs.list(current_path);
    for (const auto& f : files) {
        if (f == "." || f == "..") continue;
        
        std::string full_path = current_path;
        if (full_path.back() != '/') full_path += "/";
        full_path += f;

        Inode inode;
        if (fs.stat(full_path, inode)) {
            std::string display = f;
            if (inode.isDirectory()) display = "[" + display + "]";
            SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)toWString(display).c_str());
        }
    }
    
    SetWindowText(hPathLabel, toWString("Path: " + current_path).c_str());
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

// Viewer Window Procedure
LRESULT CALLBACK ViewerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_SIZE:
        {
            // Resize the edit control to fit the client area
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            HWND hEdit = GetDlgItem(hwnd, 1001); // ID 1001 for the edit control
            SetWindowPos(hEdit, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);
        }
        return 0;
    case WM_CLOSE:
        // Do not quit the app, just close this viewer window
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
        // Create Read-Only Edit Control for content
        // Convert content to CRLF for Windows Edit control if needed, but Edit usually handles LF okay-ish, 
        // strictly standard windows edit control likes \r\n. 
        // Let's do a quick pass to ensure \n -> \r\n if strictly needed, but often \r\n is safer.
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
            0, 0, 800, 600, // Initial size, will be resized by WM_SIZE
            hViewer, (HMENU)1001, GetModuleHandle(NULL), NULL
        );
        
        // Trigger a resize to fit correctly initially
        RECT rc;
        GetClientRect(hViewer, &rc);
        HWND hEdit = GetDlgItem(hViewer, 1001);
        MoveWindow(hEdit, 0, 0, rc.right, rc.bottom, TRUE);
        
        // Set font to fixed width for code/text viewing
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
             // Go up
             if (current_path == "/") return;
             size_t last = current_path.find_last_of('/');
             if (last == 0) current_path = "/";
             else current_path = current_path.substr(0, last);
             RefreshList();
             return;
        }

        // Check if dir or file
        bool isDir = (name.size() > 2 && name.front() == '[' && name.back() == ']');
        std::string actualName = isDir ? name.substr(1, name.size() - 2) : name;
        
        std::string target = (current_path == "/") ? "/" + actualName : current_path + "/" + actualName;

        if (isDir) {
            Navigate(target);
        } else {
            // Show content
            Inode inode;
            if (fs.stat(target, inode)) {
                 std::vector<char> buf(inode.size + 1);
                 int r = fs.read(target, buf.data(), inode.size);
                 if (r >= 0) {
                     buf[r] = 0;
                     // MessageBoxA(NULL, buf.data(), actualName.c_str(), MB_OK);
                     ShowFileViewer(actualName, buf.data());
                 }
            }
        }
    }
}

void CreateNew(bool isDir) {
    char buf[100] = {0};
    // Input dialog is complex in pure Win32, simulating with hardcoded name or simple prompt?
    // Let's just create "New Folder" or "New File"
    std::string base = isDir ? "NewFolder" : "NewFile";
    std::string name = base;
    
    // Check collision simple loop
    int i = 1;
    while(true) {
        std::string target = (current_path == "/") ? "/" + name : current_path + "/" + name;
        Inode inode;
        if (!fs.stat(target, inode)) break; // Found free name
        name = base + std::to_string(i++);
    }
    
    std::string target = (current_path == "/") ? "/" + name : current_path + "/" + name;
    if (isDir) fs.mkdir(target);
    else fs.create(target, false);
    
    RefreshList();
}

void DeleteSelected() {
    int idx = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
    if (idx != LB_ERR) {
        wchar_t buffer[256];
        SendMessage(hList, LB_GETTEXT, idx, (LPARAM)buffer);
        std::wstring item = buffer;
        std::string name = toString(item);
        
        if (name == "[..]") return;

        bool isDir = (name.size() > 2 && name.front() == '[' && name.back() == ']');
        std::string actualName = isDir ? name.substr(1, name.size() - 2) : name;
        std::string target = (current_path == "/") ? "/" + actualName : current_path + "/" + actualName;
        
        if (fs.remove(target)) {
            RefreshList();
        } else {
            MessageBox(NULL, L"Delete failed (Not empty?)", L"Error", MB_OK);
        }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        hPathLabel = CreateWindow(L"STATIC", L"Path: /", 
            WS_CHILD | WS_VISIBLE, 10, 10, 560, 20, hwnd, NULL, NULL, NULL);
        
        hList = CreateWindow(L"LISTBOX", NULL, 
            WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY, 
            10, 40, 560, 300, hwnd, (HMENU)ID_LIST, NULL, NULL);
        
        hBtnMkdir = CreateWindow(L"BUTTON", L"New Folder", 
            WS_CHILD | WS_VISIBLE, 10, 350, 100, 30, hwnd, (HMENU)ID_BTN_MKDIR, NULL, NULL);
            
        hBtnTouch = CreateWindow(L"BUTTON", L"New File", 
            WS_CHILD | WS_VISIBLE, 120, 350, 100, 30, hwnd, (HMENU)ID_BTN_TOUCH, NULL, NULL);
            
        hBtnDelete = CreateWindow(L"BUTTON", L"Delete", 
            WS_CHILD | WS_VISIBLE, 230, 350, 100, 30, hwnd, (HMENU)ID_BTN_DELETE, NULL, NULL);

        hBtnUp = CreateWindow(L"BUTTON", L"Up", 
            WS_CHILD | WS_VISIBLE, 470, 350, 100, 30, hwnd, (HMENU)ID_BTN_UP, NULL, NULL);
            
        RefreshList();
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_LIST && HIWORD(wParam) == LBN_DBLCLK) {
            OnDoubleClick();
        }
        else if (LOWORD(wParam) == ID_BTN_MKDIR) CreateNew(true);
        else if (LOWORD(wParam) == ID_BTN_TOUCH) CreateNew(false);
        else if (LOWORD(wParam) == ID_BTN_DELETE) DeleteSelected();
        else if (LOWORD(wParam) == ID_BTN_UP) {
             if (current_path == "/") return 0;
             size_t last = current_path.find_last_of('/');
             if (last == 0) current_path = "/";
             else current_path = current_path.substr(0, last);
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
        0, CLASS_NAME, L"MiniFS File Manager",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 450,
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
