
#include <string>
#include <limits>
#include <vector>
#include <set>
#include <sstream>
#include <algorithm>
#include <print>
#include <cstdio> // For stderr

#include "app_controller/AppController.hpp"
#include "common/common_utils.hpp" // for colors
// Add to the top of main.cpp
#ifdef _WIN32
#include <windows.h>
#endif

// Add this helper function before main()
void setup_console() { 
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}


// --- ** 修改：更新菜单选项 ** ---
void print_menu() {
    std::println("--- {}菜单Menu{} ---",GREEN_COLOR,RESET_COLOR);
    std::println("1. Validate Bill File(s)"); 
    std::println("2. Modify Bill File(s)"); 
    std::println("3. Parse and Insert Bill(s) to Database"); 
    std::println("4. Query & Export Yearly Report"); 
    std::println("5. Query & Export Monthly Report"); 
    std::println("6. Auto-Process Full Workflow (File or Directory)");
    std::println("7. Export All Reports from Database"); 
    std::println("8. Export by Date or Date Range"); 
    std::println("9. Version");                     
    std::println("10. Exit");                       

    std::string line(10,'-');
    std::println("{}", line);
}

// 辅助函数：去除字符串两端的空格
std::string trim(const std::string& str) {
    const std::string WHITESPACE = " \n\r\t\f\v";
    size_t first = str.find_first_not_of(WHITESPACE);
    if (std::string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(WHITESPACE);
    return str.substr(first, (last - first + 1));
}

int main() {
    setup_console();
    std::println("Welcome to the bill_master"); 
    std::println(""); // For the extra newline

    AppController controller; 

    int choice = 0;
    // --- ** 修改：更新退出条件 ** ---
    while (choice != 10) { 
        print_menu();
        std::print("\nEnter your choice: ");
        std::cin >> choice;

        if (std::cin.fail()) { 
            std::cin.clear(); 
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
            std::println(stderr, "{}Warning: {}Invalid input. Please enter a number.\n", YELLOW_COLOR, RESET_COLOR); 
            choice = 0; 
            continue; 
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 

        std::string input_str; 

        try {
            switch (choice) {
                case 1:
                    std::print("Enter path for validation: ");
                    std::getline(std::cin, input_str);
                    if (!input_str.empty()) {
                        if (!controller.handle_validation(input_str)) {
                            std::println("{}\nValidation failed. Please check the output above.{}", RED_COLOR, RESET_COLOR);
                        }
                    }
                    break; 
                case 2:
                    std::print("Enter path for modification: ");
                    std::getline(std::cin, input_str);
                    if (!input_str.empty()) {
                        if (!controller.handle_modification(input_str)) {
                            std::println("{}\nModification failed. Please check the output above.{}", RED_COLOR, RESET_COLOR);
                        }
                    }
                    break; 
                case 3:
                    std::print("Enter path to import to database: ");
                    std::getline(std::cin, input_str);
                    if (!input_str.empty()) {
                        if (!controller.handle_import(input_str)) {
                            std::println("{}\nImport failed. Please check the output above.{}", RED_COLOR, RESET_COLOR);
                        }
                    }
                    break; 
                case 4:
                    { 
                        std::print("Enter year to query (e.g., 2025): ");
                        std::getline(std::cin, input_str);
                        if (input_str.empty()) break;

                        std::print("Enter format (md/tex/typ/rst) [default: md]: ");
                        std::string format_input;
                        std::getline(std::cin, format_input);
                        std::string format_str = trim(format_input);
                        if (format_str.empty()) format_str = "md";
                        
                        // --- ** 修改：使用新的 vector 接口 ** ---
                        if (!controller.handle_export("year", {input_str}, format_str)) {
                            std::println("{}\nYearly report export failed.{}", RED_COLOR, RESET_COLOR);
                        }
                    }
                    break;
                case 5:
                    {
                        std::print("Enter month to query (YYYYMM): ");
                        std::getline(std::cin, input_str);
                        if (input_str.empty()) break;

                        std::print("Enter format (md/tex/typ/rst) [default: md]: ");
                        std::string format_input;
                        std::getline(std::cin, format_input);
                        std::string format_str = trim(format_input);
                        if (format_str.empty()) format_str = "md";

                        // --- ** 修改：使用新的 vector 接口 ** ---
                        if (!controller.handle_export("month", {input_str}, format_str)) {
                             std::println("{}\nMonthly report export failed.{}", RED_COLOR, RESET_COLOR);
                        }
                    }
                    break;
                case 6:
                    std::print("Enter path for the full workflow: ");
                    std::getline(std::cin, input_str);
                    if (!input_str.empty()) {
                        if (!controller.handle_full_workflow(input_str)) {
                            std::println("{}\nFull workflow failed.{}", RED_COLOR, RESET_COLOR);
                        }
                    }
                    break;
                case 7:
                    {
                        std::print("Enter format(s) separated by commas (md, tex, etc.) [default: all]: ");
                        std::string format_input;
                        std::getline(std::cin, format_input);

                        std::set<std::string> formats_to_export;
                        if (format_input.empty() || trim(format_input) == "all") {
                            formats_to_export = {"md", "tex", "typ", "rst"};
                        } else {
                            // ... (解析逗号分隔的格式) ...
                            std::stringstream ss(format_input);
                            std::string segment;
                            while(std::getline(ss, segment, ',')) {
                                std::string format = trim(segment);
                                if (!format.empty()) formats_to_export.insert(format);
                            }
                        }

                        bool all_succeeded = true;
                        for (const auto& format : formats_to_export) {
                             // --- ** 修改：使用新的 vector 接口 ** ---
                            if (!controller.handle_export("all", {}, format)) {
                                all_succeeded = false;
                            }
                        }

                        if (all_succeeded) {
                            std::println("{}\nAll specified formats exported successfully.{}", GREEN_COLOR, RESET_COLOR);
                        } else {
                            std::println("{}\nOne or more export operations failed.{}", RED_COLOR, RESET_COLOR);
                        }
                    }
                    break;
                // --- ** 新增：处理日期区间导出的 case ** ---
                case 8:
                    {
                        std::print("Enter date (YYYY or YYYYMM) or date range (YYYYMM YYYYMM): ");
                        std::string line_input;
                        std::getline(std::cin, line_input);
                        if (trim(line_input).empty()) break;

                        // 解析输入的一个或多个日期
                        std::stringstream ss(line_input);
                        std::string value;
                        std::vector<std::string> values;
                        while (ss >> value) {
                            values.push_back(value);
                        }

                        if (values.empty()) {
                            std::println(stderr, "No date provided.");
                            break;
                        }

                        std::print("Enter format (md/tex/typ/rst) [default: md]: ");
                        std::string format_input;
                        std::getline(std::cin, format_input);
                        std::string format_str = trim(format_input);
                        if (format_str.empty()) format_str = "md";
                        
                        // 调用统一的 handle_export 接口
                        if (!controller.handle_export("date", values, format_str)) {
                            std::println("{}\nDate-based export failed. Please check logs.{}", RED_COLOR, RESET_COLOR);
                        }
                    }
                    break;
                // --- ** 修改：更新后续选项的编号 ** ---
                case 9: 
                    controller.display_version();
                    break;
                case 10: 
                    std::println("Exiting program. Goodbye!");
                    break;
                default:
                    std::println(stderr, "{}Warning: {}Invalid choice. Please select from the menu.", YELLOW_COLOR, RESET_COLOR); 
                    break;
            }
        } catch (const std::exception& e) {
            std::println(stderr, "\n{}An unexpected error occurred: {}{}", RED_COLOR, RESET_COLOR, e.what());
        }
        std::println(""); // For the extra newline at the end of the loop
    }
    return 0; 
}