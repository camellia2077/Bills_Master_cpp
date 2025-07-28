#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include <string>

enum class Action {
    GENERATE,
    SHOW_HELP,
    SHOW_VERSION,
    ERROR
};

struct ProgramOptions {
    Action action = Action::ERROR;
    int start_year = 0;
    int end_year = 0;
    std::string error_message;
};

// Parses command line arguments and returns the results in a ProgramOptions struct.
ProgramOptions parse_arguments(int argc, char* argv[]);

#endif // ARG_PARSER_H