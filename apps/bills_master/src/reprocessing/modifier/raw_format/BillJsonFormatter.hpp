// reprocessing/modifier/raw_format/BillJsonFormatter.hpp

#ifndef BILL_JSON_FORMATTER_HPP
#define BILL_JSON_FORMATTER_HPP

#include "reprocessing/modifier/_shared_structures/BillDataStructures.hpp"
#include "nlohmann/json.hpp"
#include <map>
#include <string>
#include <vector>

/**
 * @class BillJsonFormatter
 * @brief 负责将结构化的账单数据（ParentItem 列表）格式化为最终的 JSON 字符串。
 * * 该类处理数据的序列化，包括计算各个层级的总额（SubTotal）以及
 * 整个账单的总收入（Total Income）、总支出（Total Expense）和结余（Balance）。
 */
class BillJsonFormatter {
public:
    BillJsonFormatter() = default;

    /**
     * @brief 执行格式化操作。
     * * @param bill_structure 已经过解析和排序的账单结构化数据。
     * @param metadata_lines 包含元数据（如 date, remark）的字符串列表。
     * @return std::string 格式化后的 JSON 字符串（缩进为 4 空格）。
     */
    std::string format(
        const std::vector<ParentItem>& bill_structure, 
        const std::vector<std::string>& metadata_lines) const;

private:
    /**
     * @brief 解析单行内容，提取金额、描述和注释。
     * * @param line 原始的内容行字符串（例如 "-30.00 肯德基 // 午饭"）。
     * @param[out] amount 解析出的金额（例如 -30.00）。
     * @param[out] description 解析出的描述（例如 "肯德基"）。
     * @param[out] comment 解析出的注释（例如 "午饭"），如果没有则为空。
     */
    void _parse_content_line(const std::string& line, double& amount, std::string& description, std::string& comment) const;
};

#endif // BILL_JSON_FORMATTER_HPP