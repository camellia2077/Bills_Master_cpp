// reprocessing/modifier/processor/BillContentTransformer.cpp

#include "BillContentTransformer.hpp"

#include <sstream>
#include <regex>
#include <algorithm>
#include <iomanip>
#include <iostream>

BillContentTransformer::BillContentTransformer(const Config& config) : m_config(config) {}

std::vector<ParentItem> BillContentTransformer::process(const std::string& bill_content, std::vector<std::string>& out_metadata_lines) {
    std::vector<std::string> lines = _split_string_by_lines(bill_content);
    _perform_initial_modifications(lines);
    std::vector<ParentItem> bill_structure = _parse_into_structure(lines, out_metadata_lines);

    _sort_bill_structure(bill_structure);
    _cleanup_bill_structure(bill_structure);
    
    return bill_structure;
}

// ===================================================================
// Private Member Function Implementations
// ===================================================================

void BillContentTransformer::_perform_initial_modifications(std::vector<std::string>& lines) {
    for (std::string& line : lines) {
        _sum_up_line(line);
    }

    // --- 修改：更新自动续费逻辑以使用新的数据结构 ---
    if (m_config.auto_renewal.enabled) {
        // 现在直接遍历规则的 vector
        for (const auto& rule : m_config.auto_renewal.rules) {
            const std::string& category_title = rule.header_location;
            
            auto category_it = std::find(lines.begin(), lines.end(), category_title);
            if (category_it == lines.end()) continue;

            auto content_start_it = category_it + 1;
            auto content_end_it = content_start_it;
            while (content_end_it != lines.end() && !_is_title(*content_end_it) && !content_end_it->empty()) {
                ++content_end_it;
            }
            
            bool found = false;
            for (auto it = content_start_it; it != content_end_it; ++it) {
                if (it->find(rule.description) != std::string::npos) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                std::stringstream ss;
                ss << std::fixed << std::setprecision(2) << rule.amount
                   << " " << rule.description << "(auto-renewal)";
                lines.insert(content_end_it, ss.str());
            }
        }
    }
}

void BillContentTransformer::_sum_up_line(std::string& line) {
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

std::vector<ParentItem> BillContentTransformer::_parse_into_structure(const std::vector<std::string>& lines, std::vector<std::string>& metadata_lines) const {
    std::vector<ParentItem> structure;
    ParentItem* current_parent = nullptr;
    SubItem* current_sub_item = nullptr;

    std::vector<std::string> temp_lines;
    for(const auto& line : lines) {
        if (_is_metadata_line(line)) {
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

void BillContentTransformer::_sort_bill_structure(std::vector<ParentItem>& bill_structure) const {
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

void BillContentTransformer::_cleanup_bill_structure(std::vector<ParentItem>& bill_structure) const {
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

// --- Static Helper Function Implementations ---

std::vector<std::string> BillContentTransformer::_split_string_by_lines(const std::string& str) {
    std::vector<std::string> lines;
    std::string line;
    std::istringstream stream(str);
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::string& BillContentTransformer::_trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

bool BillContentTransformer::_is_metadata_line(const std::string& line) const {
    for (const auto& prefix : m_config.metadata_prefixes) {
        if (line.rfind(prefix, 0) == 0) return true;
    }
    return false;
}

double BillContentTransformer::_get_numeric_value_from_content(const std::string& content_line) {
    try {
        size_t pos;
        double val = std::stod(content_line, &pos);
        return (pos == 0) ? 0.0 : val;
    } catch (const std::invalid_argument&) {
        return 0.0;
    }
}

bool BillContentTransformer::_is_parent_title(const std::string& line) {
    for (char c : line) {
        if (std::isupper(static_cast<unsigned char>(c))) return true;
    }
    return false;
}

bool BillContentTransformer::_is_title(const std::string& line) {
    if (line.empty()) return false;
    for(char c : line) {
        if(!isspace(c)) return !isdigit(c);
    }
    return false;
}