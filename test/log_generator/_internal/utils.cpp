#include "utils.h"
#include <iostream>

void show_version(const std::string& version, const std::string& last_update) {
    std::cout << "generator version " << version << std::endl;
    std::cout << "Last updated: " << last_update << std::endl;
}

void show_help(const char* app_name) {
    std::cerr << "Usage: " << app_name << " [options]\n\n"
              << "A pseudo-random bill file generator based on a JSON configuration.\n\n"
              << "Options:\n"
              << "  -s, --single <year>    Generate bills for a single specified year.\n"
              << "  -d, --double <start_year> <end_year>\n"
              << "                         Generate bills for all years in the inclusive range.\n"
              << "  -h, --help             Show this help message and exit.\n"
              << "  --version              Show version information and exit.\n\n"
              << "Example:\n"
              << "  " << app_name << " -s 2003\n"
              << "  " << app_name << " --double 2003 2005\n"
              << "  " << app_name << " --help\n";
}