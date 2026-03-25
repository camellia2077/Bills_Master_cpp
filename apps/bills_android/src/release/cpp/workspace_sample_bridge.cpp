#include <jni.h>

#include <string>

#include "jni_common.hpp"

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_BundledSampleNativeBindings_importBundledSampleNative(
    JNIEnv* env, jclass, jstring, jstring, jstring) {
  return bills::android::jni::SafeCall(env, []() -> std::string {
    return bills::android::jni::MakeResponse(
        false, "business.unsupported",
        "Bundled sample import is unavailable in release builds.");
  });
}
