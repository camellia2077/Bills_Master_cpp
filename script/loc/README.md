# LOC Scanner

统一代码行数扫描入口：

- `python script/loc/run.py`

## 基本用法

在仓库根目录执行：

```bash
python script/loc/run.py --lang <cpp|kt|py|rs> [paths ...] [--over N | --under [N] | --dir-over-files [N]] [--dir-max-depth N] [--log-file <path>]
```

参数说明：

- `--lang`：语言类型，必填，支持 `cpp` / `kt` / `py` / `rs`
- `paths`：可选，扫描目录列表；未传时使用 `scan_lines.toml` 中该语言的 `default_paths`
- `--over N`：扫描大文件
- `--under [N]`：扫描小文件；不传 `N` 时使用 TOML 中的 `default_under_threshold`
- `--dir-over-files [N]`：扫描目录内代码文件数超过 `N` 的目录；不传 `N` 时使用 TOML 中的 `default_dir_over_files`
- `--dir-max-depth N`：目录扫描最大深度，仅对 `--dir-over-files` 生效
- `-t/--threshold N`：兼容参数，等价于 `--over N`
- `--log-file`：自定义日志文件路径；相对路径相对 `script/loc/`
- `--config`：指定配置文件路径，默认是 `script/loc/scan_lines.toml`

## 当前默认配置

`bills_tracer` 当前已配置的默认扫描路径：

- `cpp` -> `apps/bills_cli/src`, `libs/bills_core/src`, `libs/bills_io/src`, `tests/generators/log_generator/src`
- `py` -> `tools`, `tests`, `script`

如果仓库后续新增 Kotlin / Rust 代码，可继续在 `script/loc/scan_lines.toml` 中补对应语言节。

当前目录热点扫描默认阈值：

- `cpp` -> `16`
- `py` -> `10`

## 日志输出

每次执行都会写日志，默认输出到：

- `script/loc/logs/scan_cpp.json`
- `script/loc/logs/scan_py.json`
- `script/loc/logs/scan_kt.json`
- `script/loc/logs/scan_rs.json`

可通过 `--log-file` 覆盖，例如：

```bash
python script/loc/run.py --lang py --under 120 --log-file logs/loc_scan_py.json
```

## 常用命令

扫描 Python 大文件：

```bash
python script/loc/run.py --lang py --over 200
```

扫描 C++ 大文件：

```bash
python script/loc/run.py --lang cpp --over 350
```

扫描多个路径：

```bash
python script/loc/run.py --lang py tests tools --under 80
```

扫描目录热点：

```bash
python script/loc/run.py --lang py --dir-over-files --dir-max-depth 2
```

## 配置文件

配置文件：`script/loc/scan_lines.toml`

建议统一在这里维护：

- 默认扫描路径 `default_paths`
- 文件扩展名 `extensions`
- 忽略目录 `ignore_dirs`
- 忽略前缀 `ignore_prefixes`
- 默认阈值 `default_over_threshold` / `default_under_threshold`
- 目录热点阈值 `default_dir_over_files`
- 比较方式 `over_inclusive`
