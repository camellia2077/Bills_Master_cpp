#ifndef BILL_METADATA_READER_H
#define BILL_METADATA_READER_H

#include <sqlite3.h>
#include <string>
#include <vector>

// 用于获取所有日期，以便导出所有的内容
class BillMetadataReader {
public:
    explicit BillMetadataReader(sqlite3* db_connection);

    // 从 QueryDb 移动过来的函数
    std::vector<std::string> get_all_bill_dates();

private:
    sqlite3* m_db;
};

#endif // BILL_METADATA_READER_H