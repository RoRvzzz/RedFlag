#include "compression.h"
#include <algorithm>
#include <sstream>

namespace memorysense::explorer {

    std::vector<uint8_t> Compression::Compress(const std::vector<uint8_t>& data) {
        if (data.empty()) return {};
        
        // Simple compression using RLE (Run Length Encoding) for bytecode
        std::vector<uint8_t> compressed;
        compressed.reserve(data.size());
        
        for (size_t i = 0; i < data.size(); ) {
            uint8_t current_byte = data[i];
            uint8_t count = 1;
            
            // Count consecutive identical bytes
            while (i + count < data.size() && data[i + count] == current_byte && count < 255) {
                count++;
            }
            
            if (count > 3) {
                // Use RLE encoding: 0xFF marker, count, byte
                compressed.push_back(0xFF);
                compressed.push_back(count);
                compressed.push_back(current_byte);
            } else {
                // Store bytes directly
                for (uint8_t j = 0; j < count; j++) {
                    compressed.push_back(current_byte);
                }
            }
            
            i += count;
        }
        
        return compressed;
    }

    std::vector<uint8_t> Compression::Compress(const std::string& data) {
        std::vector<uint8_t> byte_data(data.begin(), data.end());
        return Compress(byte_data);
    }

    std::vector<uint8_t> Compression::Decompress(const std::vector<uint8_t>& compressed_data) {
        if (compressed_data.empty()) return {};
        
        std::vector<uint8_t> decompressed;
        decompressed.reserve(compressed_data.size() * 2);
        
        for (size_t i = 0; i < compressed_data.size(); ) {
            if (compressed_data[i] == 0xFF && i + 2 < compressed_data.size()) {
                // RLE encoded data
                uint8_t count = compressed_data[i + 1];
                uint8_t byte_value = compressed_data[i + 2];
                
                for (uint8_t j = 0; j < count; j++) {
                    decompressed.push_back(byte_value);
                }
                
                i += 3;
            } else {
                // Direct byte
                decompressed.push_back(compressed_data[i]);
                i++;
            }
        }
        
        return decompressed;
    }

    std::string Compression::DecompressToString(const std::vector<uint8_t>& compressed_data) {
        auto decompressed = Decompress(compressed_data);
        return std::string(decompressed.begin(), decompressed.end());
    }

    std::string Compression::Base64Encode(const std::vector<uint8_t>& data) {
        const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        int val = 0, valb = -6;
        
        for (uint8_t c : data) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                result.push_back(chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        
        if (valb > -6) {
            result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
        }
        
        while (result.size() % 4) {
            result.push_back('=');
        }
        
        return result;
    }

    std::vector<uint8_t> Compression::Base64Decode(const std::string& encoded) {
        const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::vector<uint8_t> result;
        int val = 0, valb = -8;
        
        for (char c : encoded) {
            if (chars.find(c) == std::string::npos) break;
            val = (val << 6) + chars.find(c);
            valb += 6;
            if (valb >= 0) {
                result.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        
        return result;
    }

    std::vector<uint8_t> Compression::XorEncrypt(const std::vector<uint8_t>& data, const std::string& key) {
        if (key.empty()) return data;
        
        std::vector<uint8_t> result = data;
        size_t key_index = 0;
        
        for (size_t i = 0; i < result.size(); i++) {
            result[i] ^= key[key_index];
            key_index = (key_index + 1) % key.length();
        }
        
        return result;
    }

    std::vector<uint8_t> Compression::XorDecrypt(const std::vector<uint8_t>& data, const std::string& key) {
        // XOR decryption is the same as encryption
        return XorEncrypt(data, key);
    }
}
