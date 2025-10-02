// reprocessing/modifier/processor/BillParser.hpp

#ifndef BILL_PARSER_HPP
#define BILL_PARSER_HPP

#include "reprocessing/modifier/_shared_structures/BillDataStructures.hpp"
#include <vector>
#include <string>

/**
 * @class BillParser
 * @brief 负责将预处理过的文本行解析成结构化的 ParentItem 数据。
 */
class BillParser {
public:
    explicit BillParser(const Config& config);

    /**
     * @brief 执行解析。
     * @param lines 预处理过的文本行。
     * @param out_metadata_lines 用于存储元数据行的向量。
     * @return 返回结构化的账单数据。
     */
    std::vector<ParentItem> parse(const std::vector<std::string>& lines, std::vector<std::string>& out_metadata_lines) const;

private:
    const Config& m_config;

    bool _is_metadata_line(const std::string& line) const;
    static bool _is_parent_title(const std::string& line);
    static bool _is_title(const std::string& line);
    static std::string& _trim(std::string& s);
};

#endif // BILL_PARSER_HPP