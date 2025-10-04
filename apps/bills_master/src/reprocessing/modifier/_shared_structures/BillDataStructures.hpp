// reprocessing/modifier/_shared_structures/BillDataStructures.hpp

#ifndef BILL_DATA_STRUCTURES_HPP
#define BILL_DATA_STRUCTURES_HPP

#include <string>
#include <vector>
#include <map>
#include "nlohmann/json.hpp"

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

using ContentItem = std::string;

struct SubItem {
    std::string title;
    std::vector<ContentItem> contents;
};

struct ParentItem {
    std::string title;
    std::vector<SubItem> sub_items;
};

#endif // BILL_DATA_STRUCTURES_HPP