// windows/infrastructure/reports/plugins/common/plugin_export.hpp
#ifndef WINDOWS_INFRASTRUCTURE_REPORTS_PLUGINS_COMMON_PLUGIN_EXPORT_H_
#define WINDOWS_INFRASTRUCTURE_REPORTS_PLUGINS_COMMON_PLUGIN_EXPORT_H_

#ifdef _WIN32
#define REPORT_PLUGIN_EXPORT __declspec(dllexport)
#else
#define REPORT_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

#endif  // WINDOWS_INFRASTRUCTURE_REPORTS_PLUGINS_COMMON_PLUGIN_EXPORT_H_
