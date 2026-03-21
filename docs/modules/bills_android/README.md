# bills_android Docs

这是 `apps/bills_android` 的文档入口页。当前事实源已经切到“底部四 Tab + 按 feature 的 ViewModel 和窄 service”。

## 推荐阅读顺序

1. `apps/bills_android/AGENTS.md`
2. `docs/modules/bills_android/architecture.md`
3. `docs/modules/bills_android/module_map.md`
4. `docs/modules/bills_android/change_guide.md`
5. `docs/modules/bills_android/dist_and_test.md`

## 当前主线

- `MainActivity` 只做 service / ViewModel 装配和 Compose root 挂载
- `app/navigation` 管底部四 Tab 壳和 session 共享状态
- `features/workspace|query|editor|settings` 各自持有 `UiState + ViewModel + Screen`
- `data/runtime|native|services|prefs` 承接 Android 平台适配
- `platform` 只放真正跨 feature 复用的 Compose 基础件

## 边界提醒

- Android 端负责表现层、导航、私有工作区、DataStore、DocumentFile 和 JNI 参数桥接
- 业务规则不要落在 `apps/bills_android`
- 核心规则改动转到 `docs/modules/bills_core/`
- 跨端 IO / sqlite / config 文档读取优先看 `docs/modules/bills_io/`
