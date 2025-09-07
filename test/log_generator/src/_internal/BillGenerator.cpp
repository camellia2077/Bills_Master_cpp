// BillGenerator.cpp

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
        
        // --- 核心修改：加载扁平化的配置 ---
        // 假设顶层现在是一个扁平的数组
        if (raw_config.is_array()) {
            config_ = raw_config;
        } 
        // 兼容旧格式，或者当配置包含其他选项时
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

    // 1. 将扁平化的子分类按 parent_category 分组
    std::map<std::string, std::vector<json>> grouped_by_parent;
    for (const auto& sub_item : config_) {
        grouped_by_parent[sub_item["parent_category"]].push_back(sub_item);
    }

    // 2. 遍历分组后的 map 来生成文件，以确保正确的层级结构
    for (auto it = grouped_by_parent.begin(); it != grouped_by_parent.end(); ++it) {
        const std::string& parent_name = it->first;
        const std::vector<json>& sub_items = it->second;

        outfile << parent_name << std::endl; // 写入父分类

        for (const auto& sub_config : sub_items) {
            outfile << std::endl;
            outfile << sub_config["sub_category"].get<std::string>() << std::endl; // 写入子分类

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
                outfile << std::fixed << std::setprecision(2) << cost << " " << item["description"].get<std::string>();

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
    // ===================================================================
    
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