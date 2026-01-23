#pragma once
#include <string>
#include <iostream>
#include <cstdarg>
#include <consoleapi.h>
#include <WinBase.h>

namespace debugSys {
    inline void enable_virtual_terminal_processing() {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) return;

        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
#endif
    }

    enum class Color {
        Red,
        Green,
        Yellow,
        Cyan,
        Magenta,
        Blue,
        White,
        Reset
    };

    inline std::string get_color(Color color) {
        switch (color) {
        case Color::Red: return "\033[31m";
        case Color::Green: return "\033[32m";
        case Color::Yellow: return "\033[33m";
        case Color::Cyan: return "\033[36m";
        case Color::Magenta: return "\033[35m";
        case Color::Blue: return "\033[34m";
        case Color::White: return "\033[37m";
        case Color::Reset: return "\033[0m";
        }
        return "\033[0m";
    }

    template<typename... Args>
    inline void print(const char* type, Color color, Args... args) {
        std::cout << get_color(color) << "[" << type << "]" << get_color(Color::Reset) << " ";
        ((std::cout << args), ...);
        std::cout << std::endl;
        std::cout.flush();
    }

    namespace info {
        template<typename... Args>
        inline void log(Args... args) {
            print("INFO", Color::Green, args...);
        }
    }

    namespace warning {
        template<typename... Args>
        inline void log(Args... args) {
            print("WARNING", Color::Yellow, args...);
        }
    }

    namespace error {
        template<typename... Args>
        inline void log(Args... args) {
            print("ERROR", Color::Red, args...);
        }
    }

    namespace placeid {
        template<typename... Args>
        inline void log(Args... args) {
            print("PLACEID", Color::Blue, args...);
        }
    }

    namespace debug_output {
        template<typename... Args>
        inline void log(Args... args) {
            print("DEBUG", Color::Magenta, args...);
        }
    }
}
