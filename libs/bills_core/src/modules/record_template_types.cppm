module;
#include "record_template/import_preflight_service.hpp"
#include "record_template/record_template_types.hpp"

export module bill.core.record_template.types;

export namespace bills::core::modules::record_template {
using RecordTemplateErrorCategory = ::RecordTemplateErrorCategory;
using RecordTemplateError = ::RecordTemplateError;
using OrderedTemplateCategory = ::OrderedTemplateCategory;
using OrderedTemplateLayout = ::OrderedTemplateLayout;
using TemplateGenerationRequest = ::TemplateGenerationRequest;
using GeneratedTemplateFile = ::GeneratedTemplateFile;
using TemplateGenerationResult = ::TemplateGenerationResult;
using RecordPreviewFile = ::RecordPreviewFile;
using RecordPreviewResult = ::RecordPreviewResult;
using ConfigInspectResult = ::ConfigInspectResult;
using InvalidPeriodFile = ::InvalidPeriodFile;
using ListedPeriodsResult = ::ListedPeriodsResult;
using ImportPreflightRequest = ::ImportPreflightRequest;
using ImportPreflightResult = ::ImportPreflightResult;
}
