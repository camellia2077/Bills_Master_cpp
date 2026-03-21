// ingest/convert/bills_converter.cpp

#include "bills_converter.hpp"

#include "ingest/transform/bills_content_transformer.hpp"

BillConverter::BillConverter(Config config) : m_config(std::move(config)) {}

auto BillConverter::convert(const std::string& bill_content) -> ParsedBill {
  BillContentTransformer processor(m_config);
  return processor.process(bill_content);
}

