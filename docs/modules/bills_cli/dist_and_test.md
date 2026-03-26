# bills_cli Dist And Test

## 统一验证入口

- `python tools/run.py`

## 定向建议

- 改 CLI parser/feature handler 后，至少执行：
  - `python tools/run.py verify bills-tracer`
- 改 dist/目录结构后，追加执行：
  - `python tools/run.py dist bills-tracer-cli --preset debug --scope shared`

## 失败排查

- 查看 `dist/tests/artifact/<project>/latest/test_summary.json`
- 查看 `dist/tests/artifact/<project>/latest/logs/` 细分步骤日志
