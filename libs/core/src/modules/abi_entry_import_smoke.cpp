import bill.core.abi.entry;

namespace {
[[maybe_unused]] auto* kAbiVersionEntry = &bills_core_get_abi_version;
[[maybe_unused]] auto* kCapabilitiesEntry = &bills_core_get_capabilities_json;
[[maybe_unused]] auto* kInvokeEntry = &bills_core_invoke_json;
[[maybe_unused]] auto* kFreeEntry = &bills_core_free_string;
}  // namespace
