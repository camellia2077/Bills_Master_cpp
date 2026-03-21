# Android APK 编译说明

本文档说明当前仓库里推荐的 Android APK 编译命令，以及统一入口和底层脚本各自的职责。

## 1. 推荐命令

优先使用统一入口：

- debug APK：
  - `python tools/run.py dist android --preset debug`
- release APK：
  - `python tools/run.py dist android --preset release`

这条命令适合日常单变体编译，也更符合仓库当前“统一入口优先”的工具链约定。

## 2. 底层脚本

如果你需要一次编译多个变体，或者想直接使用 Android flow 脚本，可运行：

- 默认只编译 debug：
  - `python tools/flows/build_bills_android.py`
- 只编译 debug：
  - `python tools/flows/build_bills_android.py --variants debug`
- 只编译 release：
  - `python tools/flows/build_bills_android.py --variants release`
- 同时编译 debug + release：
  - `python tools/flows/build_bills_android.py --variants debug,release`
- 同时编译两个变体并先 clean：
  - `python tools/flows/build_bills_android.py --variants debug,release --clean`

统一入口和底层脚本是两层关系：

- `python tools/run.py dist android --preset ...`
  - 负责提供仓库统一的命令入口
- `python tools/flows/build_bills_android.py`
  - 负责实际执行 Gradle、签名准备和 APK 复制

## 3. 常用附加参数

- `--clean`
  - 先执行 Gradle clean 再编译
- `--skip_copy`
  - 只保留 Gradle 原始输出，不复制到 `dist/gradle/apps/bills_android/apk/`

示例：

- `python tools/flows/build_bills_android.py`
- `python tools/run.py dist android --preset release --clean`
- `python tools/flows/build_bills_android.py --variants debug,release --skip_copy`

## 4. 产物位置

Gradle 原始输出：

- `apps/bills_android/build/outputs/apk/debug/bills_android-debug.apk`
- `apps/bills_android/build/outputs/apk/release/bills_android-release.apk`

默认复制后的归档位置：

- `dist/gradle/apps/bills_android/apk/debug/`
- `dist/gradle/apps/bills_android/apk/release/`

复制后的文件名会附带 Android 展示版本号，例如：

- `dist/gradle/apps/bills_android/apk/debug/bills_android-debug-v<version>.apk`

## 5. release 说明

当编译 release APK 时，脚本会自动检查并生成本地 keystore；默认放在：

- `dist/gradle/apps/bills_android/signing/bills-android-release.keystore`

这意味着本仓库当前的 release 构建流程是“可本地跑通的发布候选包构建”，不需要你手工先准备一套 keystore 参数。
