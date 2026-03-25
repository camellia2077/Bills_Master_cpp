#if defined(BILLS_CLI_MODULES_ENABLED)
import bill.cli.presentation.entry.cli_app;
#else
#include <presentation/entry/cli_app.hpp>
#endif

#include <pch.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

void SetupConsole() {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif
}

}  // namespace

auto main(int argc, char* argv[]) -> int {
  SetupConsole();

  bills::cli::CliApp app;
  return app.Run(argc, argv);
}
