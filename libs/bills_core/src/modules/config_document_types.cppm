module;
#include "config/config_document_types.hpp"
#include "config/modifier_data.hpp"
#include "ingest/validation/bills_config.hpp"

export module bill.core.config.document_types;

export namespace bills::core::modules::config {
using ValidatorCategoryDocument = ::ValidatorCategoryDocument;
using ValidatorConfigDocument = ::ValidatorConfigDocument;
using ModifierRuleDocument = ::ModifierRuleDocument;
using ModifierConfigDocument = ::ModifierConfigDocument;
using ExportFormatsDocument = ::ExportFormatsDocument;
using ConfigDocumentBundle = ::ConfigDocumentBundle;
using Config = ::Config;
using BillConfig = ::BillConfig;
using BillValidationRules = ::BillValidationRules;
}
