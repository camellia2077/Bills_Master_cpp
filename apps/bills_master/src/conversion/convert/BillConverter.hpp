// conversion/convert/BillConverter.hpp
#ifndef BILL_CONVERTER_HPP
#define BILL_CONVERTER_HPP

#include <string>

#include "common/structures/CommonData.hpp"
#include "conversion/modifier/_shared_structures/BillDataStructures.hpp"

class BillConverter {
 public:
  explicit BillConverter(Config config);

  ParsedBill convert(const std::string& bill_content);

 private:
  Config m_config;
};

#endif  // BILL_CONVERTER_HPP
