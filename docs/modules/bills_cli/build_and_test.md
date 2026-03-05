# bills_cli Build And Test

## 统一验证入口

- `python tools/verify/verify.py`

## 定向建议

- 改 CLI 命令/控制器后，至少执行：
  - `python tools/verify/verify.py bills`
- 改构建/目录结构后，追加执行：
  - `python tools/verify/verify.py bills-build -- build_fast`

## 失败排查

- 查看 `tests/output/artifact/<project>/test_summary.json`
- 查看 `tests/output/artifact/<project>/logs/` 细分步骤日志
