
#include "globals/globals.hpp"

#include <string>
#include <wininet.h>
#include <iostream>
#include <thread>
#include "rbx/overlay/json.hpp"
#include <iostream>
#include <windows.h>
#include <wbemidl.h>
#include <comutil.h>
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <ShlObj.h>
#include <iostream>
#include <windows.h>
#include <shlobj.h>
#include <winhttp.h>
#include "misc/output_system/output/output.hpp"
#pragma comment(lib, "winhttp.lib")

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "comsuppw.lib") 
std::string GetHardwareID() {
    HRESULT hres;
    IWbemLocator* pLoc = nullptr;
    IWbemServices* pSvc = nullptr;
    IEnumWbemClassObject* pEnumerator = nullptr;
    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;
    std::string hwid = "UNKNOWN";

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return hwid;
    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres)) {
        CoUninitialize();
        return hwid;
    }

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres)) {
        CoUninitialize();
        return hwid;
    }

    hres = pLoc->ConnectServer(BSTR(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return hwid;
    }

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return hwid;
    }

    hres = pSvc->ExecQuery(BSTR(L"WQL"), BSTR(L"SELECT SerialNumber FROM Win32_BaseBoard"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return hwid;
    }

    while (pEnumerator) {
        hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (!uReturn) break;

        VARIANT vtProp;
        pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
        hwid = _bstr_t(vtProp.bstrVal);
        VariantClear(&vtProp);
        pclsObj->Release();
    }

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();
    return hwid;
}
typedef void (*StartConsoleFunc)();
bool CreateDirectoryRecursive(const std::string& path) {
    size_t pos = 0;
    while ((pos = path.find('\\', pos)) != std::string::npos) {
        std::string subdir = path.substr(0, pos);
        if (!CreateDirectoryA(subdir.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
            std::cout << "Failed to create directory: " << subdir << std::endl;
            return false;
        }
        pos++;
    }
    return CreateDirectoryA(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}
void CreateConfigPath() {
    char appDataPath[MAX_PATH];
    HRESULT result = SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath);

    if (result != S_OK) {
        std::cout << "Failed to get the AppData directory path. Error code: " << result << std::endl;
    }

    std::string dirPath = std::string(appDataPath) + "\\Void\\Configs";

    if (CreateDirectoryRecursive(dirPath)) {
        utils::output::info("Directory Created, Or Exist");
    }
    else {
        utils::output::error("Directory For Configurations Failed To Be Created.");
    }
}

#include <cstdlib> 

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::string GetLiverobloxVersion() {
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    DWORD bytesRead = 0;
    LPSTR szOutBuffer;
    DWORD dwSize = 0;
    std::string response;

    hSession = WinHttpOpen(L"A WinHTTP Example Program/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSession) {
        std::cerr << "WinHttpOpen failed" << std::endl;
        return "";
    }

    hConnect = WinHttpConnect(hSession, L"clientsettings.roblox.com", INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!hConnect) {
        std::cerr << "WinHttpConnect failed" << std::endl;
        WinHttpCloseHandle(hSession);
        return "";
    }

    hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/v2/client-version/WindowsPlayer/channel/LIVE",
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_REFRESH);

    if (!hRequest) {
        std::cerr << "WinHttpOpenRequest failed" << std::endl;
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    BOOL  bResults = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0,
        0, 0);

    if (!bResults) {
        std::cerr << "WinHttpSendRequest failed" << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResults) {
        std::cerr << "WinHttpReceiveResponse failed" << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
            std::cerr << "WinHttpQueryDataAvailable failed" << std::endl;
            break;
        }

        szOutBuffer = new char[dwSize + 1];
        if (!szOutBuffer) {
            std::cerr << "Out of memory" << std::endl;
            break;
        }

        ZeroMemory(szOutBuffer, dwSize + 1);

        if (!WinHttpReadData(hRequest, (LPVOID)szOutBuffer, dwSize, &bytesRead)) {
            std::cerr << "WinHttpReadData failed" << std::endl;
            delete[] szOutBuffer;
            break;
        }
        else {
            response.append(szOutBuffer, bytesRead);
        }

        delete[] szOutBuffer;
    } while (dwSize > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    try {
        auto jsonResponse = nlohmann::json::parse(response);
        return jsonResponse["clientVersionUpload"].get<std::string>();
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
        return "";
    }
}