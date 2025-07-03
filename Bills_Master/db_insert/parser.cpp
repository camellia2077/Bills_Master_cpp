#include "parser.h"
#include <fstream>
#include <sstream>
#include <cctype>

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
        // 跳过空行
        if (line.empty()) {
            continue;
        }

        // 处理 DATE 和 REMARK
        if (line.rfind("DATE:", 0) == 0) {
            bill_data.date = line.substr(5);
        } else if (line.rfind("REMARK:", 0) == 0) {
            bill_data.remark = line.substr(7);
        } else if (is_parent_title(line)) {
            current_parent = line;
        } else if (is_sub_title(line)) {
            current_sub = line;
        } else {
            // 处理内容行
            std::stringstream ss(line);
            double amount;
            std::string description;

            ss >> amount;
            // 读取从第一个非空格字符开始的剩余部分作为描述
            if (ss.rdbuf()->in_avail() > 0) {
                 // consume the space
                ss.get();
                std::getline(ss, description);
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

bool BillParser::is_parent_title(const std::string& line) {
    // 父标题是全大写英文字母
    for (char c : line) {
        if (!std::isupper(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return !line.empty();
}

bool BillParser::is_sub_title(const std::string& line) {
    // 子标题全是小写字母或下划线
    for (char c : line) {
        if (!std::islower(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }
    return !line.empty();
}