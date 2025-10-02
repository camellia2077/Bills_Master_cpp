// db_insert/DataProcessor.hpp

#ifndef DATA_PROCESSOR_HPP
#define DATA_PROCESSOR_HPP

#include <string>
#include "db_insert/insertor/BillInserter.hpp"

class DataProcessor {
public:
    bool process_and_insert(const std::string& bill_file_path, const std::string& db_path);
};

#endif // DATA_PROCESSOR_HPP