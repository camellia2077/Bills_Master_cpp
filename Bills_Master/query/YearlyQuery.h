#ifndef YEARLY_QUERY_H
#define YEARLY_QUERY_H

#include <string>
#include <sqlite3.h>

class YearlyQuery {
public:
    // 注意：构造函数接收一个已存在的数据库连接句柄
    explicit YearlyQuery(sqlite3* db_connection);

    // 显示指定年份的格式化摘要
    void display(const std::string& year);

private:
    sqlite3* m_db; // 不管理连接的生命周期，只使用它
};

#endif // YEARLY_QUERY_H