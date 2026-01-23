#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "http_client.h"
#include "compression.h"

namespace memorysense::explorer {
    struct LuaScript {
        std::string name;
        std::string class_name;
        std::string source;
        std::vector<uint8_t> bytecode;
        std::uint64_t address;
        bool is_compiled;
        std::string error_message;
    };

    class LuaDumper {
    public:
        LuaDumper();
        ~LuaDumper();

        // Main dumping functions
        std::vector<LuaScript> DumpAllScripts();
        LuaScript DumpScript(std::uint64_t script_address);
        
        // API integration
        bool UploadScript(const LuaScript& script, const std::string& api_endpoint = "http://api.plusgiant5.com/upload");
        std::vector<LuaScript> DownloadScripts(const std::string& api_endpoint = "http://api.plusgiant5.com/download");
        
        // Utility functions
        void SetApiEndpoint(const std::string& endpoint);
        void SetEncryptionKey(const std::string& key);
        void SetCompressionEnabled(bool enabled);
        
        // Callbacks for progress/status updates
        void SetProgressCallback(std::function<void(const std::string&, int)> callback);
        void SetStatusCallback(std::function<void(const std::string&)> callback);

    private:
        std::unique_ptr<HttpClient> http_client;
        std::string api_endpoint;
        std::string encryption_key;
        bool compression_enabled;
        
        std::function<void(const std::string&, int)> progress_callback;
        std::function<void(const std::string&)> status_callback;
        
        // Internal helper functions
        LuaScript ExtractScriptFromInstance(std::uint64_t instance_address);
        std::vector<uint8_t> ExtractBytecode(std::uint64_t script_address);
        std::string DecompileBytecode(const std::vector<uint8_t>& bytecode);
        bool IsLuaScript(std::uint64_t instance_address);
        std::string GetScriptSource(std::uint64_t script_address);
        
        // API communication helpers
        std::string SerializeScript(const LuaScript& script);
        LuaScript DeserializeScript(const std::string& data);
        std::string CreateApiPayload(const LuaScript& script);
    };
}
