// src/db_insert/insertor/BillInserter.cpp

#include "BillInserter.hpp"
#include "DatabaseManager.hpp" // 包含新的数据访问层头文件
#include <stdexcept>
#include <iostream>

BillInserter::BillInserter(const std::string& db_path) : m_db_path(db_path) {}

void BillInserter::insert_bill(const ParsedBill& bill_data) {
    if (bill_data.date.empty()) {
        throw std::runtime_error("无法插入日期为空的账单。");
    }

    // 在方法内部创建DatabaseManager实例，确保其生命周期覆盖整个操作
    DatabaseManager db_manager(m_db_path);

    // 在第一次使用数据库时，确保表已创建
    // 注意：在实际应用中，这通常在程序启动时执行一次即可
    db_manager.initialize_database();

    try {
        // 业务流程步骤 1: 开始事务
        db_manager.begin_transaction();

        // 业务流程步骤 2: 删除可能存在的旧账单
        db_manager.delete_bill_by_date(bill_data.date);

        // 业务流程步骤 3: 插入新的账单记录并获取ID
        sqlite3_int64 bill_id = db_manager.insert_bill_record(bill_data);

        // 业务流程步骤 4: 插入所有关联的交易记录
        db_manager.insert_transactions_for_bill(bill_id, bill_data.transactions);

        // 业务流程步骤 5: 提交事务
        db_manager.commit_transaction();

    } catch (const std::exception& e) {
        // 如果任何步骤失败，回滚事务并重新抛出异常
        std::cerr << "数据库操作失败，正在回滚事务..." << std::endl;
        db_manager.rollback_transaction();
        throw;
    }
}