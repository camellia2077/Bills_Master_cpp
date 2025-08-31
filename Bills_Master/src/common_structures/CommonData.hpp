// common_structures/CommonData.hpp

#ifndef COMMON_DATA_H
#define COMMON_DATA_H

#include <string>
#include <vector>
// 账单的具体结构

// 代表一笔具体的交易记录
// 这个结构体现在被 query 和 db_insert 模块共享
struct Transaction {
    std::string parent_category;
    std::string sub_category;
    double amount;
    std::string description;
};

// 代表整个解析后的账单文件
// 这个结构体主要由 db_insert 模块使用
struct ParsedBill {
    std::string date; 
    int year = 0;
    int month = 0;
    std::string remark;
    std::vector<Transaction> transactions;
};

#endif // COMMON_DATA_H