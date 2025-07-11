#include "parser.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <string>
#include <algorithm>

static bool is_whitespace(const std::string& s) {
    for (char c : s) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

// --- 函数逻辑已修改 ---
// 现在只检查行首是否为大写字母，以识别父标题行
bool BillParser::is_parent_title(const std::string& line) {
    if (line.empty()) return false;
    return std::isupper(static_cast<unsigned char>(line[0]));
}

// --- 函数逻辑已修改 ---
// 现在只检查行首是否为小写字母，以识别子标题行
bool BillParser::is_sub_title(const std::string& line) {
    if (line.empty()) return false;
    return std::islower(static_cast<unsigned char>(line[0]));
}


ParsedBill BillParser::parse(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + file_path);
    }

    ParsedBill bill_data;
    std::string line;
    std::string current_parent;
    std::string current_sub;

    while (std::getline(file, line)) {
        if (line.empty() || is_whitespace(line)) {
            continue;
        }

        if (line.rfind("DATE:", 0) == 0) {
            bill_data.date = line.substr(5);
        } else if (line.rfind("REMARK:", 0) == 0) {
            bill_data.remark = line.substr(7);
        } 
        // --- 核心修改：识别并提取正确的标题 ---
        else if (is_parent_title(line)) {
            size_t pos = 0;
            while (pos < line.length() && std::isupper(static_cast<unsigned char>(line[pos]))) {
                pos++;
            }
            current_parent = line.substr(0, pos);
        } else if (is_sub_title(line)) {
            size_t pos = 0;
            while (pos < line.length() && (std::islower(static_cast<unsigned char>(line[pos])) || line[pos] == '_')) {
                pos++;
            }
            current_sub = line.substr(0, pos);
        } 
        else {
            std::stringstream ss(line);
            double amount;
            std::string description;

            ss >> amount;

            std::string remaining_part;
            if (std::getline(ss, remaining_part)) {
                size_t first_char_pos = remaining_part.find_first_not_of(" \t");
                if (std::string::npos != first_char_pos) {
                    description = remaining_part.substr(first_char_pos);
                }
            }

            Transaction t;
            t.parent_category = current_parent;
            t.sub_category = current_sub;
            t.amount = amount;
            t.description = description;
            bill_data.transactions.push_back(t);
        }
    }

    return bill_data;
}