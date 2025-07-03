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

    // **MODIFIED**: This method now returns the report string.
    std::string get_yearly_summary_report(const std::string& year);
    
    // This method already returns the report string.
    std::string get_monthly_details_report(const std::string& month);

private:
    sqlite3* m_db;
};

#endif // QUERY_FACADE_H