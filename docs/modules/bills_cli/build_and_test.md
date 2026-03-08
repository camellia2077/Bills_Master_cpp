# bills_cli Build And Test

## 统一验证入口

- `python tools/verify/verify.py`

## 定向建议

- 改 CLI 命令/控制器后，至少执行：
  - `python tools/verify/verify.py bills`
- 改构建/目录结构后，追加执行：
  - `python tools/verify/verify.py bills-build`

## 失败排查

- 查看 `build/tests/artifact/<project>/latest/test_summary.json`
- 查看 `build/tests/artifact/<project>/latest/logs/` 细分步骤日志
