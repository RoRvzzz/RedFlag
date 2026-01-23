#include "lua_dumper.h"
#include "../../../../src/sdk/sdk.h"
#include "../../../../src/memory/memory.h"
#include <sstream>
#include <algorithm>
#include <regex>

extern std::unique_ptr<memory_t> memory;

namespace memorysense::explorer {

    LuaDumper::LuaDumper() 
        : http_client(std::make_unique<HttpClient>())
        , api_endpoint("http://api.plusgiant5.com")
        , encryption_key("MemorySense2024")
        , compression_enabled(true) {
    }

    LuaDumper::~LuaDumper() = default;

    void LuaDumper::SetApiEndpoint(const std::string& endpoint) {
        api_endpoint = endpoint;
    }

    void LuaDumper::SetEncryptionKey(const std::string& key) {
        encryption_key = key;
    }

    void LuaDumper::SetCompressionEnabled(bool enabled) {
        compression_enabled = enabled;
    }

    void LuaDumper::SetProgressCallback(std::function<void(const std::string&, int)> callback) {
        progress_callback = callback;
    }

    void LuaDumper::SetStatusCallback(std::function<void(const std::string&)> callback) {
        status_callback = callback;
    }

    std::vector<LuaScript> LuaDumper::DumpAllScripts() {
        std::vector<LuaScript> scripts;
        
        if (status_callback) {
            status_callback("Starting Lua script dump...");
        }

        // This would need to be implemented based on your specific game's memory layout
        // For now, we'll create a placeholder implementation
        
        if (status_callback) {
            status_callback("Scanning for Lua scripts...");
        }

        // Placeholder - in a real implementation, you'd scan memory for Lua script instances
        // This is just an example of how the structure would work
        
        return scripts;
    }

    LuaScript LuaDumper::DumpScript(std::uint64_t script_address) {
        LuaScript script;
        script.address = script_address;
        
        if (status_callback) {
            status_callback("Dumping script at address: 0x" + std::to_string(script_address));
        }

        try {
            // Extract script information
            script = ExtractScriptFromInstance(script_address);
            
            if (script.is_compiled) {
                // Extract bytecode
                script.bytecode = ExtractBytecode(script_address);
                
                if (compression_enabled) {
                    script.bytecode = Compression::Compress(script.bytecode);
                }
                
                if (!encryption_key.empty()) {
                    script.bytecode = Compression::XorEncrypt(script.bytecode, encryption_key);
                }
            } else {
                // Extract source code
                script.source = GetScriptSource(script_address);
            }
            
            if (status_callback) {
                status_callback("Successfully dumped script: " + script.name);
            }
            
        } catch (const std::exception& e) {
            script.error_message = "Error dumping script: " + std::string(e.what());
            if (status_callback) {
                status_callback(script.error_message);
            }
        }

        return script;
    }

    bool LuaDumper::UploadScript(const LuaScript& script, const std::string& endpoint) {
        if (status_callback) {
            status_callback("Uploading script to API...");
        }

        try {
            std::string payload = CreateApiPayload(script);
            std::string url = endpoint + "/upload";
            
            auto response = http_client->Post(url, payload, "application/json");
            
            if (response.success) {
                if (status_callback) {
                    status_callback("Script uploaded successfully!");
                }
                return true;
            } else {
                if (status_callback) {
                    status_callback("Upload failed: HTTP " + std::to_string(response.status_code));
                }
                return false;
            }
        } catch (const std::exception& e) {
            if (status_callback) {
                status_callback("Upload error: " + std::string(e.what()));
            }
            return false;
        }
    }

    std::vector<LuaScript> LuaDumper::DownloadScripts(const std::string& endpoint) {
        std::vector<LuaScript> scripts;
        
        if (status_callback) {
            status_callback("Downloading scripts from API...");
        }

        try {
            std::string url = endpoint + "/download";
            auto response = http_client->Get(url);
            
            if (response.success) {
                // Parse response and deserialize scripts
                // This would need to be implemented based on your API's response format
                if (status_callback) {
                    status_callback("Scripts downloaded successfully!");
                }
            } else {
                if (status_callback) {
                    status_callback("Download failed: HTTP " + std::to_string(response.status_code));
                }
            }
        } catch (const std::exception& e) {
            if (status_callback) {
                status_callback("Download error: " + std::string(e.what()));
            }
        }

        return scripts;
    }

    LuaScript LuaDumper::ExtractScriptFromInstance(std::uint64_t instance_address) {
        LuaScript script;
        script.address = instance_address;
        
        // This is a placeholder implementation
        // In a real implementation, you'd read the instance's properties from memory
        
        script.name = "Unknown Script";
        script.class_name = "Script";
        script.is_compiled = true;
        
        return script;
    }

    std::vector<uint8_t> LuaDumper::ExtractBytecode(std::uint64_t script_address) {
        // This is a placeholder implementation
        // In a real implementation, you'd read the bytecode from the script's memory location
        
        std::vector<uint8_t> bytecode;
        // Placeholder bytecode data
        bytecode = {0x1B, 0x4C, 0x75, 0x61, 0x53, 0x00, 0x19, 0x93, 0x0D, 0x0A, 0x1A, 0x0A};
        
        return bytecode;
    }

    std::string LuaDumper::DecompileBytecode(const std::vector<uint8_t>& bytecode) {
        // This is a placeholder implementation
        // In a real implementation, you'd use a Lua bytecode decompiler
        
        return "-- Decompiled Lua script\n-- Bytecode size: " + std::to_string(bytecode.size()) + " bytes\n";
    }

    bool LuaDumper::IsLuaScript(std::uint64_t instance_address) {
        // This is a placeholder implementation
        // In a real implementation, you'd check the instance's class name and properties
        
        return true; // Placeholder
    }

    std::string LuaDumper::GetScriptSource(std::uint64_t script_address) {
        // This is a placeholder implementation
        // In a real implementation, you'd read the source code from memory
        
        return "-- Script source code\nprint('Hello, World!')\n";
    }

    std::string LuaDumper::SerializeScript(const LuaScript& script) {
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"name\": \"" << script.name << "\",\n";
        ss << "  \"class_name\": \"" << script.class_name << "\",\n";
        ss << "  \"address\": \"" << std::hex << script.address << "\",\n";
        ss << "  \"is_compiled\": " << (script.is_compiled ? "true" : "false") << ",\n";
        
        if (script.is_compiled) {
            ss << "  \"bytecode\": \"" << Compression::Base64Encode(script.bytecode) << "\",\n";
        } else {
            ss << "  \"source\": \"" << script.source << "\",\n";
        }
        
        if (!script.error_message.empty()) {
            ss << "  \"error\": \"" << script.error_message << "\"\n";
        }
        
        ss << "}";
        return ss.str();
    }

    LuaScript LuaDumper::DeserializeScript(const std::string& data) {
        // This is a placeholder implementation
        // In a real implementation, you'd parse the JSON data
        
        LuaScript script;
        return script;
    }

    std::string LuaDumper::CreateApiPayload(const LuaScript& script) {
        return SerializeScript(script);
    }
}
