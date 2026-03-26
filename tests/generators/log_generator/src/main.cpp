#include "presentation/cli_app.h"

auto main(int argc, char* argv[]) -> int {
  CliApp app;
  return app.Run(argc, argv);
}
