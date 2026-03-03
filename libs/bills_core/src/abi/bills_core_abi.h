#ifndef BILLS_CORE_ABI_H
#define BILLS_CORE_ABI_H

#if defined(_WIN32) && defined(BILLS_CORE_SHARED)
#if defined(BILLS_CORE_ABI_BUILD)
#define BILLS_CORE_ABI_EXPORT __declspec(dllexport)
#else
#define BILLS_CORE_ABI_EXPORT __declspec(dllimport)
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#define BILLS_CORE_ABI_EXPORT __attribute__((visibility("default")))
#else
#define BILLS_CORE_ABI_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

BILLS_CORE_ABI_EXPORT const char* bills_core_get_abi_version();
BILLS_CORE_ABI_EXPORT const char* bills_core_get_capabilities_json();
BILLS_CORE_ABI_EXPORT const char* bills_core_invoke_json(
    const char* request_json_utf8);
BILLS_CORE_ABI_EXPORT void bills_core_free_string(const char* owned_utf8_str);

#ifdef __cplusplus
}
#endif

#endif  // BILLS_CORE_ABI_H
