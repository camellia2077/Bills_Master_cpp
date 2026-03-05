import bill.core.build_info;

namespace {
[[maybe_unused]] constexpr bool kModulePilotEnabled =
    bills::core::build_info::module_pilot_enabled();

[[maybe_unused]] constexpr const char* kModulePilotLibraryName =
    bills::core::build_info::kLibraryName;
}  // namespace

