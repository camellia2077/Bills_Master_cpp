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

void BillProcessor::_sum_up_line(std::string& line) {
    std::regex expr_regex(R"(^([\d\.\s]+\+[\d\.\s\+]+)(.*))");
    std::smatch match;

    if (std::regex_search(line, match, expr_regex) && match.size() > 1) {
        std::string expression = match[1].str();
        std::string description = match[2].str();
        
        expression.erase(std::remove(expression.begin(), expression.end(), ' '), expression.end());
        std::replace(expression.begin(), expression.end(), '+', ' ');

        std::stringstream ss(expression);
        double sum = 0.0, num;
        while (ss >> num) {
            sum += num;
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