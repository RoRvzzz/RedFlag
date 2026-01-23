#pragma once
#include <windows.h>
#include <string>
#include <cstdlib>
#include <ctime>
#include <thread>

//<console.hpp>
namespace utils
{
    namespace console
    {
        void set_size(int size)
        {
          
        }

        void Set_RConsole(bool answer)
        {
            if (answer) {
                HWND consoleWindow = GetConsoleWindow();
                SetWindowPos(consoleWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            }
            else {
                printf("RConsole Set To False");
            }
        }

        void hide_console()
        {
            ShowWindow(GetConsoleWindow(), SW_HIDE);
        }

        void show_console()
        {
            ShowWindow(GetConsoleWindow(), SW_SHOW);
        }
        void SetConsoleSize(int width, int height) {
            COORD coord;
            coord.X = width;
            coord.Y = height;

           
            SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coord);

           
            SMALL_RECT rect;
            rect.Top = 0;
            rect.Left = 0;
            rect.Bottom = height - 1;
            rect.Right = width - 1;
            SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &rect);
        }

        void Set_Console_Font(int FontSizeX = 0, int FontSizeY = 12)
        {
            CONSOLE_FONT_INFOEX FontInfo = {};
            FontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
            GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &FontInfo);

            FontInfo.dwFontSize.X = FontSizeX;
            FontInfo.dwFontSize.Y = FontSizeY;
            FontInfo.FontWeight = FW_NORMAL;
            wcscpy_s(FontInfo.FaceName, L"Consolas");

            SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &FontInfo);
        }

        void Set_Font(const std::wstring& FontName, int FontSizeX, int FontSizeY, bool Bold = false)
        {
            CONSOLE_FONT_INFOEX FontInfo = {};
            FontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
            FontInfo.FontWeight = Bold ? FW_BOLD : FW_NORMAL;
            FontInfo.dwFontSize.X = FontSizeX;
            FontInfo.dwFontSize.Y = FontSizeY;
            wcsncpy_s(FontInfo.FaceName, FontName.c_str(), FontName.length());

            SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &FontInfo);
        }

        void Set_Font_Size(int FontSizeX, int FontSizeY)
        {
            CONSOLE_FONT_INFOEX FontInfo = {};
            FontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
            GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &FontInfo);

            FontInfo.dwFontSize.X = FontSizeX;
            FontInfo.dwFontSize.Y = FontSizeY;

            SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &FontInfo);
        }
       
        void enable_ansi()
        {
            HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD dw_mode = 0;
            GetConsoleMode(console, &dw_mode);
            dw_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(console, dw_mode);
        }

        void set_transparency(float alpha)
        {
            HWND hwnd = GetConsoleWindow();
            if (!hwnd) return;

            LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
            SetWindowLong(hwnd, GWL_EXSTYLE, style | WS_EX_LAYERED);

            BYTE alphaValue = static_cast<BYTE>(alpha * 255);
            SetLayeredWindowAttributes(hwnd, 0, alphaValue, LWA_ALPHA);
        }


        void set_console_name(const std::string& name)
        {
            SetConsoleTitleA(name.c_str());
        }

        std::string generate_random_name()
        {
            const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
            std::string random_str;
            for (int i = 0; i < 62; ++i)
            {
                random_str += chars[rand() % chars.length()];
            }
            return  random_str + std::to_string(rand() % 10000);
        }

        void randomize_console_name()
        {
            std::thread([]() {
                srand(static_cast<unsigned>(time(0)));
                while (true)
                {
                    std::string random_name = generate_random_name();
                    set_console_name(random_name);
                //    Sleep(100);
                }
                }).detach(); 
        }
    }
}
