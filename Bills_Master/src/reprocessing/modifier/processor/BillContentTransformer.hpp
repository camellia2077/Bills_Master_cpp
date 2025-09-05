// reprocessing/modifier/processor/BillContentTransformer.hpp

#ifndef BILL_CONTENT_TRANSFORMER_H
#define BILL_CONTENT_TRANSFORMER_H

#include "reprocessing/modifier/_shared_structures/BillDataStructures.hpp"
#include <vector>
#include <string>

/**
 * @class BillContentTransformer
 * @brief 协调将原始账单文本转换为结构化数据的整个流程。
 */
class BillContentTransformer {
public:
    explicit BillContentTransformer(const Config& config);

    /**
     * @brief 执行完整的转换流程。
     * @param bill_content 原始的账单文件内容字符串。
     * @param out_metadata_lines 用于存储元数据行的向量。
     * @return 返回最终经过处理和结构化的账单数据。
     */
    std::vector<ParentItem> process(const std::string& bill_content, std::vector<std::string>& out_metadata_lines);

private:
    const Config& m_config;

    // --- Private Member Functions ---
    void _sort_bill_structure(std::vector<ParentItem>& bill_structure) const;
    void _cleanup_bill_structure(std::vector<ParentItem>& bill_structure) const;

    // --- Private Static Helper Functions ---
    static std::vector<std::string> _split_string_by_lines(const std::string& str);
    static double _get_numeric_value_from_content(const std::string& content_line);
};

#endif // BILL_CONTENT_TRANSFORMER_H