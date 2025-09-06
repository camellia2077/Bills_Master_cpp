// BillGenerator.h

#ifndef BILL_GENERATOR_H
#define BILL_GENERATOR_H

#include <filesystem>
#include <random>
#include <vector> // <-- 为注释列表添加
#include <string> // <-- 为注释列表添加
#include <nlohmann/json.hpp>

class BillGenerator {
public:
    BillGenerator();
    bool load_config(const std::string& config_path);
    void generate_bill_file(int year, int month, const std::filesystem::path& dir_path) const;

private:
    double random_double(double min, double max) const;
    int random_int(int min, int max) const;

    nlohmann::json config_;
    mutable std::mt19937 random_engine_; 

    // --- 新增成员变量 ---
    double comment_probability_ = 0.0; // 存储添加注释的概率
    std::vector<std::string> comments_;  // 存储所有可能的注释
};

#endif // BILL_GENERATOR_H