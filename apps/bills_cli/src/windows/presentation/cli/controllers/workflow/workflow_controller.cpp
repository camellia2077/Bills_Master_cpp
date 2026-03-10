// windows/presentation/cli/controllers/workflow/workflow_controller.cpp
#include "workflow_controller.hpp"

#include <iostream>
#include <stdexcept>

#include "bills_io/io_factory.hpp"
#include "application/use_cases/workflow_use_case.hpp"
#include "common/common_utils.hpp"
#include "ports/output_path_builder.hpp"

namespace terminal = common::terminal;

// 构造函数现在创建并持有 FileHandler，并将其传递给依赖项
WorkflowController::WorkflowController(const std::string& config_path,
                                       const std::string& modified_output_dir)
{
  m_config_provider = bills::io::CreateConfigProvider();
  Result<ConfigBundle> config_result = m_config_provider->Load(config_path);
  if (!config_result) {
    m_init_error = FormatError(config_result.error());
    return;
  }

  m_content_reader = bills::io::CreateBillContentReader();
  m_file_enumerator = bills::io::CreateBillFileEnumerator();
  m_serializer = bills::io::CreateBillSerializer();
  m_output_path_builder =
      bills::io::CreateYearPartitionOutputPathBuilder(modified_output_dir);

  m_use_case = std::make_unique<WorkflowUseCase>(
      std::move(config_result->validator_config),
      std::move(config_result->modifier_config), *m_content_reader,
      *m_file_enumerator, *m_serializer, *m_output_path_builder);
}

WorkflowController::~WorkflowController() = default;

auto WorkflowController::ensure_initialized() const -> bool {
  if (m_init_error.has_value()) {
    std::cerr << terminal::kRed << "错误: " << terminal::kReset << m_init_error.value()
              << std::endl;
    return false;
  }
  if (!m_use_case) {
    std::cerr << terminal::kRed << "错误: " << terminal::kReset
              << "Workflow 用例未初始化。" << std::endl;
    return false;
  }
  return true;
}

auto WorkflowController::handle_validation(const std::string& path) -> bool {
  if (!ensure_initialized()) {
    return false;
  }
  const auto kResult = m_use_case->Validate(path);
  if (!kResult) {
    std::cerr << terminal::kRed << "错误: " << terminal::kReset
              << FormatError(kResult.error()) << std::endl;
    return false;
  }
  return kResult->failure == 0;
}

auto WorkflowController::handle_convert(const std::string& path) -> bool {
  if (!ensure_initialized()) {
    return false;
  }
  const auto kResult = m_use_case->Convert(path);
  if (!kResult) {
    std::cerr << terminal::kRed << "错误: " << terminal::kReset
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
  auto repository = bills::io::CreateBillRepository(db_path);
  const auto kResult = m_use_case->Ingest(path, *repository, write_json);
  if (!kResult) {
    std::cerr << terminal::kRed << "错误: " << terminal::kReset
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
  auto repository = bills::io::CreateBillRepository(db_path);
  const auto kResult = m_use_case->Import(path, *repository);
  if (!kResult) {
    std::cerr << terminal::kRed << "错误: " << terminal::kReset
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
  auto repository = bills::io::CreateBillRepository(db_path);
  const auto kResult = m_use_case->FullWorkflow(path, *repository);
  if (!kResult) {
    std::cerr << terminal::kRed << "错误: " << terminal::kReset
              << FormatError(kResult.error()) << std::endl;
    return false;
  }
  return kResult->failure == 0;
}

