// adapters/db/SqliteBillRepository.hpp
#ifndef SQLITE_BILL_REPOSITORY_HPP
#define SQLITE_BILL_REPOSITORY_HPP

#include "ports/BillRepository.hpp"
#include <string>

class SqliteBillRepository : public BillRepository {
public:
    explicit SqliteBillRepository(std::string db_path);
    void InsertBill(const ParsedBill& bill_data) override;

private:
    std::string db_path_;
};

#endif // SQLITE_BILL_REPOSITORY_HPP
