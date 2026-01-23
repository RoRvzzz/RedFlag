#include "console.h"
#include <windows.h>
#include <thread>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")

// Simple tray icon with context menu to show/hide console window

namespace {
    HINSTANCE g_hInstance = nullptr;
    HWND g_consoleWnd = nullptr;
    UINT WM_TRAYICON = WM_APP + 0x4477;
    NOTIFYICONDATAW nid; // use explicit wide-char variant to avoid UNICODE/multibyte mismatches
    HMENU g_menu = nullptr;
    std::thread g_msgThread;
    bool g_running = false;

    enum MenuIds : UINT {
        ID_SHOW_CONSOLE = 10001,
        ID_HIDE_CONSOLE = 10002,
        ID_EXIT_APP     = 10003
    };

    void ShowConsole(bool show) {
        if (g_consoleWnd == nullptr)
            g_consoleWnd = GetConsoleWindow();
        if (g_consoleWnd)
            ShowWindow(g_consoleWnd, show ? SW_SHOW : SW_HIDE);
    }

    LRESULT CALLBACK TrayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_TRAYICON) {
            if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_CONTEXTMENU) {
                POINT pt; GetCursorPos(&pt);
                SetForegroundWindow(hWnd);
                TrackPopupMenu(g_menu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION,
                               pt.x, pt.y, 0, hWnd, nullptr);
                PostMessage(hWnd, WM_NULL, 0, 0);
                return 0;
            } else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
                ShowConsole(true);
                return 0;
            }
        }
        switch (msg) {
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
            case ID_SHOW_CONSOLE: ShowConsole(true); break;
            case ID_HIDE_CONSOLE: ShowConsole(false); break;
            case ID_EXIT_APP: ExitProcess(0); break;
            }
            return 0;
        }
        case WM_DESTROY:
            Shell_NotifyIconW(NIM_DELETE, &nid);
            if (g_menu) DestroyMenu(g_menu);
            return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    void MessageLoop() {
        g_consoleWnd = GetConsoleWindow();
        g_hInstance = GetModuleHandle(nullptr);

        const wchar_t* cls = L"aimquette_tray_class";
        WNDCLASSEXW wc{ sizeof(wc) };
        wc.lpfnWndProc = TrayWndProc;
        wc.hInstance = g_hInstance;
        wc.lpszClassName = cls;
        RegisterClassExW(&wc);

        HWND hwnd = CreateWindowExW(0, cls, L"aimquette_tray_wnd", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, g_hInstance, nullptr);

        // Try to load app icon from resources and apply it to console and tray
        HICON hAppIcon = (HICON)LoadImageW(g_hInstance, MAKEINTRESOURCEW(101), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
        if (hAppIcon && g_consoleWnd) {
            SendMessageW(g_consoleWnd, WM_SETICON, ICON_BIG,   (LPARAM)hAppIcon);
            SendMessageW(g_consoleWnd, WM_SETICON, ICON_SMALL, (LPARAM)hAppIcon);
        }

        g_menu = CreatePopupMenu();
        AppendMenuW(g_menu, MF_STRING, ID_SHOW_CONSOLE, L"Show Console");
        AppendMenuW(g_menu, MF_STRING, ID_HIDE_CONSOLE, L"Hide Console");
        AppendMenuW(g_menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(g_menu, MF_STRING, ID_EXIT_APP, L"Close");

        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = hAppIcon ? hAppIcon : LoadIcon(nullptr, IDI_APPLICATION);
        nid.uFlags |= NIF_SHOWTIP; // ensure visibility
        lstrcpynW(nid.szTip, L"Aimquette", static_cast<int>(sizeof(nid.szTip) / sizeof(nid.szTip[0])));
        Shell_NotifyIconW(NIM_ADD, &nid);
        // Some shells need an update to reflect icon changes
        Shell_NotifyIconW(NIM_MODIFY, &nid);

        MSG msg;
        while (g_running && GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        DestroyWindow(hwnd);
        UnregisterClassW(cls, g_hInstance);
    }
}

namespace tray {
    void initialize() {
        if (g_running) return;
        g_running = true;
        g_msgThread = std::thread(MessageLoop);
    }

    void shutdown() {
        if (!g_running) return;
        g_running = false;
        PostThreadMessage(GetThreadId(g_msgThread.native_handle()), WM_QUIT, 0, 0);
        if (g_msgThread.joinable()) g_msgThread.join();
    }
}

