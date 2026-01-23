#include "console.h"
#include <cstdarg>
#include <cstdio>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <Windows.h>

namespace memorysense {
    namespace console {

        void Console::AddLog(LogLevel level, const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            char buffer[1024];
            vsnprintf(buffer, sizeof(buffer), fmt, args);
            va_end(args);

            std::lock_guard<std::mutex> lock(logs_mutex);
            
            LogEntry entry;
            entry.timestamp = GetTimestamp();
            entry.level = level;
            entry.message = buffer;
            entry.color = GetColorForLevel(level);
            
            logs.push_back(entry);
            
            // Keep only last 1000 logs
            if (logs.size() > 1000) {
                logs.erase(logs.begin());
            }
            
            // Print to console/terminal with colored timestamp and log level
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            
            // Set color for both timestamp and log level (same color)
            switch (level) {
                case LOG_INFO:    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY); break;
                case LOG_DEBUG:   SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY); break; // Cyan
                case LOG_WARNING: SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); break; // Yellow
                case LOG_ERROR:   SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); break;
            }
            
            // Print timestamp and log level (same color)
            printf("[%s][%s]", entry.timestamp.c_str(), GetLevelString(level));
            
            // Reset to white for message
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            
            // Print message (bright white)
            printf(" %s\n", buffer);
        }

        void Console::Clear() {
            std::lock_guard<std::mutex> lock(logs_mutex);
            logs.clear();
        }

        void Console::Render(bool* p_open) {
            ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowBgAlpha(0.95f);
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
            
            if (!ImGui::Begin("Console", p_open, ImGuiWindowFlags_NoCollapse)) {
                ImGui::PopStyleVar(2);
                ImGui::End();
                return;
            }

            // Compact header with filter and clear button
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 60);
            filter.Draw("##filter", -1);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear")) {
                Clear();
            }

            ImGui::Separator();

            // Log window
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
            
            std::lock_guard<std::mutex> lock(logs_mutex);
            
            for (const auto& log : logs) {
                // Filter by level
                if ((log.level == LOG_INFO && !show_info) ||
                    (log.level == LOG_DEBUG && !show_debug) ||
                    (log.level == LOG_WARNING && !show_warning) ||
                    (log.level == LOG_ERROR && !show_error)) {
                    continue;
                }

                // Apply text filter
                if (!filter.PassFilter(log.message.c_str())) {
                    continue;
                }

                // Build log line
                std::string log_line;
                if (show_timestamp) {
                    log_line += log.timestamp + " ";
                }
                log_line += "[" + std::string(GetLevelString((LogLevel)log.level)) + "] ";
                log_line += log.message;

                ImGui::PushStyleColor(ImGuiCol_Text, log.color);
                ImGui::TextUnformatted(log_line.c_str());
                ImGui::PopStyleColor();
            }

            if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }

            ImGui::PopStyleVar();
            ImGui::EndChild();

            ImGui::PopStyleVar(2);
            ImGui::End();
        }

        ImVec4 Console::GetColorForLevel(LogLevel level) {
            switch (level) {
                case LOG_INFO:    return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  // Green
                case LOG_DEBUG:   return ImVec4(0.5f, 0.5f, 1.0f, 1.0f);  // Light Blue
                case LOG_WARNING: return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow
                case LOG_ERROR:   return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // Red
                default:          return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // White
            }
        }

        const char* Console::GetLevelString(LogLevel level) {
            switch (level) {
                case LOG_INFO:    return "INFO";
                case LOG_DEBUG:   return "DEBUG";
                case LOG_WARNING: return "WARNING";
                case LOG_ERROR:   return "ERROR";
                default:          return "UNKNOWN";
            }
        }

        std::string Console::GetTimestamp() {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
            
            std::tm tm_time;
            localtime_s(&tm_time, &time);
            
            std::ostringstream oss;
            oss << std::setfill('0') 
                << std::setw(2) << tm_time.tm_hour << ":"
                << std::setw(2) << tm_time.tm_min << ":"
                << std::setw(2) << tm_time.tm_sec << ":"
                << std::setw(3) << ms.count();
            
            return oss.str();
        }
    }
}

