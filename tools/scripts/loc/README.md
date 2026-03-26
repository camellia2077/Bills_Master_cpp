# LOC Scanner

统一代码行数扫描入口：

- `python scripts/loc/run.py`
- `scripts/loc/loc.bat`

## Windows BAT 快捷入口

在 Windows 下也可以直接用这些脚本：

- `scripts/loc/loc.bat`：通用透传入口，等价于 `python scripts/loc/run.py %*`
- `scripts/loc/scan_py_over.bat`：默认扫描 Python 大文件，等价于 `--lang py --over 200`
- `scripts/loc/scan_cpp_over.bat`：默认扫描 C++ 大文件，等价于 `--lang cpp --over 350`
- `scripts/loc/scan_kt_over.bat`：默认扫描 Kotlin 大文件，等价于 `--lang kt --over 180`
- `scripts/loc/scan_py_dir_over_files.bat`：默认扫描 Python 目录热点，等价于 `--lang py --dir-over-files --dir-max-depth 2`
- `scripts/loc/scan_cpp_dir_over_files.bat`：默认扫描 C++ 目录热点，等价于 `--lang cpp --dir-over-files --dir-max-depth 2`
- `scripts/loc/scan_kt_dir_over_files.bat`：默认扫描 Kotlin 目录热点，等价于 `--lang kt --dir-over-files --dir-max-depth 2`
- `scripts/loc/scan_bills_android_over.bat`：连续扫描 `apps/bills_android` 的 Kotlin / native C++ 大文件
- `scripts/loc/scan_bills_android_dir_over_files.bat`：连续扫描 `apps/bills_android` 的 Kotlin / native C++ 目录热点

示例：

```bat
scripts\loc\loc.bat --lang py tools tests --over 150
scripts\loc\scan_py_over.bat
scripts\loc\scan_kt_over.bat
scripts\loc\scan_py_over.bat tools
scripts\loc\scan_py_dir_over_files.bat scripts tools
scripts\loc\scan_bills_android_over.bat
```

## 基本用法

在仓库根目录执行：

```bash
python scripts/loc/run.py --lang <cpp|kt|py|rs> [paths ...] [--over N | --under [N] | --dir-over-files [N]] [--dir-max-depth N] [--log-file <path>]
```

参数说明：

- `--lang`：语言类型，必填，支持 `cpp` / `kt` / `py` / `rs`
- `paths`：可选，扫描目录列表；未传时使用 `scan_lines.toml` 中该语言的 `default_paths`
- `--over N`：扫描大文件
- `--under [N]`：扫描小文件；不传 `N` 时使用 TOML 中的 `default_under_threshold`
- `--dir-over-files [N]`：扫描目录内代码文件数超过 `N` 的目录；不传 `N` 时使用 TOML 中的 `default_dir_over_files`
- `--dir-max-depth N`：目录扫描最大深度，仅对 `--dir-over-files` 生效
- `-t/--threshold N`：兼容参数，等价于 `--over N`
- `--log-file`：自定义日志文件路径；相对路径相对 `scripts/loc/`
- `--config`：指定配置文件路径，默认是 `scripts/loc/scan_lines.toml`

## 当前默认配置

`bills_tracer` 当前已配置的默认扫描路径：

- `cpp` -> `apps/bills_cli/src`, `libs/core/src`, `libs/io/src`, `tests/generators/log_generator/src`
- `py` -> `tools`, `tests`, `scripts`
- `kt` -> `apps/bills_android/src/main/java`, `apps/bills_android/src/test/java`, `apps/bills_android/src/androidTest/java`

如果仓库后续新增 Kotlin / Rust 代码，可继续在 `scripts/loc/scan_lines.toml` 中补对应语言节。

当前目录热点扫描默认阈值：

- `cpp` -> `16`
- `py` -> `10`
- `kt` -> `8`

## 日志输出

每次执行都会写日志，默认输出到：

- `scripts/loc/logs/scan_cpp.json`
- `scripts/loc/logs/scan_py.json`
- `scripts/loc/logs/scan_kt.json`
- `scripts/loc/logs/scan_rs.json`

可通过 `--log-file` 覆盖，例如：

```bash
python scripts/loc/run.py --lang py --under 120 --log-file logs/loc_scan_py.json
```

## 常用命令

扫描 Python 大文件：

```bash
python scripts/loc/run.py --lang py --over 200
```

扫描 C++ 大文件：

```bash
python scripts/loc/run.py --lang cpp --over 350
```

扫描多个路径：

```bash
python scripts/loc/run.py --lang py tests tools --under 80
```

扫描目录热点：

```bash
python scripts/loc/run.py --lang py --dir-over-files --dir-max-depth 2
```

## 配置文件

配置文件：`scripts/loc/scan_lines.toml`

建议统一在这里维护：

- 默认扫描路径 `default_paths`
- 文件扩展名 `extensions`
- 忽略目录 `ignore_dirs`
- 忽略前缀 `ignore_prefixes`
- 默认阈值 `default_over_threshold` / `default_under_threshold`
- 目录热点阈值 `default_dir_over_files`
- 比较方式 `over_inclusive`
