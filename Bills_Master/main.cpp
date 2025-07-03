#include <iostream>
#include <string>
#include <limits>

#include "AppController.h"
#include "common_utils.h"

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
    std::cout << "\n===== Bill Reprocessor Menu =====\n"; 
    std::cout << "1. Validate Bill File(s)\n"; 
    std::cout << "2. Modify Bill File(s)\n"; 
    std::cout << "3. Parse and Insert Bill(s) to Database\n"; 
    std::cout << "4. Query Yearly Summary and Export\n";
    std::cout << "5. Query Monthly Details and Export\n";
    std::cout << "6. Auto-Process Full Workflow (File or Directory)\n";
    std::cout << "7. Version\n";
    std::cout << "8. Exit\n";
    std::cout << "=================================\n"; 
    std::cout << "Enter your choice: "; 
}

int main() {
    setup_console(); 
    std::cout << "Welcome to the Bill Reprocessor! (UTF-8 enabled)\n"; 

    AppController controller; // Create a single controller instance

    int choice = 0;
    while (choice != 8) { 
        print_menu();
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
                    std::cout << "Enter year to query (e.g., 2025): ";
                    std::getline(std::cin, input_str);
                    if (!input_str.empty()) controller.handle_yearly_query(input_str);
                    break;
                case 5:
                    std::cout << "Enter month to query (e.g., 202506): ";
                    std::getline(std::cin, input_str);
                     if (!input_str.empty()) controller.handle_monthly_query(input_str);
                    break;
                case 6:
                    std::cout << "Enter path for the full workflow: ";
                    std::getline(std::cin, input_str);
                    if (!input_str.empty()) controller.handle_full_workflow(input_str);
                    break;
                case 7:
                    controller.display_version();
                    break;
                case 8:
                    std::cout << "Exiting program. Goodbye!\n";
                    break;
                default:
                    std::cerr << YELLOW_COLOR << "Warning: " << RESET_COLOR << "Invalid choice. Please select a number from the menu." << "\n"; 
                    break;
            }
        } catch (const std::exception& e) {
            // This catch block handles exceptions from the controller
            std::cerr << "\n" << RED_COLOR << "An unexpected error occurred: " << RESET_COLOR << e.what() << std::endl;
        }
    }
    return 0; 
}
