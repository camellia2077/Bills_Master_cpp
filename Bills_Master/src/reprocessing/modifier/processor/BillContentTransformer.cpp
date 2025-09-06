// reprocessing/modifier/processor/BillContentTransformer.cpp

#include "BillContentTransformer.hpp"
#include "BillProcessor.hpp" // --- 引入新的预处理器 ---
#include "BillParser.hpp"    // --- 引入新的解析器 ---

#include <sstream>
#include <algorithm>
#include <stdexcept>

BillContentTransformer::BillContentTransformer(const Config& config) : m_config(config) {}

std::vector<ParentItem> BillContentTransformer::process(const std::string& bill_content, std::vector<std::string>& out_metadata_lines) {
    // 1. 将原始字符串按行分割
    std::vector<std::string> lines = _split_string_by_lines(bill_content);

    // 2. 使用 BillProcessor 对文本行进行预处理
    BillProcessor preprocessor(m_config);
    preprocessor.process(lines);

    // 3. 使用 BillParser 将处理后的行解析为结构化数据
    BillParser parser(m_config);
    std::vector<ParentItem> bill_structure = parser.parse(lines, out_metadata_lines);

    // 4. 对结构化数据进行排序和清理
    _sort_bill_structure(bill_structure);
    _cleanup_bill_structure(bill_structure);
    
    return bill_structure;
}

// ===================================================================
// Private Member Function Implementations (只保留了排序、清理和工具函数)
// ===================================================================

void BillContentTransformer::_sort_bill_structure(std::vector<ParentItem>& bill_structure) const {
    for (auto& parent : bill_structure) {
        for (auto& sub_item : parent.sub_items) {
            std::sort(sub_item.contents.begin(), sub_item.contents.end(), 
                [](const std::string& a, const std::string& b) {
                    double val_a = _get_numeric_value_from_content(a);
                    double val_b = _get_numeric_value_from_content(b);
                    if (val_a != val_b) {
                        return val_a > val_b;
                    }
                    return a < b;
                });
        }
    }
}

void BillContentTransformer::_cleanup_bill_structure(std::vector<ParentItem>& bill_structure) const {
    for (auto& parent : bill_structure) {
        parent.sub_items.erase(
            std::remove_if(parent.sub_items.begin(), parent.sub_items.end(),
                [](const SubItem& sub) { return sub.contents.empty(); }),
            parent.sub_items.end());
    }

    bill_structure.erase(
        std::remove_if(bill_structure.begin(), bill_structure.end(),
            [](const ParentItem& parent) { return parent.sub_items.empty(); }),
        bill_structure.end());
}

// --- Static Helper Function Implementations ---

std::vector<std::string> BillContentTransformer::_split_string_by_lines(const std::string& str) {
    std::vector<std::string> lines;
    std::string line;
    std::istringstream stream(str);
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

double BillContentTransformer::_get_numeric_value_from_content(const std::string& content_line) {
    try {
        size_t pos;
        double val = std::stod(content_line, &pos);
        return (pos == 0) ? 0.0 : val;
    } catch (const std::invalid_argument&) {
        return 0.0;
    }
}