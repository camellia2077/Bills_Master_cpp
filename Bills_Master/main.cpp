#include <iostream>
#include <string>
#include <limits>
#include <vector>
#include <set>
#include <sstream>
#include <algorithm>
#include <print>

#include "app_controller/AppController.h"
#include "common/common_utils.h" // for colors

// For UTF-8 output on Windows
#ifdef _WIN32
#include <windows.h>
#endif

// Sets up the console for proper UTF-8 character display.
void setup_console() { 
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

// --- ** 修改：更新菜单选项 ** ---
void print_menu() {
    std::println("--- {}Menu{} ---",GREEN_COLOR,RESET_COLOR);
    std::cout << "1. Validate Bill File(s)\n"; 
    std::cout << "2. Modify Bill File(s)\n"; 
    std::cout << "3. Parse and Insert Bill(s) to Database\n"; 
    std::cout << "4. Query & Export Yearly Report\n"; 
    std::cout << "5. Query & Export Monthly Report\n"; 
    std::cout << "6. Auto-Process Full Workflow (File or Directory)\n";
    std::cout << "7. Export All Reports from Database\n"; 
    std::cout << "8. Export by Date or Date Range\n"; // 新增选项
    std::cout << "9. Version\n";                     // 原选项 8
    std::cout << "10. Exit\n";                       // 原选项 9

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
    std::println("Welcome to the bill_master\n"); 

    AppController controller; 

    int choice = 0;
    // --- ** 修改：更新退出条件 ** ---
    while (choice != 10) { 
        print_menu();
        std::cout << "\nEnter your choice: ";
        std::cin >> choice;

        if (std::cin.fail()) { 
            std::cin.clear(); 
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
            std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Invalid input. Please enter a number." << "\n\n"; 
            choice = 0; 
            continue; 
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 

        std::string input_str; 

        try {
            switch (choice) {
                case 1:
                    std::cout << "Enter path for validation: ";
                    std::getline(std::cin, input_str);
                    if (!input_str.empty()) {
                        if (!controller.handle_validation(input_str)) {
                            std::println(RED_COLOR, "\nValidation failed. Please check the output above.", RESET_COLOR);
                        }
                    }
                    break; 
                case 2:
                    std::cout << "Enter path for modification: ";
                    std::getline(std::cin, input_str);
                    if (!input_str.empty()) {
                        if (!controller.handle_modification(input_str)) {
                            std::println(RED_COLOR, "\nModification failed. Please check the output above.", RESET_COLOR);
                        }
                    }
                    break; 
                case 3:
                    std::cout << "Enter path to import to database: ";
                    std::getline(std::cin, input_str);
                    if (!input_str.empty()) {
                        if (!controller.handle_import(input_str)) {
                            std::println(RED_COLOR, "\nImport failed. Please check the output above.", RESET_COLOR);
                        }
                    }
                    break; 
                case 4:
                    { 
                        std::cout << "Enter year to query (e.g., 2025): ";
                        std::getline(std::cin, input_str);
                        if (input_str.empty()) break;

                        std::cout << "Enter format (md/tex/typ/rst) [default: md]: ";
                        std::string format_input;
                        std::getline(std::cin, format_input);
                        std::string format_str = trim(format_input);
                        if (format_str.empty()) format_str = "md";
                        
                        // --- ** 修改：使用新的 vector 接口 ** ---
                        if (!controller.handle_export("year", {input_str}, format_str)) {
                            std::println(RED_COLOR, "\nYearly report export failed.", RESET_COLOR);
                        }
                    }
                    break;
                case 5:
                    {
                        std::cout << "Enter month to query (YYYYMM): ";
                        std::getline(std::cin, input_str);
                        if (input_str.empty()) break;

                        std::cout << "Enter format (md/tex/typ/rst) [default: md]: ";
                        std::string format_input;
                        std::getline(std::cin, format_input);
                        std::string format_str = trim(format_input);
                        if (format_str.empty()) format_str = "md";

                        // --- ** 修改：使用新的 vector 接口 ** ---
                        if (!controller.handle_export("month", {input_str}, format_str)) {
                             std::println(RED_COLOR, "\nMonthly report export failed.", RESET_COLOR);
                        }
                    }
                    break;
                case 6:
                    std::cout << "Enter path for the full workflow: ";
                    std::getline(std::cin, input_str);
                    if (!input_str.empty()) {
                        if (!controller.handle_full_workflow(input_str)) {
                            std::println(RED_COLOR, "\nFull workflow failed.", RESET_COLOR);
                        }
                    }
                    break;
                case 7:
                    {
                        std::cout << "Enter format(s) separated by commas (md, tex, etc.) [default: all]: ";
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
                            std::println(GREEN_COLOR, "\nAll specified formats exported successfully.", RESET_COLOR);
                        } else {
                            std::println(RED_COLOR, "\nOne or more export operations failed.", RESET_COLOR);
                        }
                    }
                    break;
                // --- ** 新增：处理日期区间导出的 case ** ---
                case 8:
                    {
                        std::cout << "Enter date (YYYY or YYYYMM) or date range (YYYYMM YYYYMM): ";
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
                            std::cerr << "No date provided.\n";
                            break;
                        }

                        std::cout << "Enter format (md/tex/typ/rst) [default: md]: ";
                        std::string format_input;
                        std::getline(std::cin, format_input);
                        std::string format_str = trim(format_input);
                        if (format_str.empty()) format_str = "md";
                        
                        // 调用统一的 handle_export 接口
                        if (!controller.handle_export("date", values, format_str)) {
                            std::println(RED_COLOR, "\nDate-based export failed. Please check logs.", RESET_COLOR);
                        }
                    }
                    break;
                // --- ** 修改：更新后续选项的编号 ** ---
                case 9: 
                    controller.display_version();
                    break;
                case 10: 
                    std::cout << "Exiting program. Goodbye!\n";
                    break;
                default:
                    std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Invalid choice. Please select from the menu." << "\n"; 
                    break;
            }
        } catch (const std::exception& e) {
            std::cerr << "\n" << RED_COLOR << "An unexpected error occurred: " << RESET_COLOR << e.what() << std::endl;
        }
        std::cout << "\n"; 
    }
    return 0; 
}
