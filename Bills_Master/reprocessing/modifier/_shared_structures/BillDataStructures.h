#ifndef BILL_DATA_STRUCTURES_H
#define BILL_DATA_STRUCTURES_H

#include <string>
#include <vector>
#include <map>
#include "nlohmann/json.hpp" // nlohmann/json is used by ConfigLoader but its data types are here

// --- Data Structures ---

struct Config {
    struct ModificationFlags {
        bool enable_summing = false;
        bool enable_cleanup = false;
        bool enable_sorting = false;
        bool preserve_metadata_lines = false;
    } flags;

    struct FormattingRules {
        int lines_after_parent_section = 1;
        int lines_after_parent_title = 1;
        int lines_between_sub_items = 0;
    } formatting;

    struct AutoRenewalItem {
        double amount;
        std::string description;
    };

    struct AutoRenewalConfig {
        bool enabled = false;
        std::map<std::string, std::vector<AutoRenewalItem>> rules;
    };
    AutoRenewalConfig auto_renewal;

    std::vector<std::string> metadata_prefixes;
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

#endif // BILL_DATA_STRUCTURES_H