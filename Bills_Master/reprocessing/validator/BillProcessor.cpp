#include "BillProcessor.h"
#include "BillConfig.h"
#include "ValidationResult.h"
#include <fstream>
#include <regex>

void BillProcessor::_reset_state() {
    bill_structure.clear();
}

bool BillProcessor::validate(const std::string& bill_file_path, const BillConfig& config, ValidationResult& result) {
    _reset_state();
    result.clear(); // 确保从干净的状态开始

    std::ifstream file(bill_file_path);
    if (!file.is_open()) {
        result.add_error("严重错误: 无法打开账单文件 '" + bill_file_path + "'");
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
            result.add_warning("警告 (文件结尾): 子标题 '" + current_sub + "' 缺少内容行。");
        }
    }

    _post_validation_checks(result);

    return !result.has_errors();
}

void BillProcessor::_process_line(const std::string& line, int line_num, State& current_state, std::string& current_parent, std::string& current_sub, const BillConfig& config, ValidationResult& result) {
    switch (current_state) {
        case State::EXPECT_PARENT:
            _handle_parent_state(line, line_num, current_state, current_parent, config, result);
            break;
        case State::EXPECT_SUB:
            if (!current_sub.empty() && bill_structure.count(current_parent) && bill_structure[current_parent].count(current_sub) && bill_structure[current_parent][current_sub] == 0) {
                 result.add_warning("警告 (行 " + std::to_string(line_num) + "): 子标题 '" + current_sub + "' 缺少内容行。");
            }
            current_sub.clear(); // 为新的子标题重置
            _handle_sub_state(line, line_num, current_state, current_parent, current_sub, config, result);
            break;
        case State::EXPECT_CONTENT:
            _handle_content_state(line, line_num, current_state, current_parent, current_sub, config, result);
            break;
    }
}

bool BillProcessor::_validate_date_and_remark(std::ifstream& file, int& line_num, ValidationResult& result) {
    std::string line;
    
    if (std::getline(file, line)) {
        line_num++;
        std::regex date_regex(R"(^DATE:\d{6}$)");
        if (!std::regex_match(line, date_regex)) {
            result.add_error("错误 (行 " + std::to_string(line_num) + "): 文件第一行必须是 'DATE:YYYYMM' 格式。找到: '" + line + "'");
            return false;
        }
    } else {
        result.add_error("错误: 文件为空或少于两行。");
        return false;
    }

    if (std::getline(file, line)) {
        line_num++;
        std::regex remark_regex(R"(^REMARK:.*)");
        if (!std::regex_match(line, remark_regex)) {
            result.add_error("错误 (行 " + std::to_string(line_num) + "): 文件第二行必须以 'REMARK:' 开头。找到: '" + line + "'");
            return false;
        }
    } else {
        result.add_error("错误: 文件少于两行。");
        return false;
    }
    
    return true;
}

void BillProcessor::_handle_parent_state(const std::string& line, int line_num, State& current_state, std::string& current_parent, const BillConfig& config, ValidationResult& result) {
    if (config.is_parent_title(line)) {
        current_parent = line;
        bill_structure[current_parent]; // 注册父标题
        current_state = State::EXPECT_SUB;
    } else {
        result.add_error("错误 (行 " + std::to_string(line_num) + "): 期望一个父标题, 但找到无效内容: '" + line + "'");
    }
}

void BillProcessor::_handle_sub_state(const std::string& line, int line_num, State& current_state, std::string& current_parent, std::string& current_sub, const BillConfig& config, ValidationResult& result) {
    if (config.is_parent_title(line)) {
        result.add_error("错误 (行 " + std::to_string(line_num) + "): 父级标题 '" + current_parent + "' 缺少子标题。");
        _handle_parent_state(line, line_num, current_state, current_parent, config, result);
        return;
    }

    if (config.is_valid_sub_title(current_parent, line)) {
        current_sub = line;
        bill_structure[current_parent][current_sub] = 0; 
        current_state = State::EXPECT_CONTENT;
    } else {
        result.add_error("错误 (行 " + std::to_string(line_num) + "): 子标题 '" + line + "' 对于父级标题 '" + current_parent + "' 无效。");
        // 即使无效，也记录下来并继续处理内容，以发现更多问题
        current_sub = line;
        bill_structure[current_parent][current_sub] = 0;
        current_state = State::EXPECT_CONTENT;
    }
}

void BillProcessor::_handle_content_state(const std::string& line, int line_num, State& current_state, std::string& current_parent, std::string& current_sub, const BillConfig& config, ValidationResult& result) {
    if (config.is_parent_title(line)) {
        if (!current_sub.empty() && bill_structure[current_parent][current_sub] == 0) {
            result.add_warning("警告 (行 " + std::to_string(line_num) + "): 子标题 '" + current_sub + "' 缺少内容行。");
        }
        _handle_parent_state(line, line_num, current_state, current_parent, config, result);
        current_sub.clear();
        return;
    }

    if (config.is_valid_sub_title(current_parent, line)) {
        if (!current_sub.empty() && bill_structure[current_parent][current_sub] == 0) {
            result.add_warning("警告 (行 " + std::to_string(line_num) + "): 子标题 '" + current_sub + "' 缺少内容行。");
        }
        _handle_sub_state(line, line_num, current_state, current_parent, current_sub, config, result);
        return;
    }

    std::regex content_regex(R"(^\d+(?:\.\d+)?(?:[^\d\s].*)$)");
    if (std::regex_match(line, content_regex)) {
        bill_structure[current_parent][current_sub]++;
    } else {
        result.add_error("错误 (行 " + std::to_string(line_num) + "): 期望内容行、新子标题或新父标题, 但找到无效内容: '" + line + "'");
    }
}

void BillProcessor::_post_validation_checks(ValidationResult& result) {
    for (const auto& parent_pair : bill_structure) {
        const std::string& parent_title = parent_pair.first;
        const auto& sub_map = parent_pair.second;

        if (sub_map.empty()) {
            // 这个错误在 _handle_sub_state 中已经被更即时地捕捉了，这里作为最终保障
             result.add_error("错误 (文件结尾): 父级标题 '" + parent_title + "' 缺少子标题。");
        } else {
            bool all_subs_empty = true;
            for (const auto& sub_pair : sub_map) {
                if (sub_pair.second > 0) {
                    all_subs_empty = false;
                    break;
                }
            }
            if (all_subs_empty) {
                result.add_warning("警告 (文件结尾): 父标题 '" + parent_title + "' 的所有子标题均缺少内容行。");
            }
        }
    }
}
