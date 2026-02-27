#ifndef PLUGIN_EXPORT_HPP
#define PLUGIN_EXPORT_HPP

#ifdef _WIN32
#define REPORT_PLUGIN_EXPORT __declspec(dllexport)
#else
#define REPORT_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

#endif  // PLUGIN_EXPORT_HPP
