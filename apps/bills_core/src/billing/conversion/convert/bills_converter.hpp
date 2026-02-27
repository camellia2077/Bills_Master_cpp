// conversion/convert/bills_converter.hpp
#ifndef BILL_CONVERTER_HPP
#define BILL_CONVERTER_HPP

#include <string>

#include "domain/bill/bill_record.hpp"
#include "billing/conversion/modifier/_shared_structures/bills_data_structures.hpp"

class BillConverter {
 public:
  explicit BillConverter(Config config);

  ParsedBill convert(const std::string& bill_content);

 private:
  Config m_config;
};

#endif  // BILL_CONVERTER_HPP


