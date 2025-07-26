#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <iomanip>
#include <sstream>
#include <algorithm> // For std::shuffle
#include <filesystem> // C++17
#include <chrono>     // For high-resolution timing
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// --- 伪随机数生成部分 ---

/**
 * @brief 获取一个全局的伪随机数引擎。
 * * @note 【重要修改】根据您的要求，这里禁止使用 `std::random_device`（真随机源）。
 * 我们使用一个固定的整数 `0` 作为种子来初始化伪随机数引擎 `std::mt19937`。
 * 这确保了每次程序运行时，生成的随机数序列都是完全相同的，从而使输出结果具有确定性和可复现性。
 * 这是伪随机数的核心特性。
 * * @return std::mt19937& 返回伪随机数引擎的引用。
 */
std::mt19937& get_random_engine() {
    // 使用固定的种子`0`来创建一个可复现的伪随机序列。
    static std::mt19937 gen(0);
    return gen;
}

/**
 * @brief 使用全局伪随机数引擎生成一个在[min, max]范围内的双精度浮点数。
 * * @param min 随机数的最小值。
 * @param max 随机数的最大值。
 * @return double 生成的伪随机数。
 */
double random_double(double min, double max) {
    std::uniform_real_distribution<> dis(min, max);
    return dis(get_random_engine());
}

/**
 * @brief 使用全局伪随机数引擎生成一个在[min, max]范围内的整数。
 * @param min 随机数的最小值。
 * @param max 随机数的最大值。
 * @return int 生成的伪随机数。
 */
int random_int(int min, int max) {
    std::uniform_int_distribution<> dis(min, max);
    return dis(get_random_engine());
}


// --- 文件生成部分 ---

/**
 * @brief 根据配置为给定的年月生成一个账单文件。
 * * @param year 年份。
 * @param month 月份。
 * @param dir_path 文件要保存到的目录路径。
 * @param config JSON配置对象。
 */
void generate_bill_file(int year, int month, const std::filesystem::path& dir_path, const json& config) {
    // 1. 组合文件名，格式为YYYYMM.txt
    std::stringstream ss_year_month;
    ss_year_month << year << std::setw(2) << std::setfill('0') << month;
    std::string year_month = ss_year_month.str();
    std::filesystem::path file_path = dir_path / (year_month + ".txt");

    // 2. 创建并打开文件流
    std::ofstream outfile(file_path);
    if (!outfile) {
        std::cerr << "Can not generate file: " << file_path << std::endl;
        return;
    }

    // 3. 写入文件头信息
    outfile << "DATE:" << year_month << std::endl;
    outfile << "REMARK:" << std::endl;

    // 4. 遍历JSON配置，生成文件主体内容
    // 外层循环遍历每一个父级项目 (例如："MEAL吃饭")
    for (const auto& parent_config : config) {
        // 写入父级标题
        outfile << parent_config["name"].get<std::string>() << std::endl;

        // 内层循环遍历该父级下的所有子项
        for (const auto& sub_config : parent_config["sub_items"]) {
            // a. 写入子项目名称
            outfile << sub_config["name"].get<std::string>() << std::endl;

            // b. 获取所有 details 条目
            std::vector<json> details_vector;
            for (const auto& detail_item : sub_config["details"]) {
                details_vector.push_back(detail_item);
            }

            // 确保至少有一个内容被生成
            if (details_vector.empty()) {
                continue; // 如果没有 details，跳过这个子项目
            }

            // 随机选择要生成的 detail 数量，至少为1，最多为所有 available details 的数量
            int num_details_to_generate = random_int(1, details_vector.size());

            // 随机打乱 details 向量，然后选取前 num_details_to_generate 个
            std::shuffle(details_vector.begin(), details_vector.end(), get_random_engine());

            // c. 遍历选中的 details 条目
            for (int i = 0; i < num_details_to_generate; ++i) {
                const auto& detail_item = details_vector[i];

                double min_c = detail_item["min_cost"];
                double max_c = detail_item["max_cost"];

                double cost = random_double(min_c, max_c);

                // d. 格式化花费和描述，拼接成最终内容字符串
                std::stringstream ss_content;
                ss_content << std::fixed << std::setprecision(2) << cost << detail_item["description"].get<std::string>();

                // e. 写入子项目内容
                outfile << ss_content.str() << std::endl;
            }
        }
    }

    // 5. 关闭文件
    outfile.close();
}


// --- 帮助和版本信息函数 ---

/**
 * @brief 显示程序的版本号和最后更新时间。
 */
void show_version(const std::string& version, const std::string& last_update) {
    std::cout << "generator version " << version << std::endl;
    std::cout << "Last updated: " << last_update << std::endl;
}

/**
 * @brief 显示程序的用法帮助信息。
 * @param app_name 程序的可执行文件名 (argv[0])。
 */
void show_help(const char* app_name) {
    // 使用 std::cerr 来输出帮助信息，这是一种常见的做法
    std::cerr << "Usage: " << app_name << " [options] [year_duration]\n\n"
              << "A pseudo-random bill file generator based on a JSON configuration.\n\n"
              << "Options:\n"
              << "  [year_duration]    (Optional) The number of years to generate bills for.\n"
              << "                     Must be a positive integer. Defaults to 100.\n"
              << "  -h, --help         Show this help message and exit.\n"
              << "  --version          Show version information and exit.\n\n"
              << "Example:\n"
              << "  " << app_name << " 50         # Generates bills for 50 years.\n"
              << "  " << app_name << " --help     # Shows this message.\n";
}


// --- 主函数 ---

int main(int argc, char* argv[]) {
    // 版本和更新信息
    const std::string APP_VERSION = "1.1.0";
    const std::string LAST_UPDATE = "2025-06-24";

    int year_duration = 100; // Default duration

    // 步骤0: 处理命令行参数 (--help, --version)
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "-h" || arg1 == "--help") {
            show_help(argv[0]);
            return 0;
        }
        if (arg1 == "--version") {
            show_version(APP_VERSION, LAST_UPDATE);
            return 0;
        }

        // 如果不是帮助或版本标志，则尝试将其解析为年份
        try {
            year_duration = std::stoi(arg1);
            if (year_duration <= 0) {
                std::cerr << "Warning: Year duration must be a positive integer. Using default 100 years." << std::endl;
                year_duration = 100;
            }
        } catch (const std::invalid_argument& e) {
            std::cerr << "Warning: Invalid argument '" << arg1 << "'. It is not a valid flag or number. Using default 100 years." << std::endl;
            // Optionally, show help and exit here if the argument is invalid
            // show_help(argv[0]);
            // return 1;
        } catch (const std::out_of_range& e) {
            std::cerr << "Warning: Year duration out of range. Using default 100 years. Error: " << e.what() << std::endl;
        }
    }


    // 步骤1: 读取并解析 `config.json` 配置文件
    const std::string config_filename = "config.json";
    std::ifstream config_file(config_filename);
    if (!config_file.is_open()) {
        std::cerr << "Error: Could not find or open the configuration file '" << config_filename << "'。" << std::endl;
        std::cerr << "Please ensure " << config_filename << " is in the same directory as the executable." << std::endl;
        return 1;
    }

    json config;
    try {
        config = json::parse(config_file);
    } catch (json::parse_error& e) {
        std::cerr << "Error: Failed to parse " << config_filename << ".\n" << e.what() << std::endl;
        return 1;
    }
    config_file.close();

    int start_year = 1900;
    int end_year = start_year + year_duration - 1;


    // 步骤2: 创建主输出目录 `bills_output_from_config`
    std::string output_dir_name = "bills_output_from_config";
    std::filesystem::path base_output_dir(output_dir_name);

    try {
        if (!std::filesystem::exists(base_output_dir)) {
            std::filesystem::create_directory(base_output_dir);
            std::cout << "Created base directory: " << output_dir_name << std::endl;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to create base directory: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Configuration loaded successfully, starting to generate bill files for " << year_duration << " year(s)..." << std::endl;

    // Start high-resolution timer
    auto start_time = std::chrono::high_resolution_clock::now();

    // 步骤3: 循环生成所有文件
    for (int year = start_year; year <= end_year; ++year) {
        // 为当前年份创建子目录
        std::filesystem::path year_dir = base_output_dir / std::to_string(year);
        try {
            if (!std::filesystem::exists(year_dir)) {
                std::filesystem::create_directory(year_dir);
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Failed to create directory for year " << year << ": " << e.what() << std::endl;
            continue; // 如果创建失败，跳过这一年
        }

        // 为当前年份生成12个月的账单文件
        for (int month = 1; month <= 12; ++month) {
            generate_bill_file(year, month, year_dir, config);
        }
        // std::cout << "Finished generating files for year: " << year << std::endl; // 可以取消注释来查看每年的生成进度
    }

    // End high-resolution timer
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    double total_seconds = duration.count() / 1000.0;

    // 步骤4: 打印最终总结信息和时间统计
    int total_files_generated = year_duration * 12;
    std::cout << "\n----------------------------------------" << std::endl;
    std::cout << "Successfully generate files: " << total_files_generated << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Timing Statistics:" << std::endl;
    std::cout << "Total time: " << std::fixed << std::setprecision(4) << total_seconds << " seconds (" << duration.count() << " ms)" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    return 0;
}