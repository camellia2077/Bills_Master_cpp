// billing/conversion/bills_processing_pipeline.cpp
#include "billing/conversion/bills_processing_pipeline.hpp"

#include "billing/conversion/validator/result/validation_result.hpp"

BillProcessingPipeline::BillProcessingPipeline(BillConfig validator_config,
                                               Config modifier_config) {
  m_validator = std::make_unique<BillValidator>(std::move(validator_config));
  m_converter = std::make_unique<BillConverter>(std::move(modifier_config));
}

auto BillProcessingPipeline::validate_content(const std::string& bill_content,
                                              const std::string& source_name)
    -> bool {
  ParsedBill bill_data{};
  return validate_and_convert_content(bill_content, source_name, bill_data);
}

auto BillProcessingPipeline::validate_and_convert_content(
    const std::string& bill_content, const std::string& source_name,
    ParsedBill& bill_data) -> bool {
  (void)source_name;

  ValidationResult result;
  if (!m_validator->validate_txt_structure(bill_content, result)) {
    return false;
  }

  try {
    bill_data = m_converter->convert(bill_content);
  } catch (const std::exception&) {
    return false;
  }

  result.clear();
  return m_validator->validate_bill_content(bill_data, result);
}

auto BillProcessingPipeline::convert_content(const std::string& bill_content,
                                             ParsedBill& bill_data) -> bool {
  try {
    bill_data = m_converter->convert(bill_content);
    return true;
  } catch (const std::exception&) {
    return false;
  }
}
