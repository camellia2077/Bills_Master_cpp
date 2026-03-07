// modules/phase2_core_module_import_smoke.cpp
import bill.core.common.process_stats;
import bill.core.common.result;
import bill.core.common.version;
import bill.core.config.bill_config;
import bill.core.config.modifier_data;
import bill.core.config.modifier;
import bill.core.config.validator;
import bill.core.domain.bill_record;
import bill.core.ports.bills_content_reader;
import bill.core.ports.bills_file_enumerator;
import bill.core.ports.bills_repository;
import bill.core.ports.bills_serializer;
import bill.core.ports.output_path_builder;
import bill.core.serialization.bill_json_serializer;

namespace {
using bills::core::modules::common_process_stats::ProcessStats;
using bills::core::modules::common_result::Error;
using bills::core::modules::common_result::MakeError;
using bills::core::modules::config::BillConfig;
using bills::core::modules::config::BillValidationRules;
using bills::core::modules::config::Config;
using bills::core::modules::config_validator::ModifierConfigValidator;
using bills::core::modules::config_validator::ValidatorConfigValidator;
using bills::core::modules::domain_bill_record::ParsedBill;
using bills::core::modules::domain_bill_record::Transaction;
using bills::core::modules::ports::BillContentReader;
using bills::core::modules::ports::BillFileEnumerator;
using bills::core::modules::ports::BillRepository;
using bills::core::modules::ports::BillSerializer;
using bills::core::modules::ports::OutputPathBuilder;
using bills::core::modules::serialization::BillJsonSerializer;

[[maybe_unused]] Error kPhase2Error = MakeError("phase2 module smoke");
[[maybe_unused]] ProcessStats kPhase2ProcessStats{};
[[maybe_unused]] Transaction kPhase2Transaction{};
[[maybe_unused]] ParsedBill kPhase2ParsedBill{};
[[maybe_unused]] Config kPhase2ModifierConfig{};
[[maybe_unused]] BillConfig kPhase2BillConfig{BillValidationRules{}};

[[maybe_unused]] constexpr auto kPhase2Version =
    bills::core::modules::common_version::kVersion;
[[maybe_unused]] constexpr auto kPhase2LastUpdated =
    bills::core::modules::common_version::kLastUpdated;

[[maybe_unused]] auto* kValidatorEntry = &ValidatorConfigValidator::validate;
[[maybe_unused]] auto* kModifierEntry = &ModifierConfigValidator::validate;
[[maybe_unused]] auto* kSerializerEntry = &BillJsonSerializer::serialize;
[[maybe_unused]] auto* kReadEntry = &BillJsonSerializer::read_from_file;
[[maybe_unused]] auto kReadPortEntry = &BillContentReader::Read;
[[maybe_unused]] auto kListPortEntry = &BillFileEnumerator::ListFilesByExtension;
[[maybe_unused]] auto kInsertPortEntry = &BillRepository::InsertBill;
[[maybe_unused]] auto kWritePortEntry = &BillSerializer::WriteJson;
[[maybe_unused]] auto kOutputPathPortEntry = &OutputPathBuilder::build_output_path;
}  // namespace
