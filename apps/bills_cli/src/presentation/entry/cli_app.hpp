#ifndef PRESENTATION_ENTRY_CLI_APP_HPP_
#define PRESENTATION_ENTRY_CLI_APP_HPP_

namespace bills::cli {

class CliApp {
 public:
  auto Run(int argc, char* argv[]) -> int;
};

}  // namespace bills::cli

#endif  // PRESENTATION_ENTRY_CLI_APP_HPP_
