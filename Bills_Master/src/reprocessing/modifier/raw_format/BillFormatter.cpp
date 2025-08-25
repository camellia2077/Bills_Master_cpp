
#include "BillFormatter.h"
#include <sstream>
#include <iomanip>

BillFormatter::BillFormatter(const Config& config) : m_config(config) {}

std::string BillFormatter::format(const std::vector<ParentItem>& bill_structure, const std::vector<std::string>& metadata_lines) const {
    std::stringstream ss;
    
    for (const auto& meta_line : metadata_lines) {
        ss << meta_line << "\n";
    }
    
    if (!metadata_lines.empty() && !bill_structure.empty()) {
        ss << "\n";
    }

    bool first_parent = true;
    for (const auto& parent : bill_structure) {
        if (!first_parent) {
            for(int i = 0; i < m_config.formatting.lines_after_parent_section; ++i) ss << "\n";
        }
        ss << parent.title;
        first_parent = false;

        for(int i = 0; i < m_config.formatting.lines_after_parent_title; ++i) ss << "\n";

        bool first_sub_item = true;
        for (const auto& sub_item : parent.sub_items) {
            if (!first_sub_item) {
                for(int i = 0; i < m_config.formatting.lines_between_sub_items; ++i) ss << "\n";
            }
            ss << sub_item.title;
            first_sub_item = false;

            for (const auto& content : sub_item.contents) {
                ss << "\n" << content;
            }
        }
    }
    return ss.str();
}