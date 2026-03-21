# bills_android Change Guide

本页用于让 agent 快速判断 `apps/bills_android` 要改哪些文件。

## 常见改动定位

| 场景 | 先改文件 | 联动文件 |
| --- | --- | --- |
| 调整首页 section / scaffold / 页面切换 | `apps/bills_android/src/main/java/com/billstracer/android/ui/BillsApp.kt` | `ui/app/*.kt` |
| 调整初始化流程、状态字段、动作入口 | `apps/bills_android/src/main/java/com/billstracer/android/ui/BillsViewModel.kt` | `model/BillsModels.kt` |
| 调整导入 bundled sample、导出记录文件、清空数据库 | `apps/bills_android/src/main/java/com/billstracer/android/ui/data/BillsViewModelDataActions.kt` | `ui/data/DataPane.kt`, `data/BillsNativeRepository.kt` |
| 调整记录文件打开、模板生成、预览、保存 | `apps/bills_android/src/main/java/com/billstracer/android/ui/record/BillsViewModelRecordActions.kt` | `ui/record/*.kt`, `data/BillsNativeRepository.kt` |
| 调整年/月查询、结果卡片、标准报表展示 | `apps/bills_android/src/main/java/com/billstracer/android/ui/report/BillsViewModelReportActions.kt` | `ui/report/*.kt`, `ui/QueryResultDisplay.kt` |
| 调整 TOML 配置编辑、主题设置、About 页面 | `apps/bills_android/src/main/java/com/billstracer/android/ui/config/BillsViewModelConfigActions.kt` | `ui/config/*.kt`, `data/ThemePreferenceStore.kt`, `ui/theme/*.kt` |
| 调整 APK 内 bundled config / notices / testdata 同步 | `apps/bills_android/build.gradle.kts` | `data/AssetBundleManager.kt`, `docs/notes/android_apk_build.md` |
| 调整 Kotlin <-> C++ JNI 方法、native JSON 返回结构 | `apps/bills_android/src/main/java/com/billstracer/android/data/BillsNativeBindings.kt` | `src/main/cpp/android_bridge.cpp`, `src/main/cpp/CMakeLists.txt` |
| 调整 Android 展示版本号、BuildConfig 常量、签名 / 产物归档 | `apps/bills_android/build.gradle.kts` | `docs/release/clients/android/` |

## 变更边界提醒

- Compose 页面只做状态渲染和用户交互，不复制核心业务规则。
- Android repository 可以处理 Android 私有目录、DocumentFile、DataStore、JNI 调用，但不要把跨端规则固化在这里。
- `android_bridge.cpp` 主要做 JNI 参数转换、JSON 响应封装和底层能力拼装；如改到规则本身，通常应该回到 `libs/bills_core` 或 `libs/bills_io`。

## 提交前最小检查

1. 纯构建验证：`python tools/run.py dist android --preset debug`
2. 改 `ViewModel` / 纯 Kotlin 逻辑后，可追加：`./gradlew :apps:bills_android:testDebugUnitTest`
3. 改 Compose 页面或 native repository，且本机有设备 / 模拟器时，可追加：`./gradlew :apps:bills_android:connectedDebugAndroidTest`
