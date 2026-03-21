# bills_android Architecture

`apps/bills_android` 是 Android 表现层 + 平台适配层，负责把 `bills_core` / `bills_io` 的能力包装成 Compose UI、JNI 桥接和 Android 私有工作区。

## 分层职责

- `src/main/java/com/billstracer/android/MainActivity.kt`
  - Activity 入口，创建 `BillsViewModel` 与 `BillsNativeRepository`
- `src/main/java/com/billstracer/android/ui/`
  - Compose 页面、状态渲染、用户动作入口
- `src/main/java/com/billstracer/android/data/`
  - 仓库实现、主题持久化、资产落盘、JNI 绑定
- `src/main/java/com/billstracer/android/model/BillsModels.kt`
  - UI 状态与 Android 端模型定义
- `src/main/cpp/android_bridge.cpp`
  - C++ JNI 桥接，调用 `bills_core` ABI、`bills_io` 仓库和报表能力
- `build.gradle.kts`
  - Compose / CMake / assets / notices / signing / APK 输出装配

## 运行时结构

- 私有工作区根目录：`context.noBackupFilesDir/bills_android`
- 关键子目录：
  - `bundled_assets/`
    - 每次初始化时重新展开 APK 内测试数据与 notices
  - `runtime_config/`
    - 运行时可修改的 TOML 配置，首次启动时从 assets 拷贝
  - `records/`
    - 用户保存的 `YYYY/YYYY-MM.txt`
  - `db/bills.sqlite3`
    - Android 私有 sqlite 数据库

## 构建时结构

- `build/generated/assets/main`
  - Gradle 在 `preBuild` 前同步的 Android assets
- `dist/config/android/`
  - 由 `tools/flows/distribute_configs.py --targets android` 生成的配置快照
- `dist/notices/android/`
  - 由 `tools/notices/generate_notices.py --targets android` 生成的 notices 快照
- `apps/bills_android/.cxx/`
  - Android native CMake staging

## 约束

- 核心账单规则、报表规则、ABI 契约不在此层定义
- Android 层可以做：
  - Compose 状态管理
  - DocumentFile / DataStore / asset / activity result 等平台集成
  - JNI 参数转换与 Android 本地错误提示
- Android 层不应该做：
  - 复制一份 `bills_core` 业务规则
  - 绕过 `BillsNativeBindings` 直接散落 native 协议
