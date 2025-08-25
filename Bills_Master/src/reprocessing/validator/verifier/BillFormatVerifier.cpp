
#include "BillFormatVerifier.h"

#include "reprocessing/validator/config/BillConfig.h"
#include "reprocessing/validator/result/ValidationResult.h"
#include <fstream>
#include <regex>

void BillFormatVerifier::_reset_state() {
    bill_structure.clear();
}

bool BillFormatVerifier::validate(const std::string& bill_file_path, const BillConfig& config, ValidationResult& result) {
    _reset_state();
    result.clear(); // 确保从干净的状态开始

    std::ifstream file(bill_file_path);
    if (!file.is_open()) {
        result.add_error("Critical Error: Unable to open bill file '" + bill_file_path + "'");
        return false;
    }

    int line_num = 0;
    if (!_validate_date_and_remark(file, line_num, result)) {
        // 如果头部格式错误，通常没有必要继续
    } else {
        State current_state = State::EXPECT_PARENT;
        std::string current_parent;
        std::string current_sub;
        std::string line;

        while (std::getline(file, line)) {
            line_num++;
            if (line.empty()) continue; // 跳过空行
            _process_line(line, line_num, current_state, current_parent, current_sub, config, result);
        }

        // 检查最后一个子标题是否有内容
        if (!current_sub.empty() && bill_structure[current_parent][current_sub] == 0) {
            result.add_warning("Warning (End of File): Sub-title '" + current_sub + "' is missing content lines.");
        }
    }

    _post_validation_checks(result);

    return !result.has_errors();
}

void BillFormatVerifier::_process_line(const std::string& line, int line_num, State& current_state, std::string& current_parent, std::string& current_sub, const BillConfig& config, ValidationResult& result) {
    switch (current_state) {
        case State::EXPECT_PARENT:
            _handle_parent_state(line, line_num, current_state, current_parent, config, result);
            break;
        case State::EXPECT_SUB:
            if (!current_sub.empty() && bill_structure.count(current_parent) && bill_structure[current_parent].count(current_sub) && bill_structure[current_parent][current_sub] == 0) {
                result.add_warning("Warning (Line " + std::to_string(line_num) + "): Sub-title '" + current_sub + "' is missing content lines.");
            }
            current_sub.clear(); // 为新的子标题重置
            _handle_sub_state(line, line_num, current_state, current_parent, current_sub, config, result);
            break;
        case State::EXPECT_CONTENT:
            _handle_content_state(line, line_num, current_state, current_parent, current_sub, config, result);
            break;
    }
}

bool BillFormatVerifier::_validate_date_and_remark(std::ifstream& file, int& line_num, ValidationResult& result) {
    std::string line;
    
    if (std::getline(file, line)) {
        line_num++;
        std::regex date_regex(R"(^DATE:\d{6}$)");
        if (!std::regex_match(line, date_regex)) {
            result.add_error("Error (Line " + std::to_string(line_num) + "): The first line of the file must be in 'DATE:YYYYMM' format. Found: '" + line + "'");
            return false;
        }
    } else {
        result.add_error("Error: File is empty or has less than two lines.");
        return false;
    }

    if (std::getline(file, line)) {
        line_num++;
        std::regex remark_regex(R"(^REMARK:.*)");
        if (!std::regex_match(line, remark_regex)) {
            result.add_error("Error (Line " + std::to_string(line_num) + "): The second line of the file must start with 'REMARK:'. Found: '" + line + "'");
            return false;
        }
    } else {
        result.add_error("Error: File has less than two lines.");
        return false;
    }
    
    return true;
}

void BillFormatVerifier::_handle_parent_state(const std::string& line, int line_num, State& current_state, std::string& current_parent, const BillConfig& config, ValidationResult& result) {
    if (config.is_parent_title(line)) {
        current_parent = line;
        bill_structure[current_parent]; // 注册父标题
        current_state = State::EXPECT_SUB;
    } else {
        result.add_error("Error (Line " + std::to_string(line_num) + "): Expected a parent title, but found invalid content: '" + line + "'");
    }
}

void BillFormatVerifier::_handle_sub_state(const std::string& line, int line_num, State& current_state, std::string& current_parent, std::string& current_sub, const BillConfig& config, ValidationResult& result) {
    if (config.is_parent_title(line)) {
        result.add_error("Error (Line " + std::to_string(line_num) + "): Parent title '" + current_parent + "' is missing a sub-title."); // 翻译: 错误 (行 ...): 父级标题 '...' 缺少子标题。
        _handle_parent_state(line, line_num, current_state, current_parent, config, result);
        return;
    }

    if (config.is_valid_sub_title(current_parent, line)) {
        current_sub = line;
        bill_structure[current_parent][current_sub] = 0; 
        current_state = State::EXPECT_CONTENT;
    } else {
        result.add_error("Error (Line " + std::to_string(line_num) + "): Sub-title '" + line + "' is invalid for parent title '" + current_parent + "'.");
        // 即使无效，也记录下来并继续处理内容，以发现更多问题
        current_sub = line;
        bill_structure[current_parent][current_sub] = 0;
        current_state = State::EXPECT_CONTENT;
    }
}

void BillFormatVerifier::_handle_content_state(const std::string& line, int line_num, State& current_state, std::string& current_parent, std::string& current_sub, const BillConfig& config, ValidationResult& result) {
    if (config.is_parent_title(line)) {
        if (!current_sub.empty() && bill_structure[current_parent][current_sub] == 0) {
            result.add_warning("Warning (Line " + std::to_string(line_num) + "): Sub-title '" + current_sub + "' is missing content lines.");
        }
        _handle_parent_state(line, line_num, current_state, current_parent, config, result);
        current_sub.clear();
        return;
    }

    if (config.is_valid_sub_title(current_parent, line)) {
        if (!current_sub.empty() && bill_structure[current_parent][current_sub] == 0) {
            result.add_warning("Warning (Line " + std::to_string(line_num) + "): Sub-title '" + current_sub + "' is missing content lines.");
        }
        _handle_sub_state(line, line_num, current_state, current_parent, current_sub, config, result);
        return;
    }

    std::regex content_regex(R"(^\d+(?:\.\d+)?(?:[^\d\s].*)$)");
    if (std::regex_match(line, content_regex)) {
        bill_structure[current_parent][current_sub]++;
    } else {
        result.add_error("Error (Line " + std::to_string(line_num) + "): Expected content line, new sub-title, or new parent title, but found invalid content: '" + line + "'"); // 翻译: 错误 (行 ...): 期望内容行、新子标题或新父标题, 但找到无效内容: '...'
    }
}

void BillFormatVerifier::_post_validation_checks(ValidationResult& result) {
    for (const auto& parent_pair : bill_structure) {
        const std::string& parent_title = parent_pair.first;
        const auto& sub_map = parent_pair.second;

        if (sub_map.empty()) {
            // 这个错误在 _handle_sub_state 中已经被更即时地捕捉了，这里作为最终保障
            result.add_error("Error (End of File): Parent title '" + parent_title + "' is missing a sub-title."); // 翻译: 错误 (文件结尾): 父级标题 '...' 缺少子标题。
        } else {
            bool all_subs_empty = true;
            for (const auto& sub_pair : sub_map) {
                if (sub_pair.second > 0) {
                    all_subs_empty = false;
                    break;
                }
            }
            if (all_subs_empty) {
                result.add_warning("Warning (End of File): All sub-titles under parent title '" + parent_title + "' are missing content lines.");
            }
        }
    }
}
