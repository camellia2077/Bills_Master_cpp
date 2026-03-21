#include <jni.h>

#include <string>

#include "common/version.hpp"
#include "jni_common.hpp"

namespace {

using bills::android::jni::Json;

auto core_version_info() -> std::string {
  Json data;
  data["version_name"] = std::string(bills::core::version::kVersion);
  data["last_updated"] = std::string(bills::core::version::kLastUpdated);
  return bills::android::jni::MakeResponse(
      true, "ok", "Core version loaded successfully.", std::move(data));
}

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_SettingsNativeBindings_coreVersionNative(
    JNIEnv* env, jclass) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return core_version_info();
  });
}
