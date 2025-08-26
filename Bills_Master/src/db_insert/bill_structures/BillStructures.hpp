#ifndef BILL_STRUCTURES_H
#define BILL_STRUCTURES_H
#include <string>
#include <vector>
// 代表一笔具体的交易记录
struct Transaction {
std::string parent_category;
std::string sub_category;
double amount;
std::string description;
};
// 代表整个解析后的账单文件
struct ParsedBill {
std::string date; // 保留原始的 YYYYMM 字符串
int year = 0; // 新增：年份
int month = 0; // 新增：月份
std::string remark;
std::vector<Transaction> transactions;
};
#endif // BILL_STRUCTURES_H