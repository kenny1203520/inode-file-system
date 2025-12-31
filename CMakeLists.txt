cmake_minimum_required(VERSION 3.10)
project(InodeFileSystem)

# 設定 C++ 標準
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目錄
include_directories(include)

# 原始碼檔案
set(SOURCES
    src/DiskEmulator.cpp
    src/Bitmap.cpp
    src/InodeManager.cpp
    src/FileSystem.cpp
)

# 標頭檔案
set(HEADERS
    include/config.h
    include/structures.h
    include/DiskEmulator.h
    include/Bitmap.h
    include/InodeManager.h
    include/FileSystem.h
)

# 創建可執行檔
add_executable(inode_fs_demo src/main.cpp ${SOURCES} ${HEADERS})

# 創建簡化測試程式
add_executable(test_simple src/test_simple.cpp ${SOURCES} ${HEADERS})

# 創建極簡測試程式
add_executable(test_minimal src/test_minimal.cpp ${SOURCES} ${HEADERS})

# 如果需要，可以添加編譯選項
if(MSVC)
    target_compile_options(inode_fs_demo PRIVATE /W4)
    target_compile_options(test_simple PRIVATE /W4)
    target_compile_options(test_minimal PRIVATE /W4)
else()
    target_compile_options(inode_fs_demo PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(test_simple PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(test_minimal PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 輸出訊息
message(STATUS "CMake configuration complete for Inode File System")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
