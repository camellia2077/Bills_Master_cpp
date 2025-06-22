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
    m_state = ParserState::EXPECTING_DATE; // 初始状态：期望找到一个DATE行
}

std::vector<std::string> Bill_Parser::parseFile(
    const std::string& file_path,
    std::function<void(const ParsedRecord&)> handler)
{
    std::vector<std::string> errors;
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + file_path);
    }

    std::string line;
    reset(); // 为新文件重置状态

    while (std::getline(file, line)) {
        m_line_number++;
        ValidationResult vr = m_validator.validate(line);

        if (vr.type == "empty") { // 空行在任何状态下都直接跳过
            continue;
        }

        switch (m_state) {
            case ParserState::EXPECTING_DATE:
                if (vr.type == "date") {
                    m_current_date = vr.matches[0];
                    m_current_parent.clear();
                    m_current_child.clear();
                    m_state = ParserState::EXPECTING_REMARK; // 找到DATE，下一个必须是REMARK
                } else {
                    errors.push_back("Line " + std::to_string(m_line_number) + ": Expected a DATE line to start a new section, but found '" + line + "'.");
                }
                break;

            case ParserState::EXPECTING_REMARK:
                if (vr.type == "remark") {
                    m_state = ParserState::PROCESSING_CONTENT; // 找到REMARK，接下来可以处理内容了
                    break; // 找到remark后，跳出switch，处理下一行
                } else {
                    // 记录错误：REMARK行缺失
                    errors.push_back("Line " + std::to_string(m_line_number) + ": Expected a REMARK line after a DATE line, but found '" + line + "'.");
                    // 假设REMARK只是被遗漏了，将状态推进到处理内容
                    m_state = ParserState::PROCESSING_CONTENT;
                }



            case ParserState::PROCESSING_CONTENT:
                if (vr.type == "date") { // 遇到新的DATE，开始新的循环
                    m_current_date = vr.matches[0];
                    m_current_parent.clear();
                    m_current_child.clear();
                    m_state = ParserState::EXPECTING_REMARK; // 同样，新DATE后需要一个REMARK
                } else if (vr.type == "remark") {
                    errors.push_back("Line " + std::to_string(m_line_number) + ": Unexpected REMARK line. A REMARK line must immediately follow a DATE line.");
                } else if (vr.type == "parent") {
                    m_current_parent = vr.matches[0];
                    m_current_child.clear();
                } else if (vr.type == "invalid_parent") {
                    errors.push_back("Line " + std::to_string(m_line_number) + ": Invalid parent category '" + vr.matches[0] + "'.");
                    m_current_parent.clear();
                    m_current_child.clear();
                } else if (vr.type == "child") {
                    if (m_current_parent.empty()) {
                        errors.push_back("Line " + std::to_string(m_line_number) + ": Child category '" + vr.matches[0] + "' found without a parent category.");
                    } else if (m_validator.is_valid_child_for_parent(m_current_parent, vr.matches[0])) {
                        m_current_child = vr.matches[0];
                    } else {
                        errors.push_back("Line " + std::to_string(m_line_number) + ": Child category '" + vr.matches[0] + "' is not a valid child for parent '" + m_current_parent + "'.");
                        m_current_child.clear();
                    }
                } else if (vr.type == "item") {
                    bool has_error = false;
                    if (m_current_parent.empty()) {
                        errors.push_back("Line " + std::to_string(m_line_number) + ": Item '" + line + "' found without a PARENT context.");
                        has_error = true;
                    }
                    if (m_current_child.empty()) {
                        errors.push_back("Line " + std::to_string(m_line_number) + ": Item '" + line + "' found without a valid CHILD context.");
                        has_error = true;
                    }
                    if(has_error) continue;

                    ParsedRecord record;
                    record.date = m_current_date;
                    record.parent_category = m_current_parent;
                    record.child_category = m_current_child;
                    record.amount = std::stod(vr.matches[0]);
                    record.item_description = vr.matches[1];
                    handler(record);
                } else if (vr.type == "unrecognized") {
                    errors.push_back("Line " + std::to_string(m_line_number) + ": Unrecognized line format: '" + vr.matches[0] + "'.");
                }
                break;
        }
    }

    // 在文件末尾进行最终状态检查
    if (m_state == ParserState::EXPECTING_REMARK) {
        errors.push_back("Error: File ended unexpectedly. A REMARK line was expected after the last DATE line.");
    }

    return errors;
}