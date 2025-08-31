// modifier/raw_format/BillJsonFormatter.hpp

#ifndef BILL_JSON_FORMATTER_H
#define BILL_JSON_FORMATTER_H

#include "reprocessing/modifier/_shared_structures/BillDataStructures.hpp"
#include "nlohmann/json.hpp"

class BillJsonFormatter {
public:
    // 构造函数可以为空，因为格式化逻辑不依赖外部配置
    BillJsonFormatter() = default;

    /**
     * @brief 将结构化的账单数据转换为半扁平化的 JSON 字符串。
     * @param bill_structure 从 BillContentTransformer 传入的结构化数据。
     * @param metadata_lines 包含 DATE 和 REMARK 的元数据行。
     * @return 格式化后的 JSON 字符串。
     */
    std::string format(const std::vector<ParentItem>& bill_structure, const std::vector<std::string>& metadata_lines) const;

private:
    /**
     * @brief 从内容行中解析出金额和描述。
     * @param line 原始的内容行，例如 "435.53饭"。
     * @param amount 用于接收解析出的金额的引用。
     * @param description 用于接收解析出的描述的引用。
     */
    void _parse_content_line(const std::string& line, double& amount, std::string& description) const;
};

#endif // BILL_JSON_FORMATTER_H