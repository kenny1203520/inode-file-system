#include "FileSystem.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

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
                      << "  ls [path]       List directory\n"
                      << "  cd [path]       Change directory\n"
                      << "  mkdir [path]    Create directory\n"
                      << "  touch [path]    Create empty file\n"
                      << "  rm [path]       Remove file/directory\n"
                      << "  cat [path]      Show file content\n"
                      << "  write [path] [content] Write text to file\n"
                      << "  stat [path]     Show file info\n"
                      << "  info            Show filesystem info\n"
                      << "  format          Re-format disk\n"
                      << "  exit            Exit shell\n";
        } else if (cmd == "ls") {
            std::string target = (args.size() > 1) ? args[1] : current_path;
            if (target != "/" && target[0] != '/') {
                // simple relative path handling
                std::string base = current_path;
                if (base.back() != '/') base += "/";
                target = base + target;
            }
            
            auto files = fs.list(target);
            for (const auto& f : files) {
                if (f == "." || f == "..") continue;
                // Get type info
                std::string full = target;
                if (full.back() != '/') full += "/";
                full += f;
                
                Inode inode;
                if (fs.stat(full, inode)) {
                    std::cout << (inode.isDirectory() ? "[DIR]  " : "[FILE] ") 
                              << std::setw(20) << std::left << f 
                              << " " << inode.size << " bytes" << std::endl;
                } else {
                    std::cout << "        " << f << std::endl;
                }
            }
        } else if (cmd == "cd") {
            if (args.size() < 2) {
                current_path = "/";
                continue;
            }
            std::string target = args[1];
            std::string new_path;
            if (target[0] == '/') {
                new_path = target;
            } else {
                if (current_path == "/") {
                    new_path = "/" + target;
                } else {
                    new_path = current_path + "/" + target;
                }
            }

            // Verify it exists and is a directory
            Inode inode;
            if (fs.stat(new_path, inode) && inode.isDirectory()) {
                current_path = new_path;
                // Normalize ".." manually purely for display if we wanted, 
                // but for now relying on FS path resolution which handles non-normalized strings.
                // We'll leave the path string as is or do basic cleanup?
                // Let's doing basic cleanup for ".." support to keep prompt short
                 if (target == "..") {
                    // Primitive parent resolution for string
                    size_t last_slash = current_path.find_last_of('/');
                    // Remove ".."
                    current_path = current_path.substr(0, last_slash); // removes /..
                    // Remove parent
                    last_slash = current_path.find_last_of('/');
                    if (last_slash == std::string::npos) current_path = "/";
                    else if (last_slash == 0) current_path = "/"; // root
                    else current_path = current_path.substr(0, last_slash);
                 } else if (target == ".") {
                    // no op
                 }
            } else {
                std::cout << "Directory not found or not a directory." << std::endl;
            }

        } else if (cmd == "mkdir") {
            if (args.size() < 2) {
                std::cout << "Usage: mkdir <path>" << std::endl;
                continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;
            
            if (fs.mkdir(target)) {
                std::cout << "Directory created." << std::endl;
            }
        } else if (cmd == "touch") {
            if (args.size() < 2) {
                std::cout << "Usage: touch <path>" << std::endl;
                continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;
            
            if (fs.create(target, false)) {
                std::cout << "File created." << std::endl;
            }
        } else if (cmd == "rm") {
            if (args.size() < 2) {
                std::cout << "Usage: rm <path>" << std::endl;
                continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;
            
            if (fs.remove(target)) {
                std::cout << "Removed." << std::endl;
            } else {
                 std::cout << "Failed to remove (not empty? or not found?)" << std::endl;
            }
        } else if (cmd == "cat") {
            if (args.size() < 2) {
                std::cout << "Usage: cat <path>" << std::endl;
                continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;
            
            Inode inode;
            if (fs.stat(target, inode) && inode.isFile()) {
                std::vector<char> buffer(inode.size + 1);
                int r = fs.read(target, buffer.data(), inode.size);
                if (r >= 0) {
                    buffer[r] = '\0';
                    std::cout << buffer.data() << std::endl;
                }
            } else {
                std::cout << "File not found." << std::endl;
            }
        } else if (cmd == "write") {
             if (args.size() < 3) {
                std::cout << "Usage: write <path> <content_string>" << std::endl;
                continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;
            
            // Reconstruct content from remaining tokens
            std::string content;
            for(size_t i=2; i<args.size(); ++i) {
                if (i > 2) content += " ";
                content += args[i];
            }

            // Create if not exists (simplified logic: try create first)
            fs.create(target, false);
            int w = fs.write(target, content.c_str(), content.size());
            std::cout << "Written " << w << " bytes." << std::endl;

        } else if (cmd == "stat") {
             if (args.size() < 2) {
                std::cout << "Usage: stat <path>" << std::endl;
                continue;
            }
            std::string target = args[1];
            if (target[0] != '/') target = (current_path == "/") ? "/" + target : current_path + "/" + target;
             
            Inode inode;
            if (fs.stat(target, inode)) {
                 std::cout << "Type: " << (inode.isDirectory() ? "Directory" : "File") << "\n"
                           << "Size: " << inode.size << "\n"
                           << "Direct Blocks: ";
                 for(int i=0; i<4; ++i) std::cout << inode.direct_blks[i] << " ";
                 std::cout << std::endl;
            } else {
                std::cout << "Not found" << std::endl;
            }
        } else if (cmd == "info") {
            fs.printInfo();
        } else if (cmd == "format") {
            if(fs.format("disk.img", false)) {
                 std::cout << "Disk reformatted." << std::endl;
                 current_path = "/";
            }
        } else {
            std::cout << "Unknown command." << std::endl;
        }
    }

    return 0;
}
