#include "MonthlyQuery.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm> // for std::sort

MonthlyQuery::MonthlyQuery(sqlite3* db_connection) : m_db(db_connection) {}

void MonthlyQuery::display(const std::string& month) {
    // 步骤 1 & 2: 从数据库获取数据并在内存中聚合 
    const char* sql = "SELECT parent_category, sub_category, amount, description, remark FROM transactions WHERE bill_date = ?;"; 
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) { 
        throw std::runtime_error("准备月度查询 SQL 语句失败: " + std::string(sqlite3_errmsg(m_db))); 
    }

    sqlite3_bind_text(stmt, 1, month.c_str(), -1, SQLITE_STATIC); 

    std::map<std::string, ParentCategoryData> aggregated_data; 
    std::string remark_text; 
    double grand_total = 0.0; 
    bool first_row = true; 

    while (sqlite3_step(stmt) == SQLITE_ROW) { 
        std::string parent_cat = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)); 
        std::string sub_cat = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)); 
        double amount = sqlite3_column_double(stmt, 2); 
        const unsigned char* desc_raw = sqlite3_column_text(stmt, 3); 
        
        Transaction t; 
        t.parent_category = parent_cat; 
        t.sub_category = sub_cat; 
        t.amount = amount; 
        t.description = desc_raw ? reinterpret_cast<const char*>(desc_raw) : ""; 
        
        aggregated_data[parent_cat].parent_total += amount; 
        aggregated_data[parent_cat].sub_categories[sub_cat].sub_total += amount; 
        aggregated_data[parent_cat].sub_categories[sub_cat].transactions.push_back(t); 

        grand_total += amount; 

        if (first_row) { 
            const unsigned char* remark_raw = sqlite3_column_text(stmt, 4); 
            remark_text = remark_raw ? reinterpret_cast<const char*>(remark_raw) : ""; 
            first_row = false; 
        }
    }
    sqlite3_finalize(stmt); 

    if (aggregated_data.empty()) { 
        std::cout << "\n未找到 " << month << " 月的任何数据。\n"; 
        return; 
    }

    // 步骤 3: 排序 
    for (auto& parent_pair : aggregated_data) { 
        for (auto& sub_pair : parent_pair.second.sub_categories) { 
            std::sort(sub_pair.second.transactions.begin(), sub_pair.second.transactions.end(),  
                [](const Transaction& a, const Transaction& b) {
                    return a.amount > b.amount; 
                });
        }
    }

    std::vector<std::pair<std::string, ParentCategoryData>> sorted_parents; 
    for (const auto& pair : aggregated_data) { 
        sorted_parents.push_back(pair); 
    }
    std::sort(sorted_parents.begin(), sorted_parents.end(),  
        [](const auto& a, const auto& b) {
            return a.second.parent_total > b.second.parent_total; 
        });

    // --- 步骤 4: 按最终格式打印 (此部分逻辑已重构) ---
    std::cout << std::fixed << std::setprecision(2); 
    std::cout << "\n# DATE:" << month << std::endl; 
    std::cout << "# TOTAL:¥" << grand_total << std::endl; 
    std::cout << "# REMARK:" << remark_text << std::endl; 

    // 遍历排序后的父类别
    for (const auto& parent_pair : sorted_parents) { 
        const std::string& parent_name = parent_pair.first; 
        const ParentCategoryData& parent_data = parent_pair.second; 
        
        // 打印父类别标题
        std::cout << "\n# " << parent_name << std::endl; 
        
        // 计算父类别占总额的百分比
        double parent_percentage = (grand_total > 0) ? (parent_data.parent_total / grand_total) * 100.0 : 0.0;
        
        // 打印父类别统计数据
        std::cout << "总计：¥" << parent_data.parent_total << std::endl;
        std::cout << "占比：" << parent_percentage << "%" << std::endl;

        // 遍历其下的子类别
        for (const auto& sub_pair : parent_data.sub_categories) { 
            const std::string& sub_name = sub_pair.first; 
            const SubCategoryData& sub_data = sub_pair.second; 
            
            // 打印子类别标题
            std::cout << "\n## " << sub_name << std::endl;
            
            // 计算子类别占父类别总额的百分比
            double sub_percentage = (parent_data.parent_total > 0) ? (sub_data.sub_total / parent_data.parent_total) * 100.0 : 0.0;
            
            // 打印子类别统计数据
            std::cout << "小计：¥" << sub_data.sub_total << "（占比：" << sub_percentage << "%）" << std::endl;
            
            // 遍历子类别下排好序的交易
            for (const auto& t : sub_data.transactions) { 
                // 在交易金额前加上人民币符号
                std::cout << "- ¥" << t.amount << " " << t.description << std::endl;
            }
        }
    }
}