#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace memorysense::explorer {
    class Compression {
    public:
        // Compress data using Windows Compression API
        static std::vector<uint8_t> Compress(const std::vector<uint8_t>& data);
        static std::vector<uint8_t> Compress(const std::string& data);
        
        // Decompress data using Windows Compression API
        static std::vector<uint8_t> Decompress(const std::vector<uint8_t>& compressed_data);
        static std::string DecompressToString(const std::vector<uint8_t>& compressed_data);
        
        // Base64 encoding/decoding for API transmission
        static std::string Base64Encode(const std::vector<uint8_t>& data);
        static std::vector<uint8_t> Base64Decode(const std::string& encoded);
        
        // Simple XOR encryption for basic obfuscation
        static std::vector<uint8_t> XorEncrypt(const std::vector<uint8_t>& data, const std::string& key);
        static std::vector<uint8_t> XorDecrypt(const std::vector<uint8_t>& data, const std::string& key);

    private:
        static bool InitializeCompression();
        static void CleanupCompression();
    };
}
