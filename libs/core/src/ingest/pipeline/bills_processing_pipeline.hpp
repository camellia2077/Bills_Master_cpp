// ingest/pipeline/bills_processing_pipeline.hpp
#ifndef INGEST_PIPELINE_BILLS_PROCESSING_PIPELINE_H_
#define INGEST_PIPELINE_BILLS_PROCESSING_PIPELINE_H_

#include <memory>
#include <string>
#include <vector>

#include "domain/bill/bill_record.hpp"
#include "ingest/convert/bills_converter.hpp"
#include "ingest/validation/bills_validator.hpp"

class BillProcessingPipeline {
 public:
  /**
     * @brief 构造函数，使用已解析的配置对象进行初始化。
     * @param
   * validator_config 用于 BillValidator 的配置。
     * @param modifier_config
   * 用于 BillConverter 的配置。
     */
  explicit BillProcessingPipeline(BillConfig validator_config,
                                  Config modifier_config);

  bool validate_content(const std::string& bill_content,
                        const std::string& source_name);
  bool validate_and_convert_content(const std::string& bill_content,
                                    const std::string& source_name,
                                    ParsedBill& bill_data);
  bool convert_content(const std::string& bill_content, ParsedBill& bill_data);

  [[nodiscard]] auto last_failure_stage() const -> const std::string&;
  [[nodiscard]] auto last_failure_message() const -> const std::string&;
  [[nodiscard]] auto last_failure_messages() const
      -> const std::vector<std::string>&;

 private:
  void clear_last_failure();
  void set_last_failure(std::string stage, std::string message);
  void set_last_failure(std::string stage,
                        const std::vector<std::string>& messages);

  std::unique_ptr<BillValidator> m_validator;
  std::unique_ptr<BillConverter> m_converter;
  std::string last_failure_stage_;
  std::string last_failure_message_;
  std::vector<std::string> last_failure_messages_;
};

#endif  // INGEST_PIPELINE_BILLS_PROCESSING_PIPELINE_H_
