#include <iostream>
#include <string>
#include <vector>
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

void print_help(const char* program_name) {

    std::string command_color = CYAN_COLOR,tile_color = GREEN_COLOR;

    std::println("Bill Master - A command-line tool for processing bill files.\n");
    std::println("Usage: {} <command> [arguments] [--format <md|tex|typ>]\n", program_name);
    
    // --- Full Workflow ---
    std::cout << GREEN_COLOR << "--- Full Workflow ---\n" << RESET_COLOR;
    
    std::println("{}--all process, -a p {}{}",command_color,RESET_COLOR,"<path>");
    std::println("  Runs the processing workflow (validate, modify, import).");
    std::println("  Example: {} -a p ./my_bills/\n", program_name);

    std::println("--all export, -a e <path>");
    std::println("  Runs the entire workflow (validate, modify, import, export).");
    std::println("  Example: {} -a e ./my_bills/ --format md\n", program_name);

    // --- Reprocessor ---
    std::cout << GREEN_COLOR << "--- Reprocessor ---\n" << RESET_COLOR;
    
    std::println("--validate, -v <path>");
    std::println("  Validates a .txt bill file or all .txt files in a directory.");
    std::println("  Example: {} -v ./bills/2024-01.txt\n", program_name);

    std::println("--modify, -m <path>");
    std::println("  Modifies a .txt file or all .txt files in a directory.");
    std::println("  Example: {} -m ./bills_to_process/\n", program_name);

    // --- DB Insertor ---
    std::cout << GREEN_COLOR << "--- DB Insertor ---\n" << RESET_COLOR;
    
    std::println("--import, -i <path>");
    std::println("  Parses and inserts a .txt file or a directory into the database.");
    std::println("  Example: {} -i ./modified_bills/2024-02.txt\n", program_name);
    
    // --- Query & Export ---
    std::cout << GREEN_COLOR << "--- Query & Export ---\n" << RESET_COLOR;

    std::println("--query year, -q y <year>");
    std::println("  Queries and exports the annual summary.");
    std::println("  Example: {} -q y 2024 --format tex\n", program_name);

    std::println("--query month, -q m <month>");
    std::println("  Queries and exports the monthly details (format: YYYY-MM).");
    std::println("  Example: {} -q m 2024-07\n", program_name);

    std::println("--export all, -e a");
    std::println("  Exports all available reports from the database.");
    std::println("  Use optional --type <month|year> to export only one type of report.");
    std::println("  Example: {} -e a --type month --format tex\n", program_name);
    
    std::println("--export date, -e d <date>");
    std::println("  Exports reports based on a date string.");
    std::println("  <date> format: YYYY (exports all months in that year), YYYYMM (exports a single month).");
    std::println("  Example: {} -e d 2024 -f tex\n", program_name);

    // --- Options ---
    std::cout << GREEN_COLOR << "--- Options ---\n" << RESET_COLOR;
    
    std::println("  --format, -f <format>");
    std::println("    Specify output format ('md', 'tex', 'typ' , 'rst'). Defaults to 'md'.");
    std::println("    Example: --format md\n");

    // --- General ---
    std::cout << GREEN_COLOR << "--- General ---\n" << RESET_COLOR;
    
    std::println("  -h, --help");
    std::println("    Shows this help message.");
    std::println("    Example: {} -h\n", program_name);

    std::println("  -V, --version");
    std::println("    Shows the program version.");
    std::println("    Example: {} -V\n", program_name);
}
int main(int argc, char* argv[]) {
    setup_console();
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    AppController controller;
    bool operation_successful = true;
    
    std::vector<std::string> args;
    std::string command;
    std::string path_or_value;
    std::string format_str = "md";
    std::string export_type_filter; // --- 新增：用于存储 --type 参数 ---
    bool format_specified = false;

    // --- 参数解析逻辑 (保持不变) ---
    // --- 修改：参数解析逻辑，增加对 --type 的支持 ---
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--format" || arg == "-f") {
            if (i + 1 < argc) format_str = argv[++i], format_specified = true;
            else { std::cerr << "Error: Missing value for format flag.\n"; return 1; }
        } else if (arg == "--type" || arg == "-t") { // 新增
            if (i + 1 < argc) export_type_filter = argv[++i];
            else { std::cerr << "Error: Missing value for --type flag (should be 'month' or 'year').\n"; return 1; }
        }
        else {
            args.push_back(arg);
        }
    }

    if (!args.empty()) {
        command = args[0];
        if ((command == "--query" || command == "-q" || command == "--export" || command == "-e" || command == "--all" || command == "-a") && args.size() > 1) {
            command += " " + args[1];
            if (args.size() > 2) {
                path_or_value = args[2];
            }
        } else if (args.size() > 1) {
            path_or_value = args[1];
        }
    }
    
    try {
        if (command == "--help" || command == "-h") {
            print_help(argv[0]);
        } 
        else if (command == "--version" || command == "-V") {
            controller.display_version();
        }
        else if (command == "--all export" || command == "-a e") {
            if (path_or_value.empty()) { 
                std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path for 'all export' command.\n"; 
                return 1; 
            }

            // **修改点2: 检查工作流的返回值**
            // 假设 handle_full_workflow 也被修改为返回 bool
            std::println(CYAN_COLOR, "\n--- Step 1/2: Running Full Processing Workflow (Validate, Modify, Import) ---", RESET_COLOR);
            if (!controller.handle_full_workflow(path_or_value)) {
                operation_successful = false;
            }
            
            // 只有在工作流成功时才继续导出
            if(operation_successful) {
                std::println(CYAN_COLOR, "\n--- Step 2/2: Exporting All Reports ---", RESET_COLOR);
                if (format_specified) {
                    std::cout << "Exporting all reports in " << format_str << " format...\n";
                    if (!controller.handle_export("all", "", format_str)) {
                        operation_successful = false;
                    }
                } else {
                    std::cout << "Exporting all reports in Markdown, LaTeX, Typst, and reST formats...\n";
                    // **修改点3: 只要有一个失败，整体就失败**
                    bool md_ok = controller.handle_export("all", "", "md");
                    bool tex_ok = controller.handle_export("all", "", "tex");
                    bool typ_ok = controller.handle_export("all", "", "typ");
                    bool rst_ok = controller.handle_export("all", "", "rst");
                    if (!md_ok || !tex_ok || !typ_ok || !rst_ok) {
                        operation_successful = false;
                    }
                }
            }

            if(operation_successful) {
                std::println(GREEN_COLOR, "\n✅ Entire workflow completed successfully!", RESET_COLOR);
            } else {
                std::println(RED_COLOR, "\n❌ Entire workflow failed. Check logs for details.", RESET_COLOR);
            }
        }
        else if (command == "--export all" || command == "-e a") {
            
            std::string export_target = "all";
            if (export_type_filter == "month" || export_type_filter == "m") {
                export_target = "all_months";
            } else if (export_type_filter == "year" || export_type_filter == "y") {
                export_target = "all_years";
            } else if (!export_type_filter.empty()) { // --type 传入类型检查
                std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Unknown value for --type or -t: '" << export_type_filter << "'.\n";
                std::cerr << "Please use 'month', 'm', 'year', or 'y'.\n";
                return 1; // 打印错误后，直接退出程序
            }

            if (format_specified) {
                std::cout << "Exporting reports in " << format_str << " format...\n";
                if (!controller.handle_export(export_target, "", format_str)) {
                    operation_successful = false;
                }
            } else {
                std::cout << "Exporting reports in Markdown, LaTeX, Typst, and reST formats...\n";
                bool md_ok = controller.handle_export(export_target, "", "md");
                bool tex_ok = controller.handle_export(export_target, "", "tex");
                bool typ_ok = controller.handle_export(export_target, "", "typ");
                bool rst_ok = controller.handle_export(export_target, "", "rst");
                if (!md_ok || !tex_ok || !typ_ok || !rst_ok) {
                    operation_successful = false;
                }
            }
        }

        else if (command == "--all process" || command == "-a p") {
            if (path_or_value.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path for 'all process' command.\n"; return 1; }
            if (!controller.handle_full_workflow(path_or_value)) operation_successful = false;
        }
        else if (command == "--validate" || command == "-v") {
            if (path_or_value.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path for 'validate' command.\n"; return 1; }
            if (!controller.handle_validation(path_or_value)) operation_successful = false;
        }
        else if (command == "--modify" || command == "-m") {
            if (path_or_value.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path for 'modify' command.\n"; return 1; }
            if (!controller.handle_modification(path_or_value)) operation_successful = false;
        }
        else if (command == "--import" || command == "-i") {
            if (path_or_value.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing path for 'import' command.\n"; return 1; }
            if (!controller.handle_import(path_or_value)) operation_successful = false;
        }
        else if (command == "--query year" || command == "-q y") {
            if (path_or_value.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <year> for 'query year' command.\n"; return 1; }
            if (!controller.handle_export("year", path_or_value, format_str)) operation_successful = false;
        }
        else if (command == "--query month" || command == "-q m") {
           if (path_or_value.empty()) { std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <month> for 'query month' command.\n"; return 1; }
           if (!controller.handle_export("month", path_or_value, format_str)) operation_successful = false;
       }
       // --- 新增: 处理 --export date 命令 ---
       else if (command == "--export date" || command == "-e d") {
        if (path_or_value.empty()) { 
            std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Missing <date> value for 'export date' command.\n"; 
            return 1; 
        }
        if (!controller.handle_export("date", path_or_value, format_str)) {
            operation_successful = false;
        }
        }
        else {
            std::cerr << RED_COLOR << "Error: " << RESET_COLOR << "Unknown or incomplete command '" << command << "'\n\n";
            print_help(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "\n" << RED_COLOR << "Critical Error: " << RESET_COLOR << e.what() << std::endl; 
        return 1; // 捕获到异常，直接返回失败
    }

    // **修改点5: 根据最终的成功标志返回退出码**
    return operation_successful ? 0 : 1; 
}