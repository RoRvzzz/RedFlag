#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include <string>
#include <vector>
#include <mutex>
#include <ctime>
#include <imgui/imgui.h>

namespace memorysense {
    namespace console {
        
        enum LogLevel {
            LOG_INFO,
            LOG_DEBUG,
            LOG_WARNING,
            LOG_ERROR
        };

        struct LogEntry {
            std::string timestamp;
            int level;
            std::string message;
            ImVec4 color;
        };

        class Console {
        public:
            static Console& Get() {
                static Console instance;
                return instance;
            }

            void AddLog(LogLevel level, const char* fmt, ...);
            void Clear();
            void Render(bool* p_open = nullptr);

            bool auto_scroll = true;
            bool show_timestamp = true;
            bool show_info = true;
            bool show_debug = true;
            bool show_warning = true;
            bool show_error = true;

        private:
            Console() = default;
            std::vector<LogEntry> logs;
            std::mutex logs_mutex;
            char input_buffer[256] = "";
            ImGuiTextFilter filter;

            ImVec4 GetColorForLevel(LogLevel level);
            const char* GetLevelString(LogLevel level);
            std::string GetTimestamp();
        };

        // Helper macros
        #define CONSOLE_INFO(fmt, ...) memorysense::console::Console::Get().AddLog(memorysense::console::LOG_INFO, fmt, ##__VA_ARGS__)
        #define CONSOLE_DEBUG(fmt, ...) memorysense::console::Console::Get().AddLog(memorysense::console::LOG_DEBUG, fmt, ##__VA_ARGS__)
        #define CONSOLE_WARNING(fmt, ...) memorysense::console::Console::Get().AddLog(memorysense::console::LOG_WARNING, fmt, ##__VA_ARGS__)
        #define CONSOLE_ERROR(fmt, ...) memorysense::console::Console::Get().AddLog(memorysense::console::LOG_ERROR, fmt, ##__VA_ARGS__)
    }
}

