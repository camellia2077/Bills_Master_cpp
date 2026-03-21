# bills_android

`apps/bills_android` 只承接 Android 表现层、平台适配和 JNI 宿主装配。详细说明统一放在 `docs/modules/bills_android/`。

## Start Here

1. `apps/bills_android/AGENTS.md`
2. `docs/modules/bills_android/module_map.md`
3. `docs/modules/bills_android/change_guide.md`
4. `docs/modules/bills_android/dist_and_test.md`

## Quick Pointers

- 找目录和主线：`docs/modules/bills_android/module_map.md`
- 判断改动落点：`docs/modules/bills_android/change_guide.md`
- 看整体边界：`docs/modules/bills_android/architecture.md`

## Quick Verify

- `python tools/run.py dist android --preset debug`
