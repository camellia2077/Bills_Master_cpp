#include "arg_parser.h"
#include <iostream>
#include <string>

ProgramOptions parse_arguments(int argc, char* argv[]) {
    ProgramOptions opts;

    if (argc <= 1) {
        opts.action = Action::SHOW_HELP;
        return opts;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            opts.action = Action::SHOW_HELP;
            return opts;
        }
        if (arg == "--version") {
            opts.action = Action::SHOW_VERSION;
            return opts;
        }
        if (arg == "-s" || arg == "--single") {
            if (i + 1 >= argc) {
                opts.error_message = "Missing argument for " + arg + " option.";
                return opts;
            }
            try {
                int year = std::stoi(argv[++i]);
                opts.start_year = year;
                opts.end_year = year;
                opts.action = Action::GENERATE;
                return opts;
            } catch (const std::exception&) {
                opts.error_message = "Invalid year provided for " + arg + ".";
                return opts;
            }
        }
        if (arg == "-d" || arg == "--double") {
            if (i + 2 >= argc) {
                opts.error_message = "Missing arguments for " + arg + " option.";
                return opts;
            }
            try {
                opts.start_year = std::stoi(argv[++i]);
                opts.end_year = std::stoi(argv[++i]);
                if (opts.start_year > opts.end_year) {
                    opts.error_message = "Start year cannot be after end year.";
                    return opts;
                }
                opts.action = Action::GENERATE;
                return opts;
            } catch (const std::exception&) {
                opts.error_message = "Invalid year(s) provided for " + arg + ".";
                return opts;
            }
        }
    }

    opts.error_message = "No valid operation specified.";
    opts.action = Action::SHOW_HELP; // Default to showing help if args are confusing
    return opts;
}