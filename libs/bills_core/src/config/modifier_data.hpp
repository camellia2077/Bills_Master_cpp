// config/modifier_data.hpp

#ifndef CONFIG_MODIFIER_DATA_HPP_
#define CONFIG_MODIFIER_DATA_HPP_

#include <map>
#include <string>
#include <vector>

// --- Data Structures ---

struct Config {
  struct AutoRenewalRule {
    std::string header_location;
    double amount;
    std::string description;
  };

  struct AutoRenewalConfig {
    bool enabled = false;
    std::vector<AutoRenewalRule> rules;
  };

  AutoRenewalConfig auto_renewal;
  std::vector<std::string> metadata_prefixes;
  std::map<std::string, std::map<std::string, std::string>> display_name_maps;
};

#endif  // CONFIG_MODIFIER_DATA_HPP_
