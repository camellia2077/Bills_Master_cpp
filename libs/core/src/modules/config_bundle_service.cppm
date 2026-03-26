module;
#include "config/config_bundle_service.hpp"

export module bill.core.config.bundle_service;

export namespace bills::core::modules::config {
using ConfigFileValidationReport = ::ConfigFileValidationReport;
using ConfigBundleValidationReport = ::ConfigBundleValidationReport;
using RuntimeConfigBundle = ::RuntimeConfigBundle;
using ValidatedConfigBundle = ::ValidatedConfigBundle;
using ConfigBundleService = ::ConfigBundleService;
}
