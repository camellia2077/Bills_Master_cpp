# bills_android Agent Guide

`apps/bills_android` 目录只保留入口约束，详细说明统一在 `docs/modules/bills_android/`。

## 必读顺序

1. `apps/bills_android/README.md`
2. `docs/modules/bills_android/README.md`
3. `docs/modules/bills_android/module_map.md`
4. `docs/modules/bills_android/change_guide.md`
5. `docs/modules/bills_android/dist_and_test.md`

## 约束

- `bills_android` 是 Android 表现层 + 平台适配层，不新增核心业务规则
- 账单规则、报表规则、ABI 契约优先复用 `libs/bills_core`
- 跨端 IO 能力优先复用 `libs/bills_io`；Android 目录只补平台桥接、资产同步、私有工作区管理
- 修改 JNI / native 桥接时，至少同步检查：
  - `apps/bills_android/src/main/java/com/billstracer/android/data/native/`
  - `apps/bills_android/src/main/cpp/jni_common.*`
  - `apps/bills_android/src/main/cpp/*_bridge.cpp`

## 验证

- 修改后优先执行：`python tools/run.py dist android --preset debug`
- 需要直接调用底层 flow 时可执行：`python tools/flows/build_bills_android.py --preset debug`
- 仅当用户明确要求，或问题仅在发布链路复现时，才主动编译 `release`
- 仅当本次改动为纯文档修改（`docs/**`、`*.md`）时，可跳过验证
