#include "presentation/features/config/config_handler.hpp"

#include <iostream>

#include "bills_io/host_flow_support.hpp"
#include "common/common_utils.hpp"
#include "nlohmann/json.hpp"
#include "record_template/record_template_service.hpp"

namespace terminal = common::terminal;

namespace bills::cli {
namespace {

auto ToJson(const ConfigInspectResult& inspect_result) -> nlohmann::json {
  nlohmann::json json;
  json["schema_version"] = inspect_result.schema_version;
  json["date_format"] = inspect_result.date_format;
  json["metadata_headers"] = inspect_result.metadata_headers;

  nlohmann::json categories = nlohmann::json::array();
  for (const auto& category : inspect_result.categories) {
    nlohmann::json item;
    item["parent_item"] = category.parent_item;
    item["description"] = category.description;
    item["sub_items"] = category.sub_items;
    categories.push_back(std::move(item));
  }
  json["categories"] = std::move(categories);
  return json;
}

}  // namespace

ConfigHandler::ConfigHandler(const RuntimeContext& context) : context_(context) {}

auto ConfigHandler::Handle(const ConfigRequest& request) const -> bool {
  switch (request.action) {
    case ConfigAction::kInspect: {
      const auto validated_documents =
          bills::io::LoadValidatedConfigContext(context_.config_dir);
      if (!validated_documents) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatError(validated_documents.error()) << '\n';
        return false;
      }
      const auto inspect_result = RecordTemplateService::InspectConfig(
          validated_documents->documents.validator);
      if (!inspect_result) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatRecordTemplateError(inspect_result.error()) << '\n';
        return false;
      }
      std::cout << ToJson(*inspect_result).dump(2) << '\n';
      return true;
    }

    case ConfigAction::kFormats: {
      const auto formats = LoadEnabledFormats(context_);
      if (!formats) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatError(formats.error()) << '\n';
        return false;
      }
      for (const auto& format : *formats) {
        std::cout << format << '\n';
      }
      return true;
    }
  }

  return false;
}

}  // namespace bills::cli
