// src/reprocessing/validator/verifier/BillFormatVerifier.cpp (重构后)

#include "BillFormatVerifier.hpp"
#include "reprocessing/validator/config/BillConfig.hpp"
#include "reprocessing/validator/result/ValidationResult.hpp"
#include <regex>

// --- 公共入口: 现在非常清晰，像一个流程总结 ---
bool BillFormatVerifier::validate(const std::string& bill_file_path, const BillConfig& config, ValidationResult& result) {
    _initialize_validation(config, result);

    std::ifstream file(bill_file_path);
    if (!file.is_open()) {
        m_result->add_error("Critical Error: Unable to open bill file '" + bill_file_path + "'");
        return false;
    }

    if (_process_file_header(file)) {
        _process_file_body(file);
    }
    
    _finalize_validation();

    return !m_result->has_errors();
}

// --- 提炼出的新方法 1: 初始化 ---
void BillFormatVerifier::_initialize_validation(const BillConfig& config, ValidationResult& result) {
    m_bill_structure.clear();
    result.clear();
    
    m_current_state = State::EXPECT_PARENT;
    m_current_parent.clear();
    m_current_sub.clear();
    m_line_num = 0;
    m_config = &config;
    m_result = &result;
}

// --- 提炼出的新方法 2: 处理文件头 ---
bool BillFormatVerifier::_process_file_header(std::ifstream& file) {
    std::string line;
    
    // 验证 DATE
    if (std::getline(file, line)) {
        m_line_num++;
        if (!std::regex_match(line, std::regex(R"(^DATE:\d{6}$)"))) {
            m_result->add_error("Error (Line " + std::to_string(m_line_num) + "): The first line must be 'DATE:YYYYMM'. Found: '" + line + "'");
            return false;
        }
    } else {
        m_result->add_error("Error: File is empty or has less than two lines.");
        return false;
    }

    // 验证 REMARK
    if (std::getline(file, line)) {
        m_line_num++;
        if (!std::regex_match(line, std::regex(R"(^REMARK:.*)"))) {
            m_result->add_error("Error (Line " + std::to_string(m_line_num) + "): The second line must start with 'REMARK:'. Found: '" + line + "'");
            return false;
        }
    } else {
        m_result->add_error("Error: File has less than two lines.");
        return false;
    }
    
    return true;
}

// --- 提炼出的新方法 3: 处理文件主体 ---
void BillFormatVerifier::_process_file_body(std::ifstream& file) {
    std::string line;
    while (std::getline(file, line)) {
        m_line_num++;
        if (line.empty()) continue;
        _process_line(line);
    }
}

// --- 提炼出的新方法 4: 收尾工作 ---
void BillFormatVerifier::_finalize_validation() {
    // 检查最后一个子标题是否有内容
    if (!m_current_sub.empty() && m_bill_structure[m_current_parent][m_current_sub] == 0) {
        m_result->add_warning("Warning (End of File): Sub-title '" + m_current_sub + "' is missing content lines.");
    }

    // 执行所有后置检查
    for (const auto& parent_pair : m_bill_structure) {
        const std::string& parent_title = parent_pair.first;
        const auto& sub_map = parent_pair.second;

        if (sub_map.empty()) {
            m_result->add_error("Error (End of File): Parent title '" + parent_title + "' is missing a sub-title.");
        } else {
            bool all_subs_empty = true;
            for (const auto& sub_pair : sub_map) {
                if (sub_pair.second > 0) {
                    all_subs_empty = false;
                    break;
                }
            }
            if (all_subs_empty) {
                m_result->add_warning("Warning (End of File): All sub-titles under '" + parent_title + "' are empty.");
            }
        }
    }
}


// --- 核心的 process_line 保持不变，但现在使用成员变量 ---
void BillFormatVerifier::_process_line(const std::string& line) {
    switch (m_current_state) {
        case State::EXPECT_PARENT:
            _handle_parent_state(line);
            break;
        case State::EXPECT_SUB:
            if (!m_current_sub.empty() && m_bill_structure[m_current_parent][m_current_sub] == 0) {
                m_result->add_warning("Warning (Line " + std::to_string(m_line_num) + "): Sub-title '" + m_current_sub + "' is missing content lines.");
            }
            m_current_sub.clear();
            _handle_sub_state(line);
            break;
        case State::EXPECT_CONTENT:
            _handle_content_state(line);
            break;
    }
}

// --- 状态处理函数现在也使用成员变量 ---
void BillFormatVerifier::_handle_parent_state(const std::string& line) {
    if (m_config->is_parent_title(line)) {
        m_current_parent = line;
        m_bill_structure[m_current_parent];
        m_current_state = State::EXPECT_SUB;
    } else {
        m_result->add_error("Error (Line " + std::to_string(m_line_num) + "): Expected a parent title, but found: '" + line + "'");
    }
}

void BillFormatVerifier::_handle_sub_state(const std::string& line) {
    if (m_config->is_parent_title(line)) {
        m_result->add_error("Error (Line " + std::to_string(m_line_num) + "): Parent title '" + m_current_parent + "' is missing a sub-title.");
        _handle_parent_state(line); // 直接转换到处理父标题的状态
        return;
    }

    if (m_config->is_valid_sub_title(m_current_parent, line)) {
        m_current_sub = line;
        m_bill_structure[m_current_parent][m_current_sub] = 0;
        m_current_state = State::EXPECT_CONTENT;
    } else {
        m_result->add_error("Error (Line " + std::to_string(m_line_num) + "): Sub-title '" + line + "' is invalid for parent '" + m_current_parent + "'.");
        m_current_sub = line;
        m_bill_structure[m_current_parent][m_current_sub] = 0;
        m_current_state = State::EXPECT_CONTENT;
    }
}

void BillFormatVerifier::_handle_content_state(const std::string& line) {
    if (m_config->is_parent_title(line)) {
        if (!m_current_sub.empty() && m_bill_structure[m_current_parent][m_current_sub] == 0) {
            m_result->add_warning("Warning (Before Line " + std::to_string(m_line_num) + "): Sub-title '" + m_current_sub + "' was empty.");
        }
        _handle_parent_state(line);
        m_current_sub.clear();
        return;
    }

    if (m_config->is_valid_sub_title(m_current_parent, line)) {
        if (!m_current_sub.empty() && m_bill_structure[m_current_parent][m_current_sub] == 0) {
            m_result->add_warning("Warning (Before Line " + std::to_string(m_line_num) + "): Sub-title '" + m_current_sub + "' was empty.");
        }
        _handle_sub_state(line);
        return;
    }

    std::regex content_regex(R"(^\d+(\.\d+)?\s*.*?(?://.*)?$)");
    std::string trimmed_line = line;
    trimmed_line.erase(0, trimmed_line.find_first_not_of(" \t\n\r"));
    if (trimmed_line.rfind("//", 0) == 0) {
        m_result->add_error("Error (Line " + std::to_string(m_line_num) + "): Comment line '//' cannot appear on its own.");
        return;
    }

    if (std::regex_match(line, content_regex)) {
        m_bill_structure[m_current_parent][m_current_sub]++;
    } else {
        m_result->add_error("Error (Line " + std::to_string(m_line_num) + "): Invalid content line format: '" + line + "'");
    }
}