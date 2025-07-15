#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <string>
#include "parser/BillParser.h"
#include "insertor/BillInserter.h"

/**
 * @class DataProcessor
 * @brief 提供一个简化的接口来处理账单文件并将其存入数据库。
 *
 * 封装了文件解析和数据库插入的整个流程。
 */
class DataProcessor {
public:
    /**
     * @brief 处理单个账单文件并将其内容插入到指定的数据库中。
     *
     * @param bill_file_path 要处理的账单文件的路径。
     * @param db_path 目标 SQLite 数据库的路径。
     * @return 如果整个过程成功，返回 true，否则返回 false。
     */
    bool process_and_insert(const std::string& bill_file_path, const std::string& db_path);
};

#endif // DATA_PROCESSOR_H