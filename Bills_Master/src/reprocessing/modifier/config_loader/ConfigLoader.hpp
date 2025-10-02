// reprocessing/modifier/config_loader/ConfigLoader.hpp
#ifndef CONFIG_LOADER_HPP
#define CONFIG_LOADER_HPP

#include "reprocessing/modifier/_shared_structures/BillDataStructures.hpp"
#include "nlohmann/json.hpp"

class ConfigLoader {
public:
    /**
     * @brief Loads configuration from a JSON object.
     * 
     * @param config_json The nlohmann::json object containing the configuration.
     * @return A populated Config object.
     */
    static Config load(const nlohmann::json& config_json);
};

#endif // CONFIG_LOADER_HPP