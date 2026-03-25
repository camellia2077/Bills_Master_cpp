#ifndef PRESENTATION_PARSING_CLI_PARSER_HPP_
#define PRESENTATION_PARSING_CLI_PARSER_HPP_

#include <string>
#include <vector>

#include <common/Result.hpp>
#if defined(BILLS_CLI_MODULES_ENABLED)
import bill.cli.presentation.parsing.cli_request;
#else
#include <presentation/parsing/cli_request.hpp>
#endif

namespace bills::cli {

[[nodiscard]] auto ParseCliRequest(const std::vector<std::string>& args)
    -> Result<CliRequest>;

}  // namespace bills::cli

#endif  // PRESENTATION_PARSING_CLI_PARSER_HPP_
