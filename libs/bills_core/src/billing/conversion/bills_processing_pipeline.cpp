// billing/conversion/bills_processing_pipeline.cpp
#include "billing/conversion/bills_processing_pipeline.hpp"

#include <sstream>
#include <utility>

#include "common/text_normalizer.hpp"
#include "billing/conversion/validator/result/validation_result.hpp"

BillProcessingPipeline::BillProcessingPipeline(BillConfig validator_config,
                                               Config modifier_config) {
  m_validator = std::make_unique<BillValidator>(std::move(validator_config));
  m_converter = std::make_unique<BillConverter>(std::move(modifier_config));
}

void BillProcessingPipeline::clear_last_failure() {
  last_failure_stage_.clear();
  last_failure_message_.clear();
  last_failure_messages_.clear();
}

void BillProcessingPipeline::set_last_failure(std::string stage,
                                              std::string message) {
  last_failure_stage_ = std::move(stage);
  last_failure_message_ = std::move(message);
  last_failure_messages_.clear();
  if (!last_failure_message_.empty()) {
    last_failure_messages_.push_back(last_failure_message_);
  }
}

void BillProcessingPipeline::set_last_failure(
    std::string stage, const std::vector<std::string>& messages) {
  last_failure_stage_ = std::move(stage);
  last_failure_messages_ = messages;
  if (last_failure_messages_.empty()) {
    last_failure_message_.clear();
    return;
  }

  std::ostringstream stream;
  for (std::size_t index = 0; index < last_failure_messages_.size(); ++index) {
    if (index != 0U) {
      stream << "; ";
    }
    stream << last_failure_messages_[index];
  }
  last_failure_message_ = stream.str();
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
  clear_last_failure();

  const auto normalized_text = NormalizeBillText(bill_content);
  if (!normalized_text) {
    set_last_failure("normalize_text", normalized_text.error().message_);
    return false;
  }

  ValidationResult result;
  if (!m_validator->validate_txt_structure(*normalized_text, result)) {
    set_last_failure("validate_structure", result.error_messages());
    return false;
  }

  try {
    bill_data = m_converter->convert(*normalized_text);
  } catch (const std::exception& ex) {
    set_last_failure("convert_content", ex.what());
    return false;
  }

  result.clear();
  if (!m_validator->validate_bill_content(bill_data, result)) {
    set_last_failure("validate_bill", result.error_messages());
    return false;
  }
  return true;
}

auto BillProcessingPipeline::convert_content(const std::string& bill_content,
                                             ParsedBill& bill_data) -> bool {
  clear_last_failure();
  const auto normalized_text = NormalizeBillText(bill_content);
  if (!normalized_text) {
    set_last_failure("normalize_text", normalized_text.error().message_);
    return false;
  }
  try {
    bill_data = m_converter->convert(*normalized_text);
    return true;
  } catch (const std::exception& ex) {
    set_last_failure("convert_content", ex.what());
    return false;
  }
}

auto BillProcessingPipeline::last_failure_stage() const -> const std::string& {
  return last_failure_stage_;
}

auto BillProcessingPipeline::last_failure_message() const -> const std::string& {
  return last_failure_message_;
}

auto BillProcessingPipeline::last_failure_messages() const
    -> const std::vector<std::string>& {
  return last_failure_messages_;
}
