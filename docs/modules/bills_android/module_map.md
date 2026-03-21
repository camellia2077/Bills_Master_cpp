# bills_android Module Map

本页用于快速定位 `apps/bills_android` 的改动入口。

## 代码目录

- `apps/bills_android/build.gradle.kts`
  - Android Gradle 主入口；定义版本号、CMake、签名、assets / notices 同步
- `apps/bills_android/src/main/java/com/billstracer/android/MainActivity.kt`
  - Activity 入口；装配 services 和 5 个 ViewModel
- `apps/bills_android/src/main/java/com/billstracer/android/app/navigation/`
  - 底部四 Tab 壳、session bus、session ViewModel
- `apps/bills_android/src/main/java/com/billstracer/android/app/theme/`
  - Material 主题、色板、字体
- `apps/bills_android/src/main/java/com/billstracer/android/features/workspace/`
  - import / export / clear
- `apps/bills_android/src/main/java/com/billstracer/android/features/query/`
  - year/month query、报表展示
- `apps/bills_android/src/main/java/com/billstracer/android/features/editor/`
  - 打开、编辑、预览、保存、列 period
- `apps/bills_android/src/main/java/com/billstracer/android/features/settings/`
  - config、theme、about、version
- `apps/bills_android/src/main/java/com/billstracer/android/data/runtime/`
  - 私有工作区和 assets 落盘
- `apps/bills_android/src/main/java/com/billstracer/android/data/native/`
  - task-scoped native bindings
- `apps/bills_android/src/main/java/com/billstracer/android/data/services/`
  - `WorkspaceService / QueryService / EditorService / SettingsService`
- `apps/bills_android/src/main/java/com/billstracer/android/data/prefs/`
  - `ThemePreferenceStore`
- `apps/bills_android/src/main/java/com/billstracer/android/platform/`
  - 跨 feature 复用的 Compose 基础件
- `apps/bills_android/src/main/cpp/jni_common.cpp`
  - JNI 公共 envelope / error / string bridge
- `apps/bills_android/src/main/cpp/workspace_bridge.cpp`
  - workspace native bridge
- `apps/bills_android/src/main/cpp/query_bridge.cpp`
  - query native bridge
- `apps/bills_android/src/main/cpp/editor_bridge.cpp`
  - editor native bridge
- `apps/bills_android/src/main/cpp/settings_bridge.cpp`
  - settings native bridge
- `apps/bills_android/src/main/cpp/CMakeLists.txt`
  - native target、依赖与链接配置
- `apps/bills_android/src/test/`
  - JVM 单测；按 `AppSession / Workspace / Query / Editor / Settings` 分域
- `apps/bills_android/src/androidTest/`
  - 仪器测试；按导航、feature 页面、service 分域

## 改动定位建议

- 改底部 Tab 和顶层壳：先看 `app/navigation/`
- 改导入/导出/清理：先看 `features/workspace/` + `data/services/DefaultWorkspaceService.kt`
- 改记录编辑链路：先看 `features/editor/` + `data/services/DefaultEditorService.kt`
- 改查询和报表展示：先看 `features/query/` + `data/services/DefaultQueryService.kt`
- 改配置、主题、notices、version：先看 `features/settings/` + `data/services/DefaultSettingsService.kt`
- 改工作区/资产落盘：先看 `data/runtime/`
- 改 JNI 协议：先看 `data/native/` + `src/main/cpp/*_bridge.cpp`
- 改版本号、打包参数、签名或 APK 输出：先看 `build.gradle.kts`
