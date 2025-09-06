// src/common_structures/CommonData.hpp

#ifndef COMMON_DATA_H
#define COMMON_DATA_H

#include <string>
#include <vector>

// 代表一笔具体的交易记录
struct Transaction {
    std::string parent_category;
    std::string sub_category;
    double amount; // 金额
    std::string description; // 描述物品名称
    std::string source; // source 字段,用于表示来源于自动添加还是手动添加
    std::string comment; // comment 字段,用于表示备注内容
};

// 代表整个解析后的账单文件
struct ParsedBill {
    std::string date; 
    int year = 0;
    int month = 0;
    std::string remark;
    std::vector<Transaction> transactions;
};

#endif // COMMON_DATA_H