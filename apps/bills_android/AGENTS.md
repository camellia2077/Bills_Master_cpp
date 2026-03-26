# bills_android Agent Guide

`apps/bills_android` 是 Android 表现层与平台适配层。这个文件只保留 agent 需要的最小导航和约束。

## Read First

1. `apps/bills_android/README.md`
2. `docs/modules/bills_android/module_map.md`
3. `docs/modules/bills_android/change_guide.md`
4. 需要看整体边界时再看 `docs/modules/bills_android/architecture.md`

## Use This Module When

- 你在改 Android 顶层导航、feature 页面或 feature ViewModel
- 你在改 Android 私有工作区、DataStore、DocumentFile 或 assets 同步
- 你在改 Android 的 Kotlin/native 桥接

## Boundaries

- 这里不定义核心业务规则
- 业务规则和报表规则优先去 `libs/bills_core`
- 跨端 IO、sqlite、配置文档读取优先去 `libs/io`
- 改 JNI 时，先看：
  - `apps/bills_android/src/main/java/com/billstracer/android/data/native/`
  - `apps/bills_android/src/main/cpp/jni_common.*`
  - `apps/bills_android/src/main/cpp/*_bridge.cpp`

## Verify

- `python tools/run.py dist android --preset debug`
- 纯 Kotlin 逻辑可补：`./gradlew.bat :apps:bills_android:testDebugUnitTest`
- 有设备/模拟器时可补：`./gradlew.bat :apps:bills_android:connectedDebugAndroidTest`
- 仅文档改动可跳过代码验证
