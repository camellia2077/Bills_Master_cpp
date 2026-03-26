#if defined(BILLS_CLI_MODULES_ENABLED)
import bill.cli.presentation.features.config_handler;
import bill.cli.presentation.entry.runtime_context;
import bill.cli.presentation.parsing.cli_request;
import bill.cli.deps.io_host_flow_support;
import bill.cli.deps.common_utils;
#else
#include <presentation/features/config/config_handler.hpp>
#endif

#include <pch.hpp>
#include <common/Result.hpp>
#include <nlohmann/json.hpp>

#include <iostream>

namespace terminal = bills::cli::terminal;

namespace bills::cli {
namespace {

auto ToJson(const bills::io::HostConfigInspectionResult& inspect_result)
    -> nlohmann::json {
  nlohmann::json json;
  json["schema_version"] = inspect_result.inspect.schema_version;
  json["date_format"] = inspect_result.inspect.date_format;
  json["metadata_headers"] = inspect_result.inspect.metadata_headers;
  json["enabled_export_formats"] = inspect_result.enabled_export_formats;
  json["available_export_formats"] = inspect_result.available_export_formats;

  nlohmann::json categories = nlohmann::json::array();
  for (const auto& category : inspect_result.inspect.categories) {
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
      const auto inspect_result = bills::io::InspectConfig(context_.config_dir);
      if (!inspect_result) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatError(inspect_result.error()) << '\n';
        return false;
      }
      std::cout << ToJson(*inspect_result).dump(2) << '\n';
      return true;
    }

    case ConfigAction::kFormats: {
      const auto formats = bills::io::ListEnabledExportFormats(context_.config_dir);
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
