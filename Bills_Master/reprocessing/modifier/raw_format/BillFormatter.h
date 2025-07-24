#ifndef BILL_FORMATTER_H
#define BILL_FORMATTER_H

#include "reprocessing/modifier/_shared_structures/BillDataStructures.h"

class BillFormatter {
public:
    explicit BillFormatter(const Config& config);

    std::string format(const std::vector<ParentItem>& bill_structure, const std::vector<std::string>& metadata_lines) const;

private:
    const Config& m_config;
};

#endif // BILL_FORMATTER_H