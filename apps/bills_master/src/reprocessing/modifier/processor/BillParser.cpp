// reprocessing/modifier/processor/BillParser.cpp

#include "BillParser.hpp"
#include <algorithm>
#include <cctype> // 包含 cctype 以使用 isalpha

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
                    // 如果一个文件以子标题开头，为其创建一个默认的父标题
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
            // 注意：如果内容行没有对应的子标题，它将被忽略。
            // 这是一个可以接受的设计，避免了孤立的数据。
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

bool BillParser::_is_parent_title(const std::string& line) {
    // 父分类ID不含下划线
    return line.find('_') == std::string::npos;
}

// --- 【核心修改】 ---
// 更新判断逻辑，只有当第一个非空字符是字母时，才认为是标题。
bool BillParser::_is_title(const std::string& line) {
    if (line.empty()) return false;
    for(char c : line) {
        if(!isspace(c)) {
            // 标题必须以字母开头
            // 内容行可以以数字或符号（+,-）开头
            return std::isalpha(static_cast<unsigned char>(c));
        }
    }
    return false; // 如果行只包含空格，则不是标题
}

std::string& BillParser::_trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}