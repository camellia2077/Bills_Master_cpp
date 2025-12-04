// common/structures/CommonData.hpp

#ifndef COMMON_DATA_HPP
#define COMMON_DATA_HPP

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
    std::string transaction_type; // 用于表示 "Income" 或 "Expense"
};

// 代表整个解析后的账单文件
struct ParsedBill {
    std::string date;
    std::string remark;
    int year;
    int month;
    std::vector<Transaction> transactions;

    // --- 【核心修改】 ---
    // 移除了 double total_amount; 
    // 添加了三个新的总计字段
    double total_income;
    double total_expense;
    double balance;
    // --- 修改结束 ---
};

#endif // COMMON_DATA_HPP