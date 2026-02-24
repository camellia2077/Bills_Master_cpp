// conversion/convert/BillConverter.cpp

#include "BillConverter.hpp"

#include "conversion/modifier/processor/BillContentTransformer.hpp"

BillConverter::BillConverter(Config config) : m_config(std::move(config)) {}

auto BillConverter::convert(const std::string& bill_content) -> ParsedBill {
  BillContentTransformer processor(m_config);
  return processor.process(bill_content);
}
