// reprocessing/modifier/processor/BillParser.cpp

#include "BillParser.hpp"
#include <algorithm>

BillParser::BillParser(const Config& config) : m_config(config) {}

std::vector<ParentItem> BillParser::parse(const std::vector<std::string>& lines, std::vector<std::string>& out_metadata_lines) const {
    std::vector<ParentItem> structure;
    ParentItem* current_parent = nullptr;
    SubItem* current_sub_item = nullptr;

    std::vector<std::string> temp_lines;
    for(const auto& line : lines) {
        if (_is_metadata_line(line)) {
            out_metadata_lines.push_back(line);
            continue;
        }
        std::string temp = line;
        if(!_trim(temp).empty()){
            temp_lines.push_back(temp);
        }
    }

    for (const std::string& line : temp_lines) {
        if (_is_title(line)) { // 首先判断是否是一个标题行
            if (_is_parent_title(line)) { // 然后判断是父标题还是子标题
                structure.emplace_back();
                current_parent = &structure.back();
                current_parent->title = line;
                current_sub_item = nullptr;
            } else { // 是子标题
                if (!current_parent) {
                    structure.emplace_back();
                    current_parent = &structure.back();
                    current_parent->title = "Default Parent"; 
                }
                current_parent->sub_items.emplace_back();
                current_sub_item = &current_parent->sub_items.back();
                current_sub_item->title = line;
            }
        } else { // 是内容行
            if (current_sub_item) {
                current_sub_item->contents.push_back(line);
            }
        }
    }
    return structure;
}

bool BillParser::_is_metadata_line(const std::string& line) const {
    for (const auto& prefix : m_config.metadata_prefixes) {
        if (line.rfind(prefix, 0) == 0) return true;
    }
    return false;
}

// **修改**: 更新判断逻辑。父分类ID不含下划线。
bool BillParser::_is_parent_title(const std::string& line) {
    return line.find('_') == std::string::npos;
}

bool BillParser::_is_title(const std::string& line) {
    if (line.empty()) return false;
    for(char c : line) {
        if(!isspace(c)) return !isdigit(c);
    }
    return false;
}

std::string& BillParser::_trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}