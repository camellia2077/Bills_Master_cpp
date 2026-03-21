#include "jni_common.hpp"

namespace bills::android::jni {

auto MakeResponse(bool ok, std::string code, std::string message, Json data)
    -> std::string {
  Json response;
  response["ok"] = ok;
  response["code"] = std::move(code);
  response["message"] = std::move(message);
  response["data"] = std::move(data);
  return response.dump(2);
}

auto FromJString(JNIEnv* env, jstring value) -> std::string {
  if (value == nullptr) {
    return {};
  }
  const char* raw = env->GetStringUTFChars(value, nullptr);
  if (raw == nullptr) {
    return {};
  }
  std::string text(raw);
  env->ReleaseStringUTFChars(value, raw);
  return text;
}

auto ToJString(JNIEnv* env, const std::string& value) -> jstring {
  return env->NewStringUTF(value.c_str());
}

}  // namespace bills::android::jni
