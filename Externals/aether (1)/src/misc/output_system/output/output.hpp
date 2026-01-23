#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <cstdarg>
#include <cstdint>

namespace utils
{
    inline std::string format_hex(uintptr_t Address, bool WithPrefix = true, bool Uppercase = true)
    {
        std::ostringstream Stream;
        Stream << std::hex << (Uppercase ? std::uppercase : std::nouppercase);
        if (WithPrefix)
        {
            Stream << "0x";
        }
        Stream << Address;
        return Stream.str();
    }

    namespace output
    {
        void printint(const std::string& message, int value);
        void BrandPrint(const std::string& message);
        void printaddress(const std::string& message, void* value);
        void printaddress2(const std::string& message, uint32_t value);
        void inputA(const std::string& prompt);
        void input(const std::string& message);
        void infoaddress(const std::string& message, uint64_t value);
        void print(const std::string& message);
        void Custom(const std::string& message, const std::string& prefix);
        void info(const std::string& message);
        void printString(const std::string& Message, const std::string& MessageA);
        void System(const std::string& message);
        void warning(const std::string& message);
        void error(const std::string& message);
        void Thread(const std::string& message, int fails);
        void count(const char* message, ...);
        void address(const char* message, ...);
        void begin(const std::string& message);
        void end(const std::string& message);
        void clear_screen();
    }
}
