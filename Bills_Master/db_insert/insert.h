#ifndef INSERT_H
#define INSERT_H

#include "parser.h" // 依赖于解析器定义的数据结构
#include <sqlite3.h>
#include <string>
#include <stdexcept>

/**
 * @class BillInserter
 * @brief 将解析后的账单数据插入到 SQLite 数据库中。
 *
 * 管理数据库连接，创建表结构，并提供一个方法
 * 来插入整个 ParsedBill 对象的数据。
 */
class BillInserter {
public:
    /**
     * @brief 构造函数，打开数据库连接并初始化表。
     * @param db_path SQLite 数据库文件的路径。
     */
    explicit BillInserter(const std::string& db_path);

    /**
     * @brief 析构函数，关闭数据库连接。
     */
    ~BillInserter();

    /**
     * @brief 将一个完整的 ParsedBill 对象插入数据库。
     * @param bill_data 包含要插入数据的一个 const 引用。
     */
    void insert_bill(const ParsedBill& bill_data);

private:
    sqlite3* m_db; // SQLite 数据库连接句柄

    /**
     * @brief 创建数据库表（如果表不存在）。
     */
    void initialize_database();
};

#endif // INSERT_H