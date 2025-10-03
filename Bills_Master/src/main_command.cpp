// main_command.cpp

#include "command_handler/CommandFacade.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

// 设置控制台编码，以便正确显示 UTF-8 字符
void setup_console() { 
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

int main(int argc, char* argv[]) {
    setup_console();

    CommandFacade command_facade;
    return command_facade.run(argc, argv);
}