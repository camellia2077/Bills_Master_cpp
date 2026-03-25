# bills_android Dist And Test

## 统一验证入口

- `python tools/run.py dist bills-tracer-android --preset debug`

## 底层 flow

- `python tools/run.py dist bills-tracer-android --preset debug`
- `python tools/flows/build_bills_tracer_android.py --preset debug`
- `python tools/flows/build_bills_tracer_android.py --variants debug,release`

`tools/run.py dist bills-tracer-android` 会转发到 `tools/flows/build_bills_tracer_android.py`，日常优先走统一入口。

## 定向建议

- 改 Compose / ViewModel / service 后，至少执行：
  - `python tools/run.py dist bills-tracer-android --preset debug`
- 改纯 Kotlin 状态逻辑，可追加：
  - `./gradlew :apps:bills_android:testDebugUnitTest`
- 改 Activity、Compose 页面、service / JNI 且本机有设备 / 模拟器，可追加：
  - `./gradlew :apps:bills_android:connectedDebugAndroidTest`
- 仅当用户明确要求，或问题只在发布链路出现时，再执行：
  - `python tools/run.py dist bills-tracer-android --preset release`

## 产物与排查入口

- APK 原始输出：
  - `apps/bills_android/build/outputs/apk/debug/bills_android-debug.apk`
  - `apps/bills_android/build/outputs/apk/release/bills_android-release.apk`
- 默认归档输出：
  - `dist/gradle/apps/bills_android/apk/debug/`
  - `dist/gradle/apps/bills_android/apk/release/`
- 构建期 assets：
  - `apps/bills_android/build/generated/assets/main/`
- native CMake staging：
  - `apps/bills_android/.cxx/`

## 相关说明

- Android APK 编译细节、签名与归档说明见：`docs/notes/android_apk_build.md`
- `build.gradle.kts` 会在 `preBuild` 前同步：
  - `testdata/bills/<sample>/`
  - `dist/config/android/`
  - `dist/notices/android/`
