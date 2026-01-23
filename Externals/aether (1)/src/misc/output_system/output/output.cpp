#include "output.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <cstdio>  
#include <windows.h> 

namespace utils::output
{
    struct LogInitializer
    {
        LogInitializer()
        {
            std::remove("log.txt");

            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut != INVALID_HANDLE_VALUE)
            {
                DWORD dwMode = 0;
                if (GetConsoleMode(hOut, &dwMode))
                {
                    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
                }
            }
        }
    } _LogInit;

    static std::ofstream LogFile("log.txt", std::ios::app);

    static const char* GetColorCodeForPrefix(const std::string& prefix)
    {
        if (prefix == "INFO")      return "\x1b[36m";
        if (prefix == "OUTPUT")    return "\x1b[32m"; 
        if (prefix == "X")     return "\x1b[35m"; 
        if (prefix == "PLAYERS")   return "\x1b[33m"; 
        if (prefix == "INPUT")     return "\x1b[34m"; 
        if (prefix == "SYSTEM")    return "\x1b[37m"; 
        if (prefix == "WARNING")   return "\x1b[91m"; 
        if (prefix == "CRITICAL")  return "\x1b[31m";
        if (prefix == "STATUS")    return "\x1b[35m"; 
        if (prefix == "COUNT")     return "\x1b[36m"; 
        if (prefix == "ADDRESS")   return "\x1b[32m"; 
        if (prefix == "BEGIN")     return "\x1b[94m"; 
        if (prefix == "END")       return "\x1b[94m"; 
        if (prefix == "USER")      return "\x1b[33m"; 
        return "\x1b[0m"; 
    }

    inline void WriteLog(const std::string& Prefix, const std::string& Message)
    {
        if (LogFile.is_open())
        {
            LogFile << "[" << Prefix << "] " << Message << std::endl;
            LogFile.flush(); // dont want to rape the users storage or memory
        }

        std::cout << "[" << Prefix << "]" << Message << std::endl;
    }

    void infoaddress(const std::string& Message, uint64_t Value)
    {
        std::ostringstream Stream;
        Stream << Message << ": 0x" << std::hex << std::uppercase << Value;
        WriteLog("INFO", Stream.str());
    }

    void printaddress(const std::string& Message, void* Value)
    {
        std::ostringstream Stream;
        Stream << Message << ": " << Value;
        WriteLog("OUTPUT", Stream.str());
    }

    void printaddress2(const std::string& Message, uint32_t Value)
    {
        std::ostringstream Stream;
        Stream << Message << ": 0x" << std::hex << std::uppercase << Value;
        WriteLog("OUTPUT", Stream.str());
    }

    void printint(const std::string& Message, int Value)
    {
        std::ostringstream Stream;
        Stream << Message << ": " << Value;
        WriteLog("INFO", Stream.str());
    }

    void PrintFloat(const std::string& Message, float Value)
    {
        std::ostringstream Stream;
        Stream << Message << ": " << Value;
        WriteLog("INFO", Stream.str());
    }

    void BrandPrint(const std::string& Message)
    {
        WriteLog("X", Message);
    }

    void print(const std::string& Message)
    {
        WriteLog("PLAYERS", Message);
    }

    void Custom(const std::string& Message, const std::string& Prefix)
    {
        WriteLog(Prefix, Message);
    }

    void input(const std::string& Message)
    {
        WriteLog("INPUT", Message);
    }

    void inputA(const std::string& Prompt)
    {
        WriteLog("INPUT", Prompt);
        std::string UserInput;
        std::getline(std::cin, UserInput);
        WriteLog("USER", UserInput);
    }

    void info(const std::string& Message)
    {
        if (!Message.empty())
        {
            WriteLog("INFO", Message);
        }
    }
    void printString(const std::string& Message, const std::string& MessageA)
    {
        if (!Message.empty())
        {
            WriteLog("INFO", Message);
        }
    }
    void System(const std::string& Message)
    {
        WriteLog("SYSTEM", Message);
    }

    void warning(const std::string& Message)
    {
        WriteLog("WARNING", Message);
    }

    void error(const std::string& Message)
    {
        WriteLog("CRITICAL", Message);
    }

    void Thread(const std::string& Message, int Fails)
    {
        std::ostringstream Stream;
        Stream << Message << " (" << Fails << ")";
        WriteLog("STATUS", Stream.str());
    }

    void count(const char* Message, ...)
    {
        char Buffer[1024];
        va_list Args;
        va_start(Args, Message);
        vsnprintf(Buffer, sizeof(Buffer), Message, Args);
        va_end(Args);

        WriteLog("COUNT", Buffer);
    }

    void address(const char* Message, ...)
    {
        char Buffer[1024];
        va_list Args;
        va_start(Args, Message);
        vsnprintf(Buffer, sizeof(Buffer), Message, Args);
        va_end(Args);

        WriteLog("ADDRESS", Buffer);
    }

    void begin(const std::string& Message)
    {
        WriteLog("BEGIN", Message);
    }

    void end(const std::string& Message)
    {
        WriteLog("END", Message);
    }

    void clear_screen()
    {
        system("cls");
    }
}
