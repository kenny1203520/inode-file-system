#include "FileSystem.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#ifdef _WIN32
#include <windows.h>
#endif

// Helper to tokenize input
std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::stringstream ss(input);
    std::string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// Helper to resolve relative paths for "cd" simulation
std::string resolvePath(const std::string& current, const std::string& target) {
    if (target.empty()) return current;
    if (target[0] == '/') return target; // Absolute path

    std::string result = current;
    if (result.back() != '/') result += "/";
    result += target;
    
    // Simplistic normalization could be added here if needed, 
    // but FileSystem::resolvePath handles ".." logically during lookup, 
    // though the string representation might get messy like "/home/../tmp".
    // For a simple shell, we rely on the FileSystem to traverse.
    // BUT, for the prompt to look nice, we might want to normalize.
    // Let's keep it simple: just string concat, and let FS handle the actual traversal logic.
    return result;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    FileSystem fs;
    if (!fs.mount("disk.img")) {
        // If mount fails, try format
        std::cout << "Mount failed. Formatting new disk.img..." << std::endl;
        if (!fs.format("disk.img", false)) {
            std::cerr << "Format failed!" << std::endl;
            return 1;
        }
    }

    std::string current_path = "/";
    std::cout << "MiniFS Shell. Type 'help' for commands." << std::endl;

    while (true) {
        std::cout << "minifs:" << current_path << "> ";
        std::string line;
        if (!std::getline(std::cin, line)) break;
        
        auto args = tokenize(line);
        if (args.empty()) continue;

        std::string cmd = args[0];

        if (cmd == "exit") {
            break;
        } else if (cmd == "help") {
            std::cout << "Commands:\n"
                      << "  [Mandatory Specs]\n"
                      << "  mkfs <disk>     Create/Format file system\n"
                      << "  ls <path>       List directory\n"
                      << "  mkdir <path>    Create directory\n"
                      << "  rmdir <path>    Remove empty directory\n"
                      << "  touch <path>    Create empty file\n"
                      << "  rm <path>       Remove file\n"
                      << "  cat <path>      Show file content\n"
                      << "  append <path> <text> Append text to file\n"
                      << "  stat <path>     Show inode info\n"
                      << "\n  [Extensions]\n"
                      << "  cd <path>       Change current directory\n"
                      << "  write <path> <text> Overwrite text to file\n"
                      << "  cp <src> <dest> Copy file\n"
                      << "  mv <src> <dest> Move/rename file or directory\n"
                      << "  format          Alias for mkfs\n"
                      << "  info            Show disk usage info\n"
                      << "  clear/cls       Clear screen\n"
                      << "  exit            Exit shell\n";
        } else if (cmd == "mkfs") {
            std::string diskName = "disk.img";
            if (args.size() > 1) diskName = args[1];
            
            if(fs.format(diskName, false)) {
                 std::cout << "FileSystem initialized on " << diskName << std::endl;
                 current_path = "/";
            }
        } else if (cmd == "ls") {
            std::string target = (args.size() > 1) ? args[1] : current_path;
            if (target != "/" && target[0] != '/') {
                std::string base = current_path;
                if (base.back() != '/') base += "/";
                target = base + target;
            }
            
            auto files = fs.list(target);
            for (const auto& f : files) {
                if (f == "." || f == "..") continue;
                std::string full = target;
                if (full.back() != '/') full += "/";
                full += f;
                
                Inode inode;
                if (fs.stat(full, inode)) {
                    // Simple list format
                    std::cout << f << (inode.isDirectory() ? "/" : "") << std::endl;
                }
            }
        } else if (cmd == "cd") {
             // 'cd' is NOT in the mandatory spec list but useful for navigation
             // We keep it for user convenience as a "Shell Simulation" feature
             if (args.size() < 2) {
                current_path = "/";
                continue;
            }
            std::string target = args[1];
            std::string new_path = target;
            if (target[0] != '/') {
                if (current_path == "/") new_path = "/" + target;
                else new_path = current_path + "/" + target;
            }
            Inode inode;
            if (fs.stat(new_path, inode) && inode.isDirectory()) {
                 current_path = new_path;
                 // Basic cleanup for display
                 if (target == "..") {
                      size_t last = current_path.find_last_of('/');
                      if (last != std::string::npos) {
                           // Remove last segment (..)
                           current_path = current_path.substr(0, last);
                           // Remove parent
                           last = current_path.find_last_of('/');
                           if (last == 0) current_path = "/";
                           else if (last != std::string::npos) current_path = current_path.substr(0, last);
                           else current_path = "/";
                      }
                 }
            } else {
                std::cout << "Invalid directory" << std::endl;
            }
        } else if (cmd == "mkdir") {
            if (args.size() < 2) {
                std::cout << "Usage: mkdir <path>" << std::endl;
                continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;
            fs.mkdir(target);
        } else if (cmd == "rmdir") {
            if (args.size() < 2) {
                std::cout << "Usage: rmdir <path>" << std::endl;
                continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;
            if(fs.rmdir(target)) std::cout << "Directory removed" << std::endl;
            else std::cout << "Failed (not empty or not found)" << std::endl;
        } else if (cmd == "touch") {
            if (args.size() < 2) {
                 std::cout << "Usage: touch <path>" << std::endl;
                 continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;
            fs.create(target, false);
        } else if (cmd == "rm") {
            if (args.size() < 2) {
                 std::cout << "Usage: rm <path>" << std::endl;
                 continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;
            if(fs.remove(target)) std::cout << "File removed" << std::endl; 
            else std::cout << "Failed" << std::endl;
        } else if (cmd == "cat") {
            if (args.size() < 2) {
                 std::cout << "Usage: cat <path>" << std::endl;
                 continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;
            
            Inode inode;
            if (fs.stat(target, inode)) {
                if (inode.isDirectory()) {
                    std::cout << "Is a directory" << std::endl;
                } else {
                    std::vector<char> buf(inode.size + 1);
                    if (fs.read(target, buf.data(), inode.size) >= 0) {
                        buf[inode.size] = 0;
                        std::cout << buf.data() << std::endl;
                    }
                }
            } else {
                std::cout << "File not found" << std::endl;
            }
        } else if (cmd == "append") {
             if (args.size() < 3) {
                 std::cout << "Usage: append <path> \"text\"" << std::endl;
                 continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;

            // Extract text from args[2...]
            std::string content;
            // Basic quote handling if user typed "Hello World"
            std::string first_word = args[2];
            if (first_word.front() == '"') {
                // It's a quoted string?
                // Re-parsing properly is hard with simple tokenize, 
                // let's just join all remaining args and strip quotes if present at ends.
                for (size_t i=2; i<args.size(); ++i) {
                    if (i > 2) content += " ";
                    content += args[i];
                }
                if (content.size() >= 2 && content.front() == '"' && content.back() == '"') {
                    content = content.substr(1, content.size() - 2);
                }
            } else {
                content = first_word;
            }

            Inode inode;
            if (!fs.stat(target, inode)) {
                // Spec says "append", normally implies file exists.
                // But let's auto-create if missing for friendliness?
                // Spec implies "touch" creates, "append" appends.
                std::cout << "File not found" << std::endl;
            } else {
                if (inode.isDirectory()) {
                    std::cout << "Cannot append to directory" << std::endl;
                } else {
                    int w = fs.write(target, content.c_str(), content.size(), inode.size);
                    if (w >= 0) std::cout << "Appended " << w << " bytes" << std::endl;
                    else std::cout << "Append failed" << std::endl;
                }
            }
        } else if (cmd == "stat") {
             if (args.size() < 2) {
                 std::cout << "Usage: stat <path>" << std::endl;
                 continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;
            
            Inode inode;
            if (fs.stat(target, inode)) {
                 std::cout << "Inode: " << inode.inode_num << "\n"
                           << "Size: " << inode.size << "\n"
                           << "Direct Blocks: ";
                 int used = 0;
                 for(int i=0; i<4; ++i) {
                     if (inode.direct_blks[i] != 0) {
                         std::cout << inode.direct_blks[i] << " ";
                         used++;
                     }
                 }
                 std::cout << "\nUsed Blocks: " << used << std::endl;
            } else {
                std::cout << "Not found" << std::endl;
            }
        } else if (cmd == "format") {
            // Alias for mkfs
            std::string diskName = "disk.img";
            if(fs.format(diskName, false)) {
                 std::cout << "FileSystem initialized on " << diskName << std::endl;
                 current_path = "/";
            }
        } else if (cmd == "info") {
            fs.printInfo();
        } else if (cmd == "write") {
             if (args.size() < 3) {
                std::cout << "Usage: write <path> <content_string>" << std::endl;
                continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;
            
            std::string content;
            std::string first_word = args[2];
            if (first_word.front() == '"') {
                for (size_t i=2; i<args.size(); ++i) {
                    if (i > 2) content += " ";
                    content += args[i];
                }
                if (content.size() >= 2 && content.front() == '"' && content.back() == '"') {
                    content = content.substr(1, content.size() - 2);
                }
            } else {
                content = first_word;
            }

            // Overwrite mode: create/truncate
            fs.create(target, false); 
            // write(path, buf, size, offset=0)
            int w = fs.write(target, content.c_str(), content.size(), 0);
            if (w >= 0) std::cout << "Written " << w << " bytes." << std::endl;
            else std::cout << "Write failed." << std::endl;

        } else if (cmd == "cp") {
            if (args.size() < 3) {
                std::cout << "Usage: cp <source> <destination>" << std::endl;
                continue;
            }
            
            std::string src = args[1];
            std::string dest = args[2];
            
            // Resolve relative paths
            if (src[0] != '/') src = (current_path == "/") ? "/" + src : current_path + "/" + src;
            if (dest[0] != '/') dest = (current_path == "/") ? "/" + dest : current_path + "/" + dest;
            
            if (fs.copy(src, dest)) {
                std::cout << "✓ File copied successfully: " << src << " -> " << dest << std::endl;
            } else {
                std::cout << "✗ Copy failed" << std::endl;
            }
            
        } else if (cmd == "mv") {
            if (args.size() < 3) {
                std::cout << "Usage: mv <source> <destination>" << std::endl;
                continue;
            }
            
            std::string src = args[1];
            std::string dest = args[2];
            
            // Resolve relative paths
            if (src[0] != '/') src = (current_path == "/") ? "/" + src : current_path + "/" + src;
            if (dest[0] != '/') dest = (current_path == "/") ? "/" + dest : current_path + "/" + dest;
            
            if (fs.move(src, dest)) {
                std::cout << "✓ Moved/renamed successfully: " << src << " -> " << dest << std::endl;
            } else {
                std::cout << "✗ Move failed (destination may exist or source not found)" << std::endl;
            }
            
        } else if (cmd == "clear" || cmd == "cls") {
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif

        } else {
            std::cout << "Unknown command" << std::endl;
        }
    }

    return 0;
}
