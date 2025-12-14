// reprocessing/modifier/processor/BillProcessor.cpp

#include "BillProcessor.hpp"
#include <regex>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <iostream>

BillProcessor::BillProcessor(const Config& config) : m_config(config) {}

void BillProcessor::process(std::vector<std::string>& lines) {
    for (std::string& line : lines) {
        _sum_up_line(line);
    }
    _apply_auto_renewal(lines);
}

void BillProcessor::_apply_auto_renewal(std::vector<std::string>& lines) {
    if (!m_config.auto_renewal.enabled) {
        return;
    }

    for (const auto& rule : m_config.auto_renewal.rules) {
        const std::string& category_title = rule.header_location;
        
        auto category_it = std::find(lines.begin(), lines.end(), category_title);
        if (category_it == lines.end()) {
            continue;
        }

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

// 重写此函数以处理包含加减乘法的表达式
void BillProcessor::_sum_up_line(std::string& line) {
    // 1. 预先清理行首的 BOM (Byte Order Mark)
    if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF && 
        static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF) {
        line.erase(0, 3);
    }
    
    // 2. 匹配数学表达式部分和描述部分
    // 更新正则：
    // - 允许操作符包含 + - * ×
    // - 使用非捕获组和或运算 (?:[+*\-]|×) 来安全匹配操作符
    // - 注意：在 [] 中 * 是字面量，- 需要放在最后或转义。× 是多字节字符，建议用 | 分隔
    std::regex expr_regex(R"(^([+-]?\s*\d+(?:\.\d+)?(?:[\s\t]*(?:[+*\-]|×)[\s\t]*\d+(?:\.\d+)?)*)(.*))");
    std::smatch match;

    if (std::regex_search(line, match, expr_regex)) {
        std::string expression = match[1].str();
        std::string description = match[2].str();
        
        if (expression.empty()) return;
        
        // 简单检查是否包含数字
        bool has_digit = false;
        for (char c : expression) { if (isdigit(c)) { has_digit = true; break; } }
        if (!has_digit) return;

        // 3. 清理表达式：移除空白，并将 '×' 替换为 '*'
        std::string clean_expr;
        for (size_t i = 0; i < expression.length(); ++i) {
            char c = expression[i];
            if (std::isspace(static_cast<unsigned char>(c))) continue;
            
            // 处理 UTF-8 的 '×' (0xC3 0x97)
            if (static_cast<unsigned char>(c) == 0xC3 && i + 1 < expression.length()) {
                if (static_cast<unsigned char>(expression[i+1]) == 0x97) {
                    clean_expr += '*';
                    i++; // 跳过下一个字节
                    continue;
                }
            }
            clean_expr += c;
        }
        
        if (clean_expr.empty()) return;

        // 4. 应用“默认为支出”的规则
        // 如果以数字或点开头，说明没有显式符号，加上 '-'
        if (isdigit(clean_expr[0]) || clean_expr[0] == '.') {
            clean_expr.insert(0, 1, '-');
        }

        // 5. 解析并计算 (支持优先级：先乘后加减)
        // 算法思路：将表达式看作多个“项”的和。项之间用 + 或 - 分隔。
        // 例如：-30*4+1-10  分解为三项：
        // 1. -30*4 (值 -120)
        // 2. +1    (值 +1)
        // 3. -10   (值 -10)
        // 总和 = -129
        
        double total_sum = 0.0;
        size_t pos = 0; // 当前解析位置
        
        while (pos < clean_expr.length()) {
            // 寻找当前项的结束位置（下一个 + 或 -）
            // 从 pos + 1 开始找，因为 pos 位置本身就是当前项的符号
            size_t next_plus = clean_expr.find('+', pos + 1);
            size_t next_minus = clean_expr.find('-', pos + 1);
            size_t end_of_term = std::min(next_plus, next_minus); // 如果都没找到，min 会取到 npos (即结尾)
            
            // 提取当前项字符串 (例如 "-30*4" 或 "+1")
            std::string term_str = clean_expr.substr(pos, end_of_term - pos);
            
            if (!term_str.empty()) {
                // 确定该项的符号
                double sign = (term_str[0] == '-') ? -1.0 : 1.0;
                
                // 提取数值部分 (去掉首位的 + 或 -)
                std::string val_part = term_str.substr(1);
                
                // 计算该项的乘积值
                double term_value = 1.0;
                if (!val_part.empty()) {
                    std::stringstream ss(val_part);
                    std::string segment;
                    // 按 '*' 分割并累乘
                    while(std::getline(ss, segment, '*')) {
                        if (!segment.empty()) {
                            try {
                                term_value *= std::stod(segment);
                            } catch (...) {
                                term_value = 0.0;
                            }
                        }
                    }
                } else {
                    term_value = 0.0; // 只有符号没有数字的情况
                }
                
                // 累加到总和
                total_sum += sign * term_value;
            }
            
            // 移动到下一项
            pos = end_of_term;
        }

        // 6. 构造新行
        std::stringstream result_ss;
        result_ss << std::fixed << std::setprecision(2) << total_sum;
        
        // 如果描述部分紧挨着数字（没有空格），补充一个空格方便后续解析
        if (!description.empty() && !isspace(description[0])) { result_ss << " "; }
        
        result_ss << description;
        line = result_ss.str();
    }
}


bool BillProcessor::_is_title(const std::string& line) {
    if (line.empty()) return false;
    for(char c : line) {
        if(!isspace(c)) return !isdigit(c);
    }
    return false;
}