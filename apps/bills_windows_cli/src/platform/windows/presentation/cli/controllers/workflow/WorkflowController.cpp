// controllers/workflow/WorkflowController.cpp
#include "WorkflowController.hpp"

#include <iostream>
#include <stdexcept>

#include "platform/windows/infrastructure/adapters/config/JsonConfigProvider.hpp"
#include "platform/windows/infrastructure/adapters/db/SqliteBillRepository.hpp"
#include "platform/windows/infrastructure/adapters/io/FileBillContentReader.hpp"
#include "platform/windows/infrastructure/adapters/io/FileBillFileEnumerator.hpp"
#include "adapters/serialization/JsonBillSerializer.hpp"
#include "common/process_stats.hpp"
#include "common/common_utils.hpp"

// 构造函数现在创建并持有 FileHandler，并将其传递给依赖项
WorkflowController::WorkflowController(const std::string& config_path,
                                       const std::string& modified_output_dir)
    : m_file_handler(),
      m_path_builder(modified_output_dir,
                     m_file_handler)  // 将 m_file_handler 注入 PathBuilder
{
  m_config_provider = std::make_unique<JsonConfigProvider>(m_file_handler);
  Result<ConfigBundle> config_result = m_config_provider->Load(config_path);
  if (!config_result) {
    m_init_error = config_result.error();
    return;
  }

  m_content_reader = std::make_unique<FileBillContentReader>();
  m_file_enumerator = std::make_unique<FileBillFileEnumerator>(m_file_handler);
  m_serializer = std::make_unique<JsonBillSerializer>();

  m_use_case = std::make_unique<WorkflowUseCase>(
      std::move(config_result->validator_config),
      std::move(config_result->modifier_config), *m_content_reader,
      *m_file_enumerator, *m_serializer, m_path_builder);
}

auto WorkflowController::ensure_initialized() const -> bool {
  if (m_init_error.has_value()) {
    std::cerr << RED_COLOR << "错误: " << RESET_COLOR
              << FormatError(m_init_error.value()) << std::endl;
    return false;
  }
  if (!m_use_case) {
    std::cerr << RED_COLOR << "错误: " << RESET_COLOR
              << "Workflow 用例未初始化。" << std::endl;
    return false;
  }
  return true;
}

auto WorkflowController::handle_validation(const std::string& path) -> bool {
  if (!ensure_initialized()) {
    return false;
  }
  const Result<ProcessStats> kResult = m_use_case->Validate(path);
  if (!kResult) {
    std::cerr << RED_COLOR << "错误: " << RESET_COLOR
              << FormatError(kResult.error()) << std::endl;
    return false;
  }
  return kResult->failure == 0;
}

auto WorkflowController::handle_modification(const std::string& path) -> bool {
  return handle_convert(path);
}

auto WorkflowController::handle_convert(const std::string& path) -> bool {
  if (!ensure_initialized()) {
    return false;
  }
  const Result<ProcessStats> kResult = m_use_case->Convert(path);
  if (!kResult) {
    std::cerr << RED_COLOR << "错误: " << RESET_COLOR
              << FormatError(kResult.error()) << std::endl;
    return false;
  }
  return kResult->failure == 0;
}

auto WorkflowController::handle_ingest(const std::string& path,
                                       const std::string& db_path,
                                       bool write_json) -> bool {
  if (!ensure_initialized()) {
    return false;
  }
  SqliteBillRepository repository(db_path);
  const Result<ProcessStats> kResult =
      m_use_case->Ingest(path, repository, write_json);
  if (!kResult) {
    std::cerr << RED_COLOR << "错误: " << RESET_COLOR
              << FormatError(kResult.error()) << std::endl;
    return false;
  }
  return kResult->failure == 0;
}

auto WorkflowController::handle_import(const std::string& path,
                                       const std::string& db_path) -> bool {
  if (!ensure_initialized()) {
    return false;
  }
  std::cout << "正在使用数据库文件: " << db_path << "\n";
  SqliteBillRepository repository(db_path);
  const Result<ProcessStats> kResult = m_use_case->Import(path, repository);
  if (!kResult) {
    std::cerr << RED_COLOR << "错误: " << RESET_COLOR
              << FormatError(kResult.error()) << std::endl;
    return false;
  }
  return kResult->failure == 0;
}

auto WorkflowController::handle_full_workflow(const std::string& path,
                                              const std::string& db_path)
    -> bool {
  if (!ensure_initialized()) {
    return false;
  }
  SqliteBillRepository repository(db_path);
  const Result<ProcessStats> kResult =
      m_use_case->FullWorkflow(path, repository);
  if (!kResult) {
    std::cerr << RED_COLOR << "错误: " << RESET_COLOR
              << FormatError(kResult.error()) << std::endl;
    return false;
  }
  return kResult->failure == 0;
}

