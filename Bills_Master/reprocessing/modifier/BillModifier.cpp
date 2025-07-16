#include "BillModifier.h"
#include <sstream>
#include <regex>
#include <algorithm>
#include <iomanip>
#include <iostream>

// ===================================================================
// 1. 配置解析与管理类 (ConfigLoader)
// 职责：从 nlohmann::json 解析配置并填充 Config 结构体。
// ===================================================================
class BillModifier::ConfigLoader {
public:
    explicit ConfigLoader(const nlohmann::json& config_json, Config& config_to_populate) {
        // 解析 modification_flags
        if (config_json.contains("modification_flags")) {
            const auto& flags = config_json["modification_flags"];
            config_to_populate.flags.enable_summing = flags.value("enable_summing", false);
            config_to_populate.flags.enable_cleanup = flags.value("enable_cleanup", false);
            config_to_populate.flags.enable_sorting = flags.value("enable_sorting", false);
            config_to_populate.flags.preserve_metadata_lines = flags.value("preserve_metadata_lines", false);
        }

        // 解析 formatting_rules
        if (config_json.contains("formatting_rules")) {
            const auto& formatting = config_json["formatting_rules"];
            config_to_populate.formatting.lines_after_parent_section = formatting.value("lines_after_parent_section", 1);
            config_to_populate.formatting.lines_after_parent_title = formatting.value("lines_after_parent_title", 1);
            config_to_populate.formatting.lines_between_sub_items = formatting.value("lines_between_sub_items", 1);
        }

        // 解析 auto_renewal_rules
        if (config_json.contains("auto_renewal_rules")) {
            const auto& renewal_config = config_json["auto_renewal_rules"];
            config_to_populate.auto_renewal.enabled = renewal_config.value("enabled", false);

            if (config_to_populate.auto_renewal.enabled && renewal_config.contains("rules")) {
                const auto& renewal_rules = renewal_config["rules"];
                for (auto it = renewal_rules.begin(); it != renewal_rules.end(); ++it) {
                    const std::string& category = it.key();
                    const nlohmann::json& items = it.value();
                    for (const auto& item_json : items) {
                        config_to_populate.auto_renewal.rules[category].push_back({
                            item_json.value("amount", 0.0),
                            item_json.value("description", "")
                        });
                    }
                }
            }
        }

        // 解析 metadata_prefixes
        if (config_json.contains("metadata_prefixes") && config_json["metadata_prefixes"].is_array()) {
            for (const auto& prefix_json : config_json["metadata_prefixes"]) {
                if (prefix_json.is_string()) {
                    config_to_populate.metadata_prefixes.push_back(prefix_json.get<std::string>());
                }
            }
        }
    }
};


// ===================================================================
// 2. 核心处理类 (BillProcessor)
// 职责：执行所有账单内容的修改，包括预处理、解析和结构化处理。
// ===================================================================
class BillModifier::BillProcessor {
public:
    explicit BillProcessor(const Config& config) : m_config(config) {}

    // 主处理流程
    std::vector<ParentItem> process(const std::string& bill_content, std::vector<std::string>& out_metadata_lines) {
        std::vector<std::string> lines = _split_string_by_lines(bill_content);
        
        _perform_initial_modifications(lines);
        
        std::vector<ParentItem> bill_structure = _parse_into_structure(lines, out_metadata_lines);

        if (m_config.flags.enable_sorting) {
            _sort_bill_structure(bill_structure);
        }

        if (m_config.flags.enable_cleanup) {
            _cleanup_bill_structure(bill_structure);
        }

        return bill_structure;
    }

private:
    const Config& m_config;

    // --- 所有处理相关的辅助函数都移到这里 ---

    void _perform_initial_modifications(std::vector<std::string>& lines) {
        if (m_config.flags.enable_summing) {
            for (std::string& line : lines) {
                _sum_up_line(line);
            }
        }

        if (m_config.auto_renewal.enabled) {
            // ... (自动续费逻辑不变)
            for (const auto& pair : m_config.auto_renewal.rules) {
                const std::string& category_title = pair.first;
                const auto& items_to_add = pair.second;

                auto category_it = std::find(lines.begin(), lines.end(), category_title);
                if (category_it == lines.end()) continue;

                auto content_start_it = category_it + 1;
                auto content_end_it = content_start_it;
                while (content_end_it != lines.end() && !_is_title(*content_end_it) && !content_end_it->empty()) {
                    ++content_end_it;
                }

                for (const auto& item_to_add : items_to_add) {
                    bool found = false;
                    for (auto it = content_start_it; it != content_end_it; ++it) {
                        if (it->find(item_to_add.description) != std::string::npos) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        std::stringstream ss;
                        ss << std::fixed << std::setprecision(2) << item_to_add.amount
                           << " " << item_to_add.description << "(auto-renewal)";
                        lines.insert(content_end_it, ss.str());
                        
                        content_end_it = content_start_it;
                        while (content_end_it != lines.end() && !_is_title(*content_end_it) && !content_end_it->empty()) {
                            ++content_end_it;
                        }
                    }
                }
            }
        }
    }

    void _sum_up_line(std::string& line) {
        // ... (函数实现不变)
        std::regex expr_regex(R"(^([\d\.\s]+\+[\d\.\s\+]+)(.*))");
        std::smatch match;

        if (std::regex_search(line, match, expr_regex) && match.size() > 1) {
            std::string expression = match[1].str();
            std::string description = match[2].str();
            
            expression.erase(std::remove(expression.begin(), expression.end(), ' '), expression.end());
            std::replace(expression.begin(), expression.end(), '+', ' ');

            std::stringstream ss(expression);
            double sum = 0.0, num;
            while (ss >> num) {
                sum += num;
            }

            std::stringstream result_ss;
            result_ss << std::fixed << std::setprecision(2) << sum << description;
            line = result_ss.str();
        }
    }

    std::vector<ParentItem> _parse_into_structure(const std::vector<std::string>& lines, std::vector<std::string>& metadata_lines) const {
        // ... (函数实现不变)
        std::vector<ParentItem> structure;
        ParentItem* current_parent = nullptr;
        SubItem* current_sub_item = nullptr;

        std::vector<std::string> temp_lines;
        for(const auto& line : lines) {
            if (m_config.flags.preserve_metadata_lines && _is_metadata_line(line)) {
                metadata_lines.push_back(line);
                continue;
            }
            
            std::string temp = line;
            if(!_trim(temp).empty()){
                temp_lines.push_back(temp);
            }
        }

        for (const std::string& line : temp_lines) {
            if (_is_parent_title(line)) {
                structure.emplace_back();
                current_parent = &structure.back();
                current_parent->title = line;
                current_sub_item = nullptr;
            } else if (_is_title(line)) {
                if (!current_parent) {
                    structure.emplace_back();
                    current_parent = &structure.back();
                    current_parent->title = "Default Parent"; 
                }
                current_parent->sub_items.emplace_back();
                current_sub_item = &current_parent->sub_items.back();
                current_sub_item->title = line;
            } else {
                if (current_sub_item) {
                    current_sub_item->contents.push_back(line);
                }
            }
        }
        return structure;
    }

    void _sort_bill_structure(std::vector<ParentItem>& bill_structure) const {
        // ... (函数实现不变)
        for (auto& parent : bill_structure) {
            for (auto& sub_item : parent.sub_items) {
                std::sort(sub_item.contents.begin(), sub_item.contents.end(), 
                    [](const std::string& a, const std::string& b) {
                        double val_a = _get_numeric_value_from_content(a);
                        double val_b = _get_numeric_value_from_content(b);
                        if (val_a != val_b) {
                            return val_a > val_b;
                        }
                        return a < b;
                    });
            }
        }
    }

    void _cleanup_bill_structure(std::vector<ParentItem>& bill_structure) const {
        // ... (函数实现不变)
        for (auto& parent : bill_structure) {
            parent.sub_items.erase(
                std::remove_if(parent.sub_items.begin(), parent.sub_items.end(),
                    [](const SubItem& sub) { return sub.contents.empty(); }),
                parent.sub_items.end());
        }

        bill_structure.erase(
            std::remove_if(bill_structure.begin(), bill_structure.end(),
                [](const ParentItem& parent) { return parent.sub_items.empty(); }),
            bill_structure.end());
    }

    // --- 静态辅助函数 ---
    static std::vector<std::string> _split_string_by_lines(const std::string& str) {
        std::vector<std::string> lines;
        std::string line;
        std::istringstream stream(str);
        while (std::getline(stream, line)) {
            lines.push_back(line);
        }
        return lines;
    }

    static std::string& _trim(std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
        return s;
    }

    bool _is_metadata_line(const std::string& line) const {
        for (const auto& prefix : m_config.metadata_prefixes) {
            if (line.rfind(prefix, 0) == 0) return true;
        }
        return false;
    }

    static double _get_numeric_value_from_content(const std::string& content_line) {
        try {
            size_t pos;
            double val = std::stod(content_line, &pos);
            return (pos == 0) ? 0.0 : val;
        } catch (const std::invalid_argument&) {
            return 0.0;
        }
    }

    static bool _is_parent_title(const std::string& line) {
        for (char c : line) {
            if (std::isupper(static_cast<unsigned char>(c))) return true;
        }
        return false;
    }

    static bool _is_title(const std::string& line) {
        if (line.empty()) return false;
        for(char c : line) {
            if(!isspace(c)) return !isdigit(c);
        }
        return false;
    }
};


// ===================================================================
// 3. 内容重构与格式化类 (BillFormatter)
// 职责：将处理过的结构化数据根据格式化规则，转换回最终的字符串。
// ===================================================================
class BillModifier::BillFormatter {
public:
    explicit BillFormatter(const Config& config) : m_config(config) {}

    std::string format(const std::vector<ParentItem>& bill_structure, const std::vector<std::string>& metadata_lines) const {
        std::stringstream ss;
        
        for (const auto& meta_line : metadata_lines) {
            ss << meta_line << "\n";
        }
        
        if (!metadata_lines.empty() && !bill_structure.empty()) {
            ss << "\n";
        }

        bool first_parent = true;
        for (const auto& parent : bill_structure) {
            if (!first_parent) {
                for(int i = 0; i < m_config.formatting.lines_after_parent_section; ++i) ss << "\n";
            }
            ss << parent.title;
            first_parent = false;

            for(int i = 0; i < m_config.formatting.lines_after_parent_title; ++i) ss << "\n";

            bool first_sub_item = true;
            for (const auto& sub_item : parent.sub_items) {
                if (!first_sub_item) {
                    for(int i = 0; i < m_config.formatting.lines_between_sub_items; ++i) ss << "\n";
                }
                ss << sub_item.title;
                first_sub_item = false;

                for (const auto& content : sub_item.contents) {
                    ss << "\n" << content;
                }
            }
        }
        return ss.str();
    }

private:
    const Config& m_config;
};


// ===================================================================
// BillModifier 公共接口实现
// 职责：作为总协调者，调用内部类完成任务。
// ===================================================================

BillModifier::BillModifier(const nlohmann::json& config_json) {
    // 构造函数仅负责委托 ConfigLoader 来加载配置
    ConfigLoader loader(config_json, m_config);
}

std::string BillModifier::modify(const std::string& bill_content) {
    // modify 方法负责协调 Processor 和 Formatter
    
    // 1. 创建处理器，处理账单内容，得到结构化数据
    BillProcessor processor(m_config);
    std::vector<std::string> metadata_lines;
    std::vector<ParentItem> bill_structure = processor.process(bill_content, metadata_lines);

    // 2. 创建格式化器，将结构化数据转换为最终字符串
    BillFormatter formatter(m_config);
    return formatter.format(bill_structure, metadata_lines);
}