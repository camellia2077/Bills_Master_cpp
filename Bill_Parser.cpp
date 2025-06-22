// Bill_Parser.cpp
#include "Bill_Parser.h"
#include <fstream>
#include <vector>
#include <sstream>

Bill_Parser::Bill_Parser(LineValidator& validator) : m_validator(validator) {
    reset();
}

void Bill_Parser::reset() {
    m_current_date.clear();
    m_current_parent.clear();
    m_current_child.clear();
    m_line_number = 0;
}

std::vector<std::string> Bill_Parser::parseFile(
    const std::string& file_path,
    std::function<void(const ParsedRecord&)> handler)
{
    std::vector<std::string> errors;
    std::ifstream file(file_path);
    if (!file.is_open()) {
        // 对于文件无法打开这种严重错误，我们仍然抛出异常
        throw std::runtime_error("Could not open file: " + file_path);
    }

    std::string line;
    reset(); // 为新文件重置状态

    while (std::getline(file, line)) {
        m_line_number++;
        ValidationResult vr = m_validator.validate(line);

        if (vr.type == "empty" || vr.type == "remark") {
            continue;
        }

        if (vr.type == "date") {
            m_current_date = vr.matches[0];
            m_current_parent.clear(); // 新日期重置父类别
            m_current_child.clear();  // 新日期重置子类别
        } else if (vr.type == "parent") {
            if (m_current_date.empty()) {
                errors.push_back("Line " + std::to_string(m_line_number) + ": Parent category '" + vr.matches[0] + "' found before any DATE line.");
                continue; // 继续寻找下一个错误
            }
            m_current_parent = vr.matches[0];
            m_current_child.clear(); // 新父类别重置子类别
        } else if (vr.type == "child") {
            if (m_current_parent.empty()) {
                errors.push_back("Line " + std::to_string(m_line_number) + ": Child category '" + vr.matches[0] + "' found without a parent category.");
                continue;
            }
            if (m_validator.is_valid_child_for_parent(m_current_parent, vr.matches[0])) {
                m_current_child = vr.matches[0]; // 关系有效，设置当前子类别
            } else {
                errors.push_back("Line " + std::to_string(m_line_number) + ": Child category '" + vr.matches[0] + "' is not a valid child for parent '" + m_current_parent + "'.");
                m_current_child.clear(); // 关系无效，清空
            }
        } else if (vr.type == "item") {
            bool has_error = false;
            if (m_current_date.empty()) {
                errors.push_back("Line " + std::to_string(m_line_number) + ": Item '" + line + "' found without a DATE context.");
                has_error = true;
            }
            if (m_current_parent.empty()) {
                errors.push_back("Line " + std::to_string(m_line_number) + ": Item '" + line + "' found without a PARENT context.");
                has_error = true;
            }
            if (m_current_child.empty()) {
                errors.push_back("Line " + std::to_string(m_line_number) + ": Item '" + line + "' found without a valid CHILD context.");
                has_error = true;
            }
            if(has_error) continue; // 如果有上下文错误，则不处理该条目

            // 如果上下文都有效，则创建记录
            ParsedRecord record;
            record.date = m_current_date;
            record.parent_category = m_current_parent;
            record.child_category = m_current_child;
            record.amount = std::stod(vr.matches[0]);
            record.item_description = vr.matches[1];
            handler(record); // 将有效记录传递给处理函数

        } else if (vr.type == "unrecognized") {
            errors.push_back("Line " + std::to_string(m_line_number) + ": Unrecognized line format: '" + vr.matches[0] + "'.");
        }
    }

    return errors;
}