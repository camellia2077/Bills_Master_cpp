// main_command.cpp

#include "command_handler/command_dispatcher.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

// 设置控制台编码，以便正确显示 UTF-8 字符
void SetupConsole() {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif
}

auto main(int argc, char* argv[]) -> int {
  SetupConsole();

  CommandDispatcher command_dispatcher;
  return command_dispatcher.run(argc, argv);
}
