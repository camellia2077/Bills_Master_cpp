// app_controller/ConfigLoader.hpp
#ifndef CONFIG_LOADER_HPP
#define CONFIG_LOADER_HPP

#include "nlohmann/json.hpp"
#include <string>

// Forward declaration
class FileHandler;

class ConfigLoader {
public:
    static nlohmann::json load_and_validate_validator_config(const std::string& config_path, FileHandler& file_handler);
    static nlohmann::json load_and_validate_modifier_config(const std::string& config_path, FileHandler& file_handler);
};

#endif // CONFIG_LOADER_HPP