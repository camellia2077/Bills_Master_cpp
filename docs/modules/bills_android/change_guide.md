# bills_android Change Guide

本页用于让 agent 快速判断 `apps/bills_android` 要改哪些文件。

## 常见改动定位

| 场景 | 先改文件 | 联动文件 |
| --- | --- | --- |
| 调整底部 Tab、默认页、顶层壳 | `apps/bills_android/src/main/java/com/billstracer/android/app/navigation/AppNavigation.kt` | `MainActivity.kt` |
| 调整 session 初始化、全局消息、主题同步 | `apps/bills_android/src/main/java/com/billstracer/android/app/navigation/AppSessionViewModel.kt` | `AppSessionBus.kt` |
| 调整导入样例、导出记录文件、清空数据库 | `apps/bills_android/src/main/java/com/billstracer/android/features/workspace/WorkspaceViewModel.kt` | `WorkspaceScreen.kt`, `data/services/DefaultWorkspaceService.kt` |
| 调整记录文件打开、模板生成、预览、保存 | `apps/bills_android/src/main/java/com/billstracer/android/features/editor/EditorViewModel.kt` | `EditorScreen.kt`, `data/services/DefaultEditorService.kt` |
| 调整年/月查询、结果卡片、标准报表展示 | `apps/bills_android/src/main/java/com/billstracer/android/features/query/QueryViewModel.kt` | `QueryScreen.kt`, `QueryResultBlocks.kt`, `data/services/DefaultQueryService.kt` |
| 调整 TOML 配置编辑、主题设置、About 页面 | `apps/bills_android/src/main/java/com/billstracer/android/features/settings/SettingsViewModel.kt` | `SettingsScreen.kt`, `SettingsBlocks.kt`, `data/services/DefaultSettingsService.kt` |
| 调整 APK 内 bundled config / notices / testdata 同步 | `apps/bills_android/build.gradle.kts` | `data/runtime/AssetBundleManager.kt`, `docs/notes/android_apk_build.md` |
| 调整 Kotlin <-> C++ JNI 方法、native JSON 返回结构 | `apps/bills_android/src/main/java/com/billstracer/android/data/native/*.kt` | `src/main/cpp/*_bridge.cpp`, `src/main/cpp/jni_common.*`, `src/main/cpp/CMakeLists.txt` |
| 调整 Android 展示版本号、BuildConfig 常量、签名 / 产物归档 | `apps/bills_android/build.gradle.kts` | `docs/release/clients/android/` |

## 变更边界提醒

- Compose 页面只做状态渲染和用户交互，不复制核心业务规则。
- feature ViewModel 只承接一个任务域，不跨域混合。
- service 层负责 Android 平台适配；不要再回到“大而全 repository”。
- JNI 只做参数转换、response envelope 和 task 域调用；规则改动仍应回到 `libs/bills_core` 或 `libs/io`。

## 提交前最小检查

1. 纯构建验证：`python tools/run.py dist bills-tracer-android --preset debug`
2. 改 `ViewModel` / 纯 Kotlin 逻辑后，可追加：`./gradlew :apps:bills_android:testDebugUnitTest`
3. 改 Compose 页面或 native repository，且本机有设备 / 模拟器时，可追加：`./gradlew :apps:bills_android:connectedDebugAndroidTest`
