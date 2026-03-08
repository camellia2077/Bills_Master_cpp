# bills_cli Dist And Test

## 统一验证入口

- `python tools/verify/verify.py`

## 定向建议

- 改 CLI 命令/控制器后，至少执行：
  - `python tools/verify/verify.py bills`
- 改 dist/目录结构后，追加执行：
  - `python tools/verify/verify.py bills-dist`

## 失败排查

- 查看 `dist/tests/artifact/<project>/latest/test_summary.json`
- 查看 `dist/tests/artifact/<project>/latest/logs/` 细分步骤日志
