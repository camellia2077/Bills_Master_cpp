# bills_android Architecture

`apps/bills_android` 现在是“单 Activity + 底部四 Tab + feature 页面”结构。依赖方向固定为：

- `MainActivity`
  - 手工装配 `WorkspaceService / QueryService / EditorService / SettingsService`
  - 手工装配 `AppSessionViewModel` 和 4 个 feature ViewModel
  - 只负责挂载 Compose root
- `app/navigation`
  - 底部四 Tab 壳和全局 session 共享状态
- `features/workspace|query|editor|settings`
  - 每个 feature 自己持有 `UiState + ViewModel + Screen`
- `data/runtime`
  - 私有工作区准备和 assets 解包
- `data/prefs`
  - Theme DataStore
- `data/native`
  - Kotlin `external` 声明和 native JSON 辅助
- `data/services`
  - feature service 接口与实现
- `platform`
  - 真正跨 feature 复用的 Compose 基础件
- `src/main/cpp/jni_common.cpp` + `workspace_bridge.cpp` + `query_bridge.cpp` + `editor_bridge.cpp` + `settings_bridge.cpp`
  - 按任务域拆开的 JNI 桥接

## 运行时结构

- 私有工作区根目录：`context.noBackupFilesDir/bills_android`
- 关键子目录：
  - `bundled_assets/`
  - `runtime_config/`
  - `records/`
  - `db/bills.sqlite3`

## 约束

- 不再保留大而全的 `BillsViewModel / BillsRepository / android_bridge.cpp`
- 顶层交互入口恢复成 4 个底部 Tab：
  - `Workspace`
  - `Editor`
  - `Query`
  - `Settings`
- 原来 `Home` 承担的环境摘要、版本和最近状态消息收进 `Workspace` 顶部
- UI 只依赖 feature ViewModel，不直接粘住底层 native 或 runtime 对象
- JNI 只负责参数转换、response envelope 和 task 域调用
- 业务规则仍然只在 `libs/core` / `libs/io`
