#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <cstdint>

void xor_encrypt(const std::string& input_path, 
                const std::string& output_path,
                uint8_t xor_key = 0xAA) {
    std::ifstream in_file(input_path, std::ios::binary);
    if (!in_file) {
        throw std::runtime_error("无法打开输入文件: " + input_path);
    }

    std::vector<uint8_t> buffer(
        (std::istreambuf_iterator<char>(in_file)),
        std::istreambuf_iterator<char>());
    
    for (auto& byte : buffer) {
        byte ^= xor_key;
    }

    std::ofstream out_file(output_path, std::ios::binary);
    if (!out_file) {
        throw std::runtime_error("无法创建输出文件: " + output_path);
    }
    
    out_file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "用法: " << argv[0] << " <输入文件> <输出文件>\n";
            return 1;
        }
        
        xor_encrypt(argv[1], argv[2]);
        std::cout << "加密成功: " << argv[2] << std::endl;
        return 0;
    } 
    catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 2;
    }
}