#include "http_client.h"
#include <iostream>
#include <sstream>

namespace memorysense::explorer {

    HttpClient::HttpClient() : hInternet(nullptr), hConnect(nullptr), timeout_ms(30000) {
        user_agent = "MemorySense/1.0";
        Initialize();
    }

    HttpClient::~HttpClient() {
        Cleanup();
    }

    bool HttpClient::Initialize() {
        hInternet = InternetOpenA(user_agent.c_str(), 
                                 INTERNET_OPEN_TYPE_PRECONFIG, 
                                 nullptr, 
                                 nullptr, 
                                 0);
        return hInternet != nullptr;
    }

    void HttpClient::Cleanup() {
        if (hConnect) {
            InternetCloseHandle(hConnect);
            hConnect = nullptr;
        }
        if (hInternet) {
            InternetCloseHandle(hInternet);
            hInternet = nullptr;
        }
    }

    void HttpClient::SetTimeout(int timeout_ms) {
        this->timeout_ms = timeout_ms;
    }

    void HttpClient::SetUserAgent(const std::string& user_agent) {
        this->user_agent = user_agent;
    }

    HttpClient::HttpResponse HttpClient::Post(const std::string& url, const std::string& data, const std::string& content_type) {
        HttpResponse response;
        response.success = false;
        response.status_code = 0;

        if (!hInternet) {
            return response;
        }

        // Parse URL
        URL_COMPONENTSA urlComp;
        ZeroMemory(&urlComp, sizeof(urlComp));
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.dwSchemeLength = -1;
        urlComp.dwHostNameLength = -1;
        urlComp.dwUrlPathLength = -1;
        urlComp.dwExtraInfoLength = -1;

        if (!InternetCrackUrlA(url.c_str(), url.length(), 0, &urlComp)) {
            return response;
        }

        std::string hostname(urlComp.lpszHostName, urlComp.dwHostNameLength);
        std::string path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
        if (urlComp.lpszExtraInfo) {
            path += std::string(urlComp.lpszExtraInfo, urlComp.dwExtraInfoLength);
        }

        // Connect to server
        hConnect = InternetConnectA(hInternet, 
                                   hostname.c_str(), 
                                   urlComp.nPort, 
                                   nullptr, 
                                   nullptr, 
                                   INTERNET_SERVICE_HTTP, 
                                   0, 
                                   0);
        
        if (!hConnect) {
            return response;
        }

        // Create request
        HINTERNET hRequest = HttpOpenRequestA(hConnect, 
                                             "POST", 
                                             path.c_str(), 
                                             "HTTP/1.1", 
                                             nullptr, 
                                             nullptr, 
                                             INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 
                                             0);
        
        if (!hRequest) {
            return response;
        }

        // Set headers
        std::stringstream headers;
        headers << "Content-Type: " << content_type << "\r\n";
        headers << "Content-Length: " << data.length() << "\r\n";
        headers << "User-Agent: " << user_agent << "\r\n";
        headers << "Connection: close\r\n";

        HttpAddRequestHeadersA(hRequest, headers.str().c_str(), -1, HTTP_ADDREQ_FLAG_ADD);

        // Send request
        if (HttpSendRequestA(hRequest, nullptr, 0, const_cast<char*>(data.c_str()), data.length())) {
            // Get status code
            DWORD statusCode;
            DWORD statusCodeSize = sizeof(statusCode);
            HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, nullptr);
            response.status_code = statusCode;

            // Read response
            response.body = ReadResponse(hRequest);
            response.headers = GetHeaders(hRequest);
            response.success = (statusCode >= 200 && statusCode < 300);
        }

        InternetCloseHandle(hRequest);
        return response;
    }

    HttpClient::HttpResponse HttpClient::Get(const std::string& url) {
        HttpResponse response;
        response.success = false;
        response.status_code = 0;

        if (!hInternet) {
            return response;
        }

        // Parse URL
        URL_COMPONENTSA urlComp;
        ZeroMemory(&urlComp, sizeof(urlComp));
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.dwSchemeLength = -1;
        urlComp.dwHostNameLength = -1;
        urlComp.dwUrlPathLength = -1;
        urlComp.dwExtraInfoLength = -1;

        if (!InternetCrackUrlA(url.c_str(), url.length(), 0, &urlComp)) {
            return response;
        }

        std::string hostname(urlComp.lpszHostName, urlComp.dwHostNameLength);
        std::string path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
        if (urlComp.lpszExtraInfo) {
            path += std::string(urlComp.lpszExtraInfo, urlComp.dwExtraInfoLength);
        }

        // Connect to server
        hConnect = InternetConnectA(hInternet, 
                                   hostname.c_str(), 
                                   urlComp.nPort, 
                                   nullptr, 
                                   nullptr, 
                                   INTERNET_SERVICE_HTTP, 
                                   0, 
                                   0);
        
        if (!hConnect) {
            return response;
        }

        // Create request
        HINTERNET hRequest = HttpOpenRequestA(hConnect, 
                                             "GET", 
                                             path.c_str(), 
                                             "HTTP/1.1", 
                                             nullptr, 
                                             nullptr, 
                                             INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 
                                             0);
        
        if (!hRequest) {
            return response;
        }

        // Set headers
        std::stringstream headers;
        headers << "User-Agent: " << user_agent << "\r\n";
        headers << "Connection: close\r\n";

        HttpAddRequestHeadersA(hRequest, headers.str().c_str(), -1, HTTP_ADDREQ_FLAG_ADD);

        // Send request
        if (HttpSendRequestA(hRequest, nullptr, 0, nullptr, 0)) {
            // Get status code
            DWORD statusCode;
            DWORD statusCodeSize = sizeof(statusCode);
            HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, nullptr);
            response.status_code = statusCode;

            // Read response
            response.body = ReadResponse(hRequest);
            response.headers = GetHeaders(hRequest);
            response.success = (statusCode >= 200 && statusCode < 300);
        }

        InternetCloseHandle(hRequest);
        return response;
    }

    std::string HttpClient::ReadResponse(HINTERNET hRequest) {
        std::string response;
        char buffer[4096];
        DWORD bytesRead;

        while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            response.append(buffer, bytesRead);
        }

        return response;
    }

    std::string HttpClient::GetHeaders(HINTERNET hRequest) {
        char headers[8192];
        DWORD headersSize = sizeof(headers);
        
        if (HttpQueryInfoA(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, headers, &headersSize, nullptr)) {
            return std::string(headers, headersSize);
        }
        
        return "";
    }
}
