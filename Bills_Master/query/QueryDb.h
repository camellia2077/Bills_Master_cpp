#ifndef QUERY_FACADE_H
#define QUERY_FACADE_H

#include <string>
#include <sqlite3.h>

class QueryFacade {
public:
    explicit QueryFacade(const std::string& db_path);
    ~QueryFacade();

    // 禁止拷贝和赋值，因为我们自己管理资源 (数据库连接)
    QueryFacade(const QueryFacade&) = delete;
    QueryFacade& operator=(const QueryFacade&) = delete;

    void show_yearly_summary(const std::string& year);
    void show_monthly_details(const std::string& month);

private:
    sqlite3* m_db;
};

#endif // QUERY_FACADE_H