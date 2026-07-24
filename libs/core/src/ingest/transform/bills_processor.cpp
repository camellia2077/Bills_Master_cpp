// ingest/transform/bills_processor.cpp

#include "bills_processor.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
}  // namespace

BillProcessor::BillProcessor(const Config& config) : m_config(config) {}

void BillProcessor::process(std::vector<std::string>& lines) {
  _apply_auto_renewal(lines);
}

void BillProcessor::_apply_auto_renewal(std::vector<std::string>& lines) {
  if (!m_config.auto_renewal.enabled) {
    return;
  }

  for (const auto& rule : m_config.auto_renewal.rules) {
    const std::string& category_title = rule.header_location;

    auto category_it = std::find(lines.begin(), lines.end(), category_title);
    if (category_it == lines.end()) {
      continue;
    }

    auto content_start_it = category_it + 1;
    auto content_end_it = content_start_it;
    while (content_end_it != lines.end() && !_is_title(*content_end_it) &&
           !content_end_it->empty()) {
      ++content_end_it;
    }

    bool found = false;
    for (auto it = content_start_it; it != content_end_it; ++it) {
      if (it->find(rule.description) != std::string::npos) {
        found = true;
        break;
      }
    }

    if (!found) {
      std::stringstream line_stream;
      line_stream << std::fixed << std::setprecision(2) << rule.amount << " "
                  << rule.description << "(auto-renewal)";
      lines.insert(content_end_it, line_stream.str());
    }
  }
}

auto BillProcessor::_is_title(const std::string& line) -> bool {
  if (line.empty()) {
    return false;
  }
  for (char character : line) {
    if (isspace(character) == 0) {
      return isdigit(character) == 0;
    }
  }
  return false;
}
