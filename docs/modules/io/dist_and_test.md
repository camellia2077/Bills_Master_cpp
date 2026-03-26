# io Dist And Test

## 统一验证入口

- `python tools/run.py`

## 定向建议

- 变更 `io_factory` / 适配器后，至少执行：
  - `python tools/run.py verify bills-tracer`
- 变更分层边界后，追加执行：
  - `python tools/run.py verify import-layer-check -- --stats`
  - `python tools/run.py verify boundary-layer-check -- --stats`

## 失败排查

- 先看 `dist/tests/artifact/<project>/latest/test_summary.json`
- 再看 `dist/tests/artifact/<project>/latest/logs/*.log`
