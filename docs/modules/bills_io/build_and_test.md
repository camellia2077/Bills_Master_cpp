# bills_io Build And Test

## 统一验证入口

- `python tools/verify/verify.py`

## 定向建议

- 变更 `io_factory` / 适配器后，至少执行：
  - `python tools/verify/verify.py bills`
- 变更分层边界后，追加执行：
  - `python tools/verify/verify.py import-layer-check --stats`
  - `python tools/verify/verify.py boundary-layer-check --stats`

## 失败排查

- 先看 `tests/output/artifact/<project>/test_summary.json`
- 再看 `tests/output/artifact/<project>/logs/*.log`
