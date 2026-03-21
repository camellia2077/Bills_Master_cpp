// bills_io/adapters/db/bill_inserter.hpp

#ifndef BILLS_IO_ADAPTERS_DB_BILL_INSERTER_H_
#define BILLS_IO_ADAPTERS_DB_BILL_INSERTER_H_

#include <string>

#include "domain/bill/bill_record.hpp"

/**
 * @class BillInserter
 * @brief 服务层类，负责编排将账单数据插入数据库的业务逻辑。
 */
class BillInserter {
 public:
  explicit BillInserter(std::string db_path);

  /**
   * @brief 执行一个完整的账单插入事务：先删除旧数据，再插入新数据。
   * @param bill_data 从JSON文件解析出的完整账单数据。
   * @throws std::runtime_error 如果在处理过程中发生任何数据库错误。
   */
  void insert_bill(const ParsedBill& bill_data);

 private:
  std::string m_db_path;  // 只存储数据库路径，而不是连接句柄
};

#endif  // BILLS_IO_ADAPTERS_DB_BILL_INSERTER_H_
