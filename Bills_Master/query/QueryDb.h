// QueryDb.h
#ifndef QUERY_FACADE_H
#define QUERY_FACADE_H

#include <string>
#include <vector>
#include <map> // 新增: 用于存储格式化目录名的映射
#include <sqlite3.h>

class QueryFacade {
public:
    // --- 修改: 构造函数增加了 export_base_dir 和 format_folder_names 参数 ---
    // 为新参数提供了默认值，以保持向后兼容。
    // 使用 C++11 的列表初始化 {} 为 map 提供默认空值。
    explicit QueryFacade(
        const std::string& db_path, 
        const std::string& plugin_directory_path,
        const std::string& export_base_dir = "exported_files",
        const std::map<std::string, std::string>& format_folder_names = {}
    );
    explicit QueryFacade(
        const std::string& db_path, 
        const std::vector<std::string>& plugin_paths,
        const std::string& export_base_dir = "exported_files",
        const std::map<std::string, std::string>& format_folder_names = {}
    );
    ~QueryFacade();

    QueryFacade(const QueryFacade&) = delete;
    QueryFacade& operator=(const QueryFacade&) = delete;

    // Public API for report generation
    std::string get_yearly_summary_report(int year, const std::string& format_name);
    std::string get_monthly_details_report(int year, int month, const std::string& format_name);

    std::vector<std::string> get_all_bill_dates();

    // Export methods remain the same
    bool export_yearly_report(const std::string& year_str, const std::string& format_name, bool suppress_output = false);
    bool export_monthly_report(const std::string& month_str, const std::string& format_name, bool suppress_output = false);
    bool export_all_reports(const std::string& format_name);

private:
    sqlite3* m_db;
    std::string m_plugin_directory_path;
    std::vector<std::string> m_plugin_paths;
    bool m_use_plugin_list;

    // --- 新增: 用于存储导出配置的成员变量 ---
    std::string m_export_base_dir;
    std::map<std::string, std::string> m_format_folder_names;

    void save_report(const std::string& report_content, const std::string& file_path_str);
    std::string get_display_format_name(const std::string& short_name) const;
};

#endif // QUERY_FACADE_H