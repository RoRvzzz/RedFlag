#include "entry.hpp"
#include <Windows.h>
#include <string>
#include <thread>
#include <iostream>
#include "misc/output_system/output/console.hpp"
#include "rbx/main.h"

int main(/* this cheat is developed by: playboi carti, DDG, and ken carson */) {

    utils::console::enable_ansi();
    utils::console::SetConsoleSize(60, 21);
    utils::console::Set_Console_Font();
    utils::console::Set_RConsole(true);
    utils::console::set_console_name("aether.gg");
    CreateConfigPath();
    RBX::Initializer();
    while (true) {}
    return 0;
}