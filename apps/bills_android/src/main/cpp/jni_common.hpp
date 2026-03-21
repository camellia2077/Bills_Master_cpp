#ifndef APPS_BILLS_ANDROID_SRC_MAIN_CPP_JNI_COMMON_HPP_
#define APPS_BILLS_ANDROID_SRC_MAIN_CPP_JNI_COMMON_HPP_

#include <jni.h>

#include <exception>
#include <string>
#include <utility>

#include "nlohmann/json.hpp"

namespace bills::android::jni {

using Json = nlohmann::ordered_json;

[[nodiscard]] auto MakeResponse(bool ok, std::string code, std::string message,
                                Json data = Json::object()) -> std::string;

[[nodiscard]] auto FromJString(JNIEnv* env, jstring value) -> std::string;

[[nodiscard]] auto ToJString(JNIEnv* env, const std::string& value) -> jstring;

template <typename Fn>
auto SafeCall(JNIEnv* env, Fn&& callback) -> jstring {
  try {
    return ToJString(env, std::forward<Fn>(callback)());
  } catch (const std::exception& error) {
    return ToJString(
        env, MakeResponse(false, "system.native_failure", error.what()));
  }
}

}  // namespace bills::android::jni

#endif  // APPS_BILLS_ANDROID_SRC_MAIN_CPP_JNI_COMMON_HPP_
