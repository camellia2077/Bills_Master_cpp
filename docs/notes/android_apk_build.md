# Android APK 编译说明

本文档说明当前仓库里推荐的 Android APK 编译命令。

## 1. 推荐命令

统一使用 `python tools/run.py`：

- debug APK：
  - `python tools/run.py dist bills-tracer-android --preset debug`
- release APK：
  - `python tools/run.py dist bills-tracer-android --preset release`

常用附加参数：

- `--clean`
  - 先执行 Gradle clean 再编译

示例：

- `python tools/run.py dist bills-tracer-android --preset release --clean`

## 2. 产物位置

Gradle 原始输出：

- `apps/bills_android/build/outputs/apk/debug/bills_android-debug.apk`
- `apps/bills_android/build/outputs/apk/release/bills_android-release.apk`

默认复制后的归档位置：

- `dist/gradle/apps/bills_android/apk/debug/`
- `dist/gradle/apps/bills_android/apk/release/`

复制后的文件名会附带 Android 展示版本号，例如：

- `dist/gradle/apps/bills_android/apk/debug/bills_android-debug-v<version>.apk`

## 3. release 说明

当编译 release APK 时，脚本会自动检查并生成本地 keystore；默认放在：

- `dist/gradle/apps/bills_android/signing/bills-android-release.keystore`

这意味着本仓库当前的 release 构建流程是“可本地跑通的发布候选包构建”，不需要你手工先准备一套 keystore 参数。
