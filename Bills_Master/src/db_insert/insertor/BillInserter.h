#ifndef INSERT_H
#define INSERT_H
#include "db_insert/bill_structures/BillStructures.h"
#include <sqlite3.h>
#include <string>
#include <stdexcept>
#include <vector> // 为 Transaction 列表添加

class BillInserter {
public:
    explicit BillInserter(const std::string& db_path);
    ~BillInserter();

    void insert_bill(const ParsedBill& bill_data);

private:
    sqlite3* m_db; // SQLite 数据库连接句柄
    
    /**
     * @brief 初始化数据库，创建表（如果不存在）。
     */
    void initialize_database();

    // --- NEW PRIVATE HELPER METHODS ---
    /**
     * @brief 根据账单日期删除一个账单及其所有关联的交易。
     * @param date 账单日期 (格式: "YYYY-MM")。
     */
    void delete_bill_by_date(const std::string& date);

    /**
     * @brief 向 'bills' 表插入一条新的账单记录。
     * @param bill_data 包含账单元数据的已解析账单对象。
     * @return 新插入的账单记录的ID。
     */
    sqlite3_int64 insert_bill_record(const ParsedBill& bill_data);
    
    /**
     * @brief 为指定的账单ID插入所有交易记录。
     * @param bill_id 与这些交易关联的 'bills' 表的主键。
     * @param transactions 要插入的交易记录的向量。
     */
    void insert_transactions_for_bill(sqlite3_int64 bill_id, const std::vector<Transaction>& transactions);
};
#endif