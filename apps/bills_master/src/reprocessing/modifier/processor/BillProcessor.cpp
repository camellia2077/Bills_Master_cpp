// reprocessing/modifier/processor/BillProcessor.cpp

#include "BillProcessor.hpp"
#include <regex>
#include <sstream>
#include <algorithm>
#include <iomanip>

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

// --- 【核心修改】 ---
// 重写此函数以处理包含加减法的表达式
void BillProcessor::_sum_up_line(std::string& line) {
    // 匹配一个完整的数学表达式（可能包含+和-）以及后面的描述
    std::regex expr_regex(R"(^([+-]?\s*\d+(?:\.\d+)?(?:\s*[+-]\s*\d+(?:\.\d+)?)*)(.*))");
    std::smatch match;

    if (std::regex_search(line, match, expr_regex) && match.size() > 2) {
        std::string expression = match[1].str();
        std::string description = match[2].str();

        // 移除表达式中的所有空白字符
        expression.erase(std::remove(expression.begin(), expression.end(), ' '), expression.end());

        // 如果表达式不是以+或-开头，在前面加上一个+，方便解析
        if (expression.front() != '+' && expression.front() != '-') {
            expression.insert(0, 1, '+');
        }

        double sum = 0.0;
        std::regex token_regex(R"(([+-])(\d+(?:\.\d+)?))");
        auto words_begin = std::sregex_iterator(expression.begin(), expression.end(), token_regex);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch token_match = *i;
            char op = token_match[1].str()[0];
            double value = std::stod(token_match[2].str());

            if (op == '+') {
                sum += value;
            } else if (op == '-') {
                sum -= value;
            }
        }

        std::stringstream result_ss;
        result_ss << std::fixed << std::setprecision(2) << sum << description;
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