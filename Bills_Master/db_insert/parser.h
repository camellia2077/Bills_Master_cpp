#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <stdexcept>

// 代表一笔具体的交易记录
struct Transaction {
    std::string parent_category;
    std::string sub_category;
    double amount;
    std::string description;
};

// 代表整个解析后的账单文件
struct ParsedBill {
    std::string date; // 保留原始的 YYYYMM 字符串
    int year = 0;      // 新增：年份
    int month = 0;     // 新增：月份
    std::string remark;
    std::vector<Transaction> transactions;
};

/**
 * @class BillParser
 * @brief 从指定的文本文件中解析账单数据。
 *
 * 读取一个遵循特定格式的账单文件，并将其内容转换为
 * 一个结构化的 ParsedBill 对象，以便后续处理。
 */
class BillParser {
public:
    /**
     * @brief 解析指定的账单文件。
     * @param file_path 要解析的账单文件的路径。
     * @return 一个包含所有解析数据的 ParsedBill 结构体。
     * @throws std::runtime_error 如果文件无法打开或日期格式不正确。
     */
    ParsedBill parse(const std::string& file_path);

private:
    // 辅助函数，用于判断行的类型
    bool is_parent_title(const std::string& line);
    bool is_sub_title(const std::string& line);
};

#endif // PARSER_H