#ifndef INSERT_H
#define INSERT_H
#include "bill_structures/BillStructures.h" // 依赖于解析器定义的数据结构，现在从独立的头文件引入
#include <sqlite3.h>
#include <string>
#include <stdexcept>

class BillInserter {
public:

explicit BillInserter(const std::string& db_path);

~BillInserter();

void insert_bill(const ParsedBill& bill_data);
private:
sqlite3* m_db; // SQLite 数据库连接句柄
/**
 * @brief 创建数据库表（如果表不存在）。
 */
void initialize_database();
};
#endif