// ingest/convert/bills_converter.hpp
#ifndef INGEST_CONVERT_BILLS_CONVERTER_H_
#define INGEST_CONVERT_BILLS_CONVERTER_H_

#include <string>

#include "domain/bill/bill_record.hpp"
#include "config/modifier_data.hpp"

class BillConverter {
 public:
  explicit BillConverter(Config config);

  ParsedBill convert(const std::string& bill_content);

 private:
  Config m_config;
};

#endif  // INGEST_CONVERT_BILLS_CONVERTER_H_
