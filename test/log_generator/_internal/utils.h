#ifndef UTILS_H
#define UTILS_H

#include <string>

// Displays the program's version and last update information.
void show_version(const std::string& version, const std::string& last_update);

// Displays the command-line usage help message.
void show_help(const char* app_name);

#endif // UTILS_H