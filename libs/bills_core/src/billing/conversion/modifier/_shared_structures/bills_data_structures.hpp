// conversion/modifier/_shared_structures/bills_data_structures.hpp

#ifndef BILL_DATA_STRUCTURES_HPP
#define BILL_DATA_STRUCTURES_HPP

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

#endif  // BILL_DATA_STRUCTURES_HPP
