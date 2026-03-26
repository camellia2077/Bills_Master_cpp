export module bill.core.build_info;

export namespace bills::core::build_info {
inline constexpr const char* kLibraryName = "bills_core";
inline constexpr const char* kPilotModuleName = "bill.core.build_info";

constexpr auto module_pilot_enabled() noexcept -> bool {
  return true;
}
}  // namespace bills::core::build_info

