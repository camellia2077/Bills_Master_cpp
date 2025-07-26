#ifndef BILL_PARSER_H
#define BILL_PARSER_H
#include "db_insert/bill_structures/BillStructures.h" // 引入共享数据结构
#include <string>
#include <vector>
#include <stdexcept>

class BillParser {
public:

ParsedBill parse(const std::string& file_path);
private:
// 辅助函数，用于判断行的类型
bool is_parent_title(const std::string& line);
bool is_sub_title(const std::string& line);
};
#endif