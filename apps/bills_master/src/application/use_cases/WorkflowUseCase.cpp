// application/use_cases/WorkflowUseCase.cpp
#include "WorkflowUseCase.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#include "common/common_utils.hpp"
#include "conversion/Reprocessor.hpp"

namespace fs = std::filesystem;

namespace {
const std::string kTextExtension = ".txt";
const std::string kJsonExtension = ".json";
const std::string kValidationLabel = "Validation";
const std::string kConversionLabel = "Conversion";
const std::string kIngestLabel = "Ingest";
const std::string kImportLabel = "Database Import";
const std::string kWorkflowLabel = "Full Workflow";
}  // namespace

WorkflowUseCase::WorkflowUseCase(BillConfig validator_config,
                                 Config modifier_config,
                                 BillContentReader& content_reader,
                                 BillFileEnumerator& file_enumerator,
                                 BillSerializer& serializer,
                                 PathBuilder& path_builder)
    : validator_config_(std::move(validator_config)),
      modifier_config_(std::move(modifier_config)),
      content_reader_(content_reader),
      file_enumerator_(file_enumerator),
      serializer_(serializer),
      path_builder_(path_builder) {}

auto WorkflowUseCase::Validate(const std::string& path)
    -> Result<ProcessStats> {
  ProcessStats stats;
  try {
    Reprocessor reprocessor(validator_config_, modifier_config_);
    std::vector<fs::path> files = file_enumerator_.ListFilesByExtension(
        std::string_view{path}, std::string_view{kTextExtension});
    for (const auto& file : files) {
      try {
        std::string bill_content = content_reader_.Read(file.string());
        if (reprocessor.validate_content(bill_content, file.string())) {
          stats.success++;
        } else {
          stats.failure++;
        }
      } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "错误: " << RESET_COLOR << e.what()
                  << std::endl;
        stats.failure++;
      }
    }
  } catch (const std::exception& e) {
    return std::unexpected(MakeError(std::string(e.what()), kValidationLabel));
  }
  stats.print_summary(kValidationLabel);
  return stats;
}

auto WorkflowUseCase::Convert(const std::string& path) -> Result<ProcessStats> {
  ProcessStats stats;
  try {
    Reprocessor reprocessor(validator_config_, modifier_config_);
    std::vector<fs::path> files = file_enumerator_.ListFilesByExtension(
        std::string_view{path}, std::string_view{kTextExtension});
    for (const auto& file : files) {
      fs::path final_output_path = path_builder_.build_output_path(file);

      std::cout << "\n--- 正在转换: " << file.string() << " -> "
                << final_output_path.string() << " ---\n";
      try {
        std::string bill_content = content_reader_.Read(file.string());
        ParsedBill bill_data{};
        if (!reprocessor.convert_content(bill_content, bill_data)) {
          stats.failure++;
          continue;
        }
        serializer_.WriteJson(bill_data, final_output_path.string());
        std::cout << "--- 转换成功。输出已保存至 '"
                  << final_output_path.string() << "' ---\n";
        stats.success++;
      } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "错误: " << RESET_COLOR << e.what()
                  << std::endl;
        stats.failure++;
      }
    }
  } catch (const std::exception& e) {
    return std::unexpected(MakeError(std::string(e.what()), kConversionLabel));
  }
  stats.print_summary(kConversionLabel);
  return stats;
}

auto WorkflowUseCase::Ingest(const std::string& path,
                             BillRepository& repository, bool write_json)
    -> Result<ProcessStats> {
  ProcessStats stats;
  std::cout << "--- Ingest workflow started ---\n";
  try {
    Reprocessor reprocessor(validator_config_, modifier_config_);
    std::vector<fs::path> files = file_enumerator_.ListFilesByExtension(
        std::string_view{path}, std::string_view{kTextExtension});
    if (files.empty()) {
      std::cout << "未找到需要处理的 .txt 文件。\n";
      stats.print_summary(kIngestLabel);
      return stats;
    }

    for (const auto& file : files) {
      std::cout << "\n========================================\n";
      std::cout << "正在处理文件: " << file.string() << "\n";
      std::cout << "========================================\n";

      ParsedBill bill_data{};
      try {
        std::string bill_content = content_reader_.Read(file.string());
        if (!reprocessor.validate_and_convert_content(
                bill_content, file.string(), bill_data)) {
          std::cerr << RED_COLOR << "验证或转换失败" << RESET_COLOR
                    << " 文件: " << file.string() << "。已跳过此文件。\n";
          stats.failure++;
          continue;
        }
      } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "错误: " << RESET_COLOR << e.what()
                  << std::endl;
        stats.failure++;
        continue;
      }

      if (write_json) {
        fs::path output_path = path_builder_.build_output_path(file);
        try {
          serializer_.WriteJson(bill_data, output_path.string());
          std::cout << GREEN_COLOR << "成功: " << RESET_COLOR
                    << "JSON 已保存至 '" << output_path.string() << "'.\n";
        } catch (const std::exception& e) {
          std::cerr << RED_COLOR << "JSON 写入失败" << RESET_COLOR
                    << " 文件: " << file.string() << "，原因: " << e.what()
                    << "\n";
          stats.failure++;
          continue;
        }
      }

      try {
        repository.InsertBill(bill_data);
        std::cout << GREEN_COLOR << "成功: " << RESET_COLOR
                  << "此文件的数据已成功导入数据库。\n";
        stats.success++;
      } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "数据库导入失败" << RESET_COLOR
                  << " 文件: " << file.string() << "，原因: " << e.what()
                  << "\n";
        stats.failure++;
      }
    }
  } catch (const std::exception& e) {
    return std::unexpected(MakeError(std::string(e.what()), kIngestLabel));
  }
  stats.print_summary(kIngestLabel);
  return stats;
}

auto WorkflowUseCase::Import(const std::string& path,
                             BillRepository& repository)
    -> Result<ProcessStats> {
  ProcessStats stats;
  try {
    std::vector<fs::path> files = file_enumerator_.ListFilesByExtension(
        std::string_view{path}, std::string_view{kJsonExtension});
    for (const auto& file : files) {
      std::cout << "\n--- 正在处理并导入数据库: " << file.string() << " ---\n";
      try {
        ParsedBill import_bill = serializer_.ReadJson(file.string());
        repository.InsertBill(import_bill);
        stats.success++;
      } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "数据库导入失败" << RESET_COLOR
                  << " 文件: " << file.string() << "，原因: " << e.what()
                  << "\n";
        stats.failure++;
      }
    }
  } catch (const std::exception& e) {
    return std::unexpected(MakeError(std::string(e.what()), kImportLabel));
  }
  stats.print_summary(kImportLabel);
  return stats;
}

auto WorkflowUseCase::FullWorkflow(const std::string& path,
                                   BillRepository& repository)
    -> Result<ProcessStats> {
  ProcessStats stats;
  std::cout << "--- 自动化处理工作流已启动 ---\n";
  try {
    Reprocessor reprocessor(validator_config_, modifier_config_);
    std::vector<fs::path> files = file_enumerator_.ListFilesByExtension(
        std::string_view{path}, std::string_view{kTextExtension});
    if (files.empty()) {
      std::cout << "未找到需要处理的 .txt 文件。\n";
      stats.print_summary(kWorkflowLabel);
      return stats;
    }

    for (const auto& file_path : files) {
      std::cout << "\n========================================\n";
      std::cout << "正在处理文件: " << file_path.string() << "\n";
      std::cout << "========================================\n";
      std::string bill_content;
      try {
        bill_content = content_reader_.Read(file_path.string());
      } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "错误: " << RESET_COLOR << e.what()
                  << std::endl;
        stats.failure++;
        continue;
      }

      std::cout << "\n[步骤 1/3] 正在验证账单文件...\n";
      if (!reprocessor.validate_content(bill_content, file_path.string())) {
        std::cerr << RED_COLOR << "验证失败" << RESET_COLOR
                  << " 文件: " << file_path.string() << "。已跳过此文件。\n";
        stats.failure++;
        continue;
      }
      std::cout << GREEN_COLOR << "成功: " << RESET_COLOR << "验证完成。\n";

      fs::path modified_path = path_builder_.build_output_path(file_path);

      std::cout << "\n[步骤 2/3] 正在转换账单文件...\n";
      ParsedBill bill_data{};
      if (!reprocessor.convert_content(bill_content, bill_data)) {
        std::cerr << RED_COLOR << "转换失败" << RESET_COLOR
                  << " 文件: " << file_path.string() << "。已跳过此文件。\n";
        stats.failure++;
        continue;
      }
      try {
        serializer_.WriteJson(bill_data, modified_path.string());
        std::cout << GREEN_COLOR << "成功: " << RESET_COLOR
                  << "转换完成。转换后的文件已保存至 '"
                  << modified_path.string() << "'.\n";
      } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "JSON 写入失败" << RESET_COLOR
                  << " 文件: " << file_path.string() << "，原因: " << e.what()
                  << "\n";
        stats.failure++;
        continue;
      }

      std::cout << "\n[步骤 3/3] 正在解析并插入数据库...\n";
      try {
        ParsedBill import_bill = serializer_.ReadJson(modified_path.string());
        repository.InsertBill(import_bill);
        std::cout << GREEN_COLOR << "成功: " << RESET_COLOR
                  << "此文件的数据已成功导入数据库。\n";
        stats.success++;
      } catch (const std::exception& e) {
        std::cerr << RED_COLOR << "数据库导入失败" << RESET_COLOR
                  << " 文件: " << modified_path.string()
                  << "，原因: " << e.what() << "\n";
        stats.failure++;
      }
    }
  } catch (const std::exception& e) {
    return std::unexpected(MakeError(std::string(e.what()), kWorkflowLabel));
  }
  stats.print_summary(kWorkflowLabel);
  return stats;
}
