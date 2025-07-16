#ifndef BILL_MODIFIER_H
#define BILL_MODIFIER_H

#include <string>
#include <vector>
#include <map>
#include "nlohmann/json.hpp" // 需要 nlohmann/json 库

// ... (结构体定义保持不变) ...
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
    Config m_config;

    // 阶段 1: 直接修改文件行
    void _perform_initial_modifications(std::vector<std::string>& lines);
    void _sum_up_line(std::string& line);

    // 阶段 2: 结构化修改
    std::string _process_structured_modifications(const std::vector<std::string>& lines);
    
    // -- 解析 --
    std::vector<ParentItem> _parse_into_structure(const std::vector<std::string>& lines, std::vector<std::string>& metadata_lines) const;

    void _sort_bill_structure(std::vector<ParentItem>& bill_structure) const;
    void _cleanup_bill_structure(std::vector<ParentItem>& bill_structure) const;

    std::string _reconstruct_content_with_formatting(const std::vector<ParentItem>& bill_structure, const std::vector<std::string>& metadata_lines) const;

    // -- 辅助函数 --
    bool _is_metadata_line(const std::string& line) const;
    // --- 修改开始 ---
    // 添加缺失的函数声明
    static bool _is_parent_title(const std::string& line);
    // --- 修改结束 ---
    static double _get_numeric_value_from_content(const std::string& content_line);
    static bool _is_title(const std::string& line);
    static std::vector<std::string> _split_string_by_lines(const std::string& str);
    static std::string& _trim(std::string& s);
};

#endif // BILL_MODIFIER_H