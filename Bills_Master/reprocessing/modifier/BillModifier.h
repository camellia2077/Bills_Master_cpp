#ifndef BILL_MODIFIER_H
#define BILL_MODIFIER_H

#include <string>
#include <vector>
#include <map>
#include "nlohmann/json.hpp" // 需要 nlohmann/json 库

// 数据结构定义保持不变，它们是各个内部类之间沟通的桥梁
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


class BillModifier {
public:
    explicit BillModifier(const nlohmann::json& config_json);
    std::string modify(const std::string& bill_content);

private:
    // 前置声明内部实现类，将实现细节隐藏在 .cpp 文件中
    class ConfigLoader;
    class BillProcessor;
    class BillFormatter;

    // BillModifier 现在只持有一个配置对象
    Config m_config;
};

#endif // BILL_MODIFIER_H