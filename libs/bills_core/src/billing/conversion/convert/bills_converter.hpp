// billing/conversion/convert/bills_converter.hpp
#ifndef BILLING_CONVERSION_CONVERT_BILLS_CONVERTER_H_
#define BILLING_CONVERSION_CONVERT_BILLS_CONVERTER_H_

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

#endif  // BILLING_CONVERSION_CONVERT_BILLS_CONVERTER_H_


