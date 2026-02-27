#include "abi/internal/abi_shared.hpp"

#include "billing/conversion/bills_processing_pipeline.hpp"
#include "ports/bills_repository.hpp"
#include "serialization/bills_json_serializer.hpp"

namespace bills::core::abi {

namespace {

class InMemoryBillRepository final : public BillRepository {
 public:
  void InsertBill(const ParsedBill& bill_data) override {
    bills_.push_back(bill_data);
  }

  [[nodiscard]] auto Size() const -> std::size_t { return bills_.size(); }

 private:
  std::vector<ParsedBill> bills_;
};

}  // namespace

auto handle_ingest_command(const Json& request) -> std::string {
  const Json params = request.value("params", Json::object());
  if (!params.is_object()) {
    return make_response(false, error_code::kParamInvalidRequest,
                         "'params' must be a JSON object.");
  }

  const std::string input_path = params.value("input_path", "");
  if (input_path.empty()) {
    return make_response(false, error_code::kParamInvalidRequest,
                         "Ingest requires non-empty 'params.input_path'.");
  }

  const std::string output_dir_raw =
      params.value("output_dir",
                   std::string(constants::kDefaultConvertOutputDir));
  const bool write_json = params.value("write_json", false);
  const bool include_serialized_json =
      params.value("include_serialized_json", false);

  BillConfig validator_config{BillValidationRules{}};
  Config modifier_config{};
  const std::string config_error =
      read_and_validate_configs(params, validator_config, modifier_config);
  if (!config_error.empty()) {
    Json data;
    data["detail"] = config_error;
    return make_response(false, error_code::kParamInvalidConfig,
                         "Failed to load/validate configuration.",
                         std::move(data));
  }

  std::vector<fs::path> files;
  try {
    files = list_txt_files(fs::path(input_path));
  } catch (const std::exception& ex) {
    Json data;
    data["detail"] = ex.what();
    return make_response(false, error_code::kParamInvalidInputPath,
                         "Failed to enumerate input files.", std::move(data));
  }

  if (files.empty()) {
    Json data;
    data["input_path"] = input_path;
    return make_response(false, error_code::kBusinessNoInputFiles,
                         "No .txt files found under input_path.",
                         std::move(data));
  }

  const fs::path output_dir = fs::path(output_dir_raw);
  BillProcessingPipeline pipeline(validator_config, modifier_config);
  InMemoryBillRepository repository{};

  Json file_results = Json::array();
  std::size_t success = 0;
  std::size_t failure = 0;

  for (const auto& file : files) {
    Json item;
    item["path"] = file.string();
    ParsedBill bill_data{};

    try {
      const std::string content = read_text_file(file);
      const bool ingested =
          pipeline.validate_and_convert_content(content, file.string(), bill_data);
      item["ok"] = ingested;
      if (!ingested) {
        ++failure;
        file_results.push_back(std::move(item));
        continue;
      }

      if (write_json) {
        const fs::path final_output_path =
            build_convert_output_path(output_dir, file);
        BillJsonSerializer::write_to_file(bill_data, final_output_path.string());
        item["output_path"] = final_output_path.string();
      }

      repository.InsertBill(bill_data);
      if (include_serialized_json) {
        item["json"] = BillJsonSerializer::serialize(bill_data);
      }

      ++success;
    } catch (const std::exception& ex) {
      item["ok"] = false;
      item["error"] = ex.what();
      ++failure;
    }

    file_results.push_back(std::move(item));
  }

  Json data;
  data["input_path"] = input_path;
  data["output_dir"] = output_dir_raw;
  data["write_json"] = write_json;
  data["processed"] = files.size();
  data["success"] = success;
  data["failure"] = failure;
  data["imported"] = repository.Size();
  data["repository_mode"] = "memory";
  data["all_ingested"] = (failure == 0U);
  data["files"] = std::move(file_results);

  if (failure == 0U) {
    return make_response(true, error_code::kOk, "Ingest completed successfully.",
                         std::move(data));
  }
  return make_response(false, error_code::kBusinessIngestFailed,
                       "One or more files failed ingest.", std::move(data));
}

auto handle_import_command(const Json& request) -> std::string {
  const Json params = request.value("params", Json::object());
  if (!params.is_object()) {
    return make_response(false, error_code::kParamInvalidRequest,
                         "'params' must be a JSON object.");
  }

  const std::string input_path = params.value("input_path", "");
  if (input_path.empty()) {
    return make_response(false, error_code::kParamInvalidRequest,
                         "Import requires non-empty 'params.input_path'.");
  }

  std::vector<fs::path> files;
  try {
    files = list_json_files(fs::path(input_path));
  } catch (const std::exception& ex) {
    Json data;
    data["detail"] = ex.what();
    return make_response(false, error_code::kParamInvalidInputPath,
                         "Failed to enumerate input files.", std::move(data));
  }

  if (files.empty()) {
    Json data;
    data["input_path"] = input_path;
    return make_response(false, error_code::kBusinessNoInputFiles,
                         "No .json files found under input_path.",
                         std::move(data));
  }

  InMemoryBillRepository repository{};
  Json file_results = Json::array();
  std::size_t success = 0;
  std::size_t failure = 0;

  for (const auto& file : files) {
    Json item;
    item["path"] = file.string();

    try {
      const ParsedBill bill_data = BillJsonSerializer::read_from_file(file.string());
      repository.InsertBill(bill_data);
      item["ok"] = true;
      item["date"] = bill_data.date;
      item["year"] = bill_data.year;
      item["month"] = bill_data.month;
      item["transaction_count"] = bill_data.transactions.size();
      ++success;
    } catch (const std::exception& ex) {
      item["ok"] = false;
      item["error"] = ex.what();
      ++failure;
    }

    file_results.push_back(std::move(item));
  }

  Json data;
  data["input_path"] = input_path;
  data["processed"] = files.size();
  data["success"] = success;
  data["failure"] = failure;
  data["imported"] = repository.Size();
  data["repository_mode"] = "memory";
  data["all_imported"] = (failure == 0U);
  data["files"] = std::move(file_results);

  if (failure == 0U) {
    return make_response(true, error_code::kOk, "Import completed successfully.",
                         std::move(data));
  }
  return make_response(false, error_code::kBusinessImportFailed,
                       "One or more files failed import.", std::move(data));
}

}  // namespace bills::core::abi
