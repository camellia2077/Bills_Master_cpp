// db_insert/parser/BillJsonParser.hpp

#ifndef BILL_JSON_PARSER_H
#define BILL_JSON_PARSER_H

#include "common_structures/CommonData.hpp"
#include <string>
#include <stdexcept>

/**
 * @class BillJsonParser
 * @brief 负责从 JSON 文件中解析账单数据。
 *
 * 此类读取由预处理器生成的 JSON 文件，并将其内容填充到
 * ParsedBill 和 Transaction 结构体中，以供后续的数据库插入操作使用。
 */
class BillJsonParser {
public:
    /**
     * @brief 解析指定的 JSON 文件。
     * @param file_path JSON 文件的路径。
     * @return 一个填充了数据的 ParsedBill 结构体。
     * @throws std::runtime_error 如果文件无法打开或 JSON 格式不正确。
     */
    ParsedBill parse(const std::string& file_path);
};

#endif // BILL_JSON_PARSER_H