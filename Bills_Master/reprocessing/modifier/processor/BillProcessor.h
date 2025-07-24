#ifndef BILL_PROCESSOR_H
#define BILL_PROCESSOR_H

#include "reprocessing/modifier/_shared_structures/BillDataStructures.h"

class BillProcessor {
public:
    explicit BillProcessor(const Config& config);

    std::vector<ParentItem> process(const std::string& bill_content, std::vector<std::string>& out_metadata_lines);

private:
    const Config& m_config;

    // --- Private Member Functions ---
    void _perform_initial_modifications(std::vector<std::string>& lines);
    void _sum_up_line(std::string& line);
    std::vector<ParentItem> _parse_into_structure(const std::vector<std::string>& lines, std::vector<std::string>& metadata_lines) const;
    void _sort_bill_structure(std::vector<ParentItem>& bill_structure) const;
    void _cleanup_bill_structure(std::vector<ParentItem>& bill_structure) const;

    // --- Private Static Helper Functions ---
    static std::vector<std::string> _split_string_by_lines(const std::string& str);
    static std::string& _trim(std::string& s);
    bool _is_metadata_line(const std::string& line) const;
    static double _get_numeric_value_from_content(const std::string& content_line);
    static bool _is_parent_title(const std::string& line);
    static bool _is_title(const std::string& line);
};

#endif // BILL_PROCESSOR_H