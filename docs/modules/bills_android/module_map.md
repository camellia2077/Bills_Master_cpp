# bills_android Module Map

本页用于快速定位 `apps/bills_android` 的改动入口。

## 代码目录

- `apps/bills_android/build.gradle.kts`
  - Android Gradle 主入口；定义版本号、BuildConfig、CMake、签名、assets / notices 同步
- `apps/bills_android/src/main/java/com/billstracer/android/MainActivity.kt`
  - Activity 入口，注入 `BillsViewModel`
- `apps/bills_android/src/main/java/com/billstracer/android/ui/BillsApp.kt`
  - 顶层 Compose shell；管理 Data / Record / Report / Config 四个 section
- `apps/bills_android/src/main/java/com/billstracer/android/ui/BillsViewModel.kt`
  - UI 状态初始化、动作分发入口、repository 调用编排
- `apps/bills_android/src/main/java/com/billstracer/android/ui/app/`
  - 顶部 header、底部 section bar 等壳层 UI
- `apps/bills_android/src/main/java/com/billstracer/android/ui/common/`
  - 共用 scaffold、日期输入等 UI 基础件
- `apps/bills_android/src/main/java/com/billstracer/android/ui/data/`
  - 数据导入、导出、清理相关页面与 ViewModel action
- `apps/bills_android/src/main/java/com/billstracer/android/ui/record/`
  - 记录文件打开、编辑、预览、保存相关页面与 action
- `apps/bills_android/src/main/java/com/billstracer/android/ui/report/`
  - 查询页面、结果卡片、标准报表解析与展示
- `apps/bills_android/src/main/java/com/billstracer/android/ui/config/`
  - TOML 配置编辑、主题设置、About / notices 展示
- `apps/bills_android/src/main/java/com/billstracer/android/ui/theme/`
  - Material 主题、色板、字体
- `apps/bills_android/src/main/java/com/billstracer/android/ui/QueryResultDisplay.kt`
  - 查询结果展示的共用入口
- `apps/bills_android/src/main/java/com/billstracer/android/ui/MarkdownText.kt`
  - Markdown 文本渲染支持
- `apps/bills_android/src/main/java/com/billstracer/android/data/BillsNativeRepository.kt`
  - Android 仓库实现；负责工作区初始化、导入、查询、记录文件和配置写回
- `apps/bills_android/src/main/java/com/billstracer/android/data/AssetBundleManager.kt`
  - assets 解包、私有工作区目录准备、数据库清理
- `apps/bills_android/src/main/java/com/billstracer/android/data/ThemePreferenceStore.kt`
  - DataStore 主题偏好持久化
- `apps/bills_android/src/main/java/com/billstracer/android/data/BillsNativeBindings.kt`
  - Kotlin <-> JNI native 方法声明
- `apps/bills_android/src/main/java/com/billstracer/android/model/BillsModels.kt`
  - `BillsUiState` 与 Android 端模型集合
- `apps/bills_android/src/main/cpp/android_bridge.cpp`
  - JNI 具体实现；组装 `bills_core` ABI / `bills_io` 查询与模板能力
- `apps/bills_android/src/main/cpp/CMakeLists.txt`
  - native target、依赖与链接配置
- `apps/bills_android/src/test/`
  - JVM 单测；当前重点覆盖 `BillsViewModel`
- `apps/bills_android/src/androidTest/`
  - 仪器测试；当前重点覆盖 Activity、Compose 页面与 native repository

## 改动定位建议

- 改 Tab 切换、页面壳层、导航布局：先看 `ui/BillsApp.kt` + `ui/app/`
- 改导入样例、导出 TXT / config、清库行为：先看 `ui/data/` + `data/BillsNativeRepository.kt`
- 改记录文件编辑、模板生成、预览、保存：先看 `ui/record/` + `data/BillsNativeRepository.kt`
- 改查询参数、结果卡片、标准报表渲染：先看 `ui/report/` + `ui/QueryResultDisplay.kt`
- 改配置编辑、主题切换、About / notices：先看 `ui/config/` + `data/ThemePreferenceStore.kt`
- 改私有工作区、assets 同步、样例文件来源：先看 `data/AssetBundleManager.kt` + `build.gradle.kts`
- 改 JNI 方法签名、native JSON 协议、C++ 侧查询流程：先看 `data/BillsNativeBindings.kt` + `src/main/cpp/android_bridge.cpp`
- 改版本号、打包参数、签名或 APK 输出：先看 `build.gradle.kts`
