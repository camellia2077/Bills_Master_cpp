# bills_android Docs

这是 `apps/bills_android` 的文档落地页，目标是让人或 agent 在进入模块后，先用一页确定“先读什么、改哪里、怎么验证”。

## 推荐阅读顺序

1. `apps/bills_android/README.md`
2. `apps/bills_android/AGENTS.md`
3. `docs/modules/bills_android/module_map.md`
4. `docs/modules/bills_android/change_guide.md`
5. `docs/modules/bills_android/dist_and_test.md`
6. 按需要继续看 `docs/modules/bills_android/architecture.md`

## 常用入口

- 代码定位：`docs/modules/bills_android/module_map.md`
- 改动落点：`docs/modules/bills_android/change_guide.md`
- 架构说明：`docs/modules/bills_android/architecture.md`
- 构建与验证：`docs/modules/bills_android/dist_and_test.md`
- Android APK 说明：`docs/notes/android_apk_build.md`

## 遇到这些需求先看哪里

- 改 Compose 页面、Tab 切换、ViewModel 动作入口：
  - `docs/modules/bills_android/module_map.md`
- 改导入、记录编辑、查询报表、配置编辑：
  - `docs/modules/bills_android/change_guide.md`
- 改 JNI / native 桥接或 CMake：
  - `docs/modules/bills_android/module_map.md`
  - `docs/modules/bills_android/architecture.md`
- 改 APK 打包、签名、资产同步、notices：
  - `docs/modules/bills_android/dist_and_test.md`
  - `docs/notes/android_apk_build.md`

## 边界提醒

- Android 端负责表现层、平台交互、私有工作区与 JNI 桥接
- 业务规则不要落在 `apps/bills_android`
- 如需改核心规则，转到 `docs/modules/bills_core/`
- 如需改跨端 IO / sqlite / config provider，优先看 `docs/modules/bills_io/`
