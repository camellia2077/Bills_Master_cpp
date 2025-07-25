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

void print_menu() {
    std::println("--- {}Menu{} ---",GREEN_COLOR,RESET_COLOR);
    std::cout << "1. Validate Bill File(s)\n"; 
    std::cout << "2. Modify Bill File(s)\n"; 
    std::cout << "3. Parse and Insert Bill(s) to Database\n"; 
    std::cout << "4. Query Yearly Summary and Export (md/tex/typ)\n"; 
    std::cout << "5. Query Monthly Details and Export (md/tex/typ)\n"; 
    std::cout << "6. Auto-Process Full Workflow (File or Directory)\n";
    std::cout << "7. Export All Reports from Database (md, tex, typ, or all)\n"; 
    std::cout << "8. Version\n";
    std::cout << "9. Exit\n";

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

    AppController controller; // Create a single controller instance

    int choice = 0;
    while (choice != 9) { 
        print_menu();

        std::cout << "\nEnter your choice: ";

        std::cin >> choice;

        if (std::cin.fail()) { 
            std::cin.clear(); 
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
            std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Invalid input. Please enter a number." << "\n"; 
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
                    if (!input_str.empty()) controller.handle_validation(input_str);
                    break; 
                case 2:
                    std::cout << "Enter path for modification: ";
                    std::getline(std::cin, input_str);
                    if (!input_str.empty()) controller.handle_modification(input_str);
                    break; 
                case 3:
                    std::cout << "Enter path to import to database: ";
                    std::getline(std::cin, input_str);
                    if (!input_str.empty()) controller.handle_import(input_str);
                    break; 
                case 4:
                    { 
                        std::cout << "Enter year to query (e.g., 2025): ";
                        std::getline(std::cin, input_str);
                        if (input_str.empty()) break;

                        std::cout << "Enter format (md/tex/typ) [default: md]: ";
                        std::string format_input;
                        std::getline(std::cin, format_input);

                        std::string format_str = trim(format_input);
                        std::string format_to_use = "md";
                        if (format_str == "tex" || format_str == "typ") {
                            format_to_use = format_str;
                        }
                        controller.handle_export("year", input_str, format_to_use);
                    }
                    break;
                case 5:
                    {
                        std::cout << "Enter month to query (e.g., 202507): ";
                        std::getline(std::cin, input_str);
                        if (input_str.empty()) break;

                        std::cout << "Enter format (md/tex/typ) [default: md]: ";
                        std::string format_input;
                        std::getline(std::cin, format_input);
                        
                        std::string format_str = trim(format_input);
                        std::string format_to_use = "md";
                        if (format_str == "tex" || format_str == "typ") {
                            format_to_use = format_str;
                        }
                        controller.handle_export("month", input_str, format_to_use);
                    }
                    break;
                case 6:
                    std::cout << "Enter path for the full workflow: ";
                    std::getline(std::cin, input_str);
                    if (!input_str.empty()) controller.handle_full_workflow(input_str);
                    break;
                case 7:
                    {
                        std::cout << "Enter format(s) (e.g., md, tex, typ) [default: all]: ";
                        std::string format_input;
                        std::getline(std::cin, format_input);

                        std::set<std::string> formats_to_export;
                        if (format_input.empty()) {
                            formats_to_export.insert("md");
                            formats_to_export.insert("tex");
                            formats_to_export.insert("typ");
                            formats_to_export.insert("rst");
                        } else {
                            std::stringstream ss(format_input);
                            std::string segment;
                            while(std::getline(ss, segment, ',')) {
                                std::string format = trim(segment);
                                if (format == "md" || format == "tex" || format == "typ") {
                                    formats_to_export.insert(format);
                                }
                            }
                        }

                        if (formats_to_export.empty()){
                             std::cout << "No valid formats entered. Defaulting to all formats.\n";
                             formats_to_export.insert("md");
                             formats_to_export.insert("tex");
                             formats_to_export.insert("typ");
                             formats_to_export.insert("rst");
                        }

                        for (const auto& format : formats_to_export) {
                            controller.handle_export("all", "", format);
                        }
                    }
                    break;
                case 8: 
                    controller.display_version();
                    break;
                case 9: 
                    std::cout << "Exiting program. Goodbye!\n";
                    break;
                default:
                    std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Invalid choice. Please select a number from the menu." << "\n"; 
                    break;
            }
        } catch (const std::exception& e) {
            std::cerr << "\n" << RED_COLOR << "An unexpected error occurred: " << RESET_COLOR << e.what() << std::endl;
        }
    }
    return 0; 
}