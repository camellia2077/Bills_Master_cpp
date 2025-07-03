#include "DataProcessor.h"
#include <iostream>

bool DataProcessor::process_and_insert(const std::string& bill_file_path, const std::string& db_path) {
    try {
        // 步骤 1: 解析文件
        std::cout << "\n--- 开始解析文件: " << bill_file_path << " ---\n";
        BillParser parser;
        ParsedBill bill_data = parser.parse(bill_file_path);
        std::cout << "文件解析成功，找到 " << bill_data.transactions.size() << " 条交易记录。\n";

        // 步骤 2: 将解析后的数据插入数据库
        std::cout << "--- 开始将数据插入数据库: " << db_path << " ---\n";
        BillInserter inserter(db_path);
        inserter.insert_bill(bill_data);
        std::cout << "数据成功插入数据库。\n";

    } catch (const std::exception& e) {
        std::cerr << "处理过程中发生错误: " << e.what() << std::endl;
        return false;
    }

    std::cout << "--- 所有操作成功完成 ---\n";
    return true;
}