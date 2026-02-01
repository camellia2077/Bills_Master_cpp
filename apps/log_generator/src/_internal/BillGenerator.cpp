// src/_internal/BillGenerator.cpp

#include "BillGenerator.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>

using json = nlohmann::json;

BillGenerator::BillGenerator() : random_engine_(std::random_device{}()) {} // Use a random seed

bool BillGenerator::load_config(const std::string& config_path) {
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        std::cerr << "Error: Could not find or open configuration file '" << config_path << "'." << std::endl;
        return false;
    }
    try {
        json raw_config = json::parse(config_file);
        
        if (raw_config.is_array()) {
            config_ = raw_config;
        } 
        else if (raw_config.contains("categories") && raw_config["categories"].is_array()) {
            config_ = raw_config["categories"];
        } else {
             std::cerr << "Error: Config root must be an array of sub-category objects." << std::endl;
             return false;
        }

        if (raw_config.contains("comment_options")) {
            const auto& comment_opts = raw_config["comment_options"];
            comment_probability_ = comment_opts.value("probability", 0.3);
            if (comment_opts.contains("comments") && comment_opts["comments"].is_array()) {
                comments_ = comment_opts["comments"].get<std::vector<std::string>>();
            }
        }

    } catch (json::parse_error& e) {
        std::cerr << "Error: Failed to parse " << config_path << ".\n" << e.what() << std::endl;
        return false;
    }
    config_file.close();
    return true;
}

void BillGenerator::generate_bill_file(int year, int month, const std::filesystem::path& dir_path) const {
    std::stringstream ss_year_month;
    ss_year_month << year << std::setw(2) << std::setfill('0') << month;
    std::string year_month = ss_year_month.str();
    std::filesystem::path file_path = dir_path / (year_month + ".txt");

    std::ofstream outfile(file_path);
    if (!outfile) {
        std::cerr << "Cannot generate file: " << file_path << std::endl;
        return;
    }
    // 标题行的date和remark
    outfile << "date:" << year_month << std::endl;
    outfile << "remark:" << std::endl << std::endl;

    std::map<std::string, std::vector<json>> grouped_by_parent;
    for (const auto& sub_item : config_) {
        grouped_by_parent[sub_item["parent_category"]].push_back(sub_item);
    }

    for (auto it = grouped_by_parent.begin(); it != grouped_by_parent.end(); ++it) {
        const std::string& parent_name = it->first;
        const std::vector<json>& sub_items = it->second;

        outfile << parent_name << std::endl;

        for (const auto& sub_config : sub_items) {
            outfile << std::endl;
            outfile << sub_config["sub_category"].get<std::string>() << std::endl;

            std::vector<json> details_vector;
            if (sub_config.contains("details")) {
                for (const auto& detail_item : sub_config["details"]) {
                    details_vector.push_back(detail_item);
                }
            }
            if (details_vector.empty()) continue;

            int num_to_gen = random_int(1, details_vector.size());
            std::shuffle(details_vector.begin(), details_vector.end(), random_engine_);

            for (int i = 0; i < num_to_gen; ++i) {
                const auto& item = details_vector[i];
                double cost = random_double(item["min_cost"], item["max_cost"]);
                
                // 1. 生成一个-10到+10之间的随机调整值
                double adjustment = random_double(-10.0, 10.0);

                // --- 修改开始：符号处理与乘法模拟 ---

                // 2. 符号处理
                // 只有 income 分类强制使用 '+'
                // 其他分类不输出符号（即默认为正数形式，依靠 Parser 的规则自动识别为支出）
                if (parent_name == "income") {
                    outfile << "+";
                }

                // 3. 乘法模拟
                // 30% 的概率将 cost 拆分为 "单价 * 数量" 的形式
                bool use_multiplication = (random_double(0.0, 1.0) < 0.3);

                if (use_multiplication) {
                    int quantity = random_int(2, 6); // 随机数量 2 到 6
                    double unit_price = cost / quantity;

                    // 输出单价
                    outfile << std::fixed << std::setprecision(2) << unit_price;

                    // 随机选择乘法符号 * 或 ×
                    if (random_int(0, 1) == 0) {
                        outfile << "*";
                    } else {
                        outfile << "×";
                    }

                    // 输出数量
                    outfile << quantity;
                } else {
                    // 不使用乘法，直接输出总价
                    outfile << std::fixed << std::setprecision(2) << cost;
                }

                // 4. 输出带符号的随机调整值 (保持不变，用于模拟 +0.07 或 -1.5 这种修饰)
                outfile << std::showpos << std::fixed << std::setprecision(2) << adjustment;
                outfile << std::noshowpos; // 恢复默认行为

                // --- 修改结束 ---

                // 5. 输出描述
                outfile << " " << item["description"].get<std::string>();

                std::uniform_real_distribution<> prob_dist(0.0, 1.0);
                if (!comments_.empty() && prob_dist(random_engine_) < comment_probability_) {
                    int comment_index = random_int(0, comments_.size() - 1);
                    outfile << " // " << comments_[comment_index];
                }

                outfile << std::endl;
            }
        }

        if (std::next(it) != grouped_by_parent.end()) {
            outfile << std::endl;
        }
    }
    
    outfile.close();
    std::cout << "Successfully generated bill file: " << file_path << std::endl;
}

double BillGenerator::random_double(double min, double max) const {
    std::uniform_real_distribution<> dist(min, max);
    return dist(random_engine_);
}

int BillGenerator::random_int(int min, int max) const {
    std::uniform_int_distribution<> dist(min, max);
    return dist(random_engine_);
}