// reprocessing/modifier/_shared_structures/BillDataStructures.hpp

#ifndef BILL_DATA_STRUCTURES_H
#define BILL_DATA_STRUCTURES_H

#include <string>
#include <vector>
#include <map>
#include "nlohmann/json.hpp"

// --- Data Structures ---

struct Config {
    // --- 新增：为自动续费规则定义一个清晰的结构体 ---
    struct AutoRenewalRule {
        std::string header_location;
        double amount;
        std::string description;
    };

    // --- 修改：更新 AutoRenewalConfig 结构体 ---
    struct AutoRenewalConfig {
        bool enabled = false;
        // 将原来的 map 结构改为新结构体的 vector，以匹配JSON数组
        std::vector<AutoRenewalRule> rules;
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