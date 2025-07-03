#ifndef QUERY_AGENT_H
#define QUERY_AGENT_H

#include <string>
#include <vector>
#include <sqlite3.h>
#include "parser.h" // We can reuse the Transaction struct for convenience

/**
 * @class QueryAgent
 * @brief 从 SQLite 数据库中查询并显示账单数据。
 *
 * 管理数据库连接，并提供按年和按月查询数据的方法。
 */
class QueryAgent {
public:
    /**
     * @brief 构造函数，打开一个只读的数据库连接。
     * @param db_path SQLite 数据库文件的路径。
     */
    explicit QueryAgent(const std::string& db_path);

    /**
     * @brief 析构函数，关闭数据库连接。
     */
    ~QueryAgent();

    /**
     * @brief 查询并显示指定年份每个月的总支出。
     * @param year 要查询的年份，格式为 "YYYY"。
     */
    void display_yearly_summary(const std::string& year);

    /**
     * @brief 查询并显示指定月份的所有交易记录。
     * @param month 要查询的月份，格式为 "YYYYMM"。
     */
    void display_monthly_details(const std::string& month);

private:
    sqlite3* m_db; // SQLite 数据库连接句柄
};

#endif // QUERY_AGENT_H