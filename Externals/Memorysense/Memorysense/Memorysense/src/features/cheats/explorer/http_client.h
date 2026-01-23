#pragma once
#include <windows.h>
#include <wininet.h>
#include <string>
#include <vector>
#include <memory>

#pragma comment(lib, "wininet.lib")

namespace memorysense::explorer {
    class HttpClient {
    public:
        HttpClient();
        ~HttpClient();

        struct HttpResponse {
            int status_code;
            std::string body;
            std::string headers;
            bool success;
        };

        HttpResponse Post(const std::string& url, const std::string& data, const std::string& content_type = "application/octet-stream");
        HttpResponse Get(const std::string& url);
        
        void SetTimeout(int timeout_ms = 30000);
        void SetUserAgent(const std::string& user_agent);

    private:
        HINTERNET hInternet;
        HINTERNET hConnect;
        int timeout_ms;
        std::string user_agent;
        
        bool Initialize();
        void Cleanup();
        std::string ReadResponse(HINTERNET hRequest);
        std::string GetHeaders(HINTERNET hRequest);
    };
}
