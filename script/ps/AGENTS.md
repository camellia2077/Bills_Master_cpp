# AGENTS Instructions (`script/ps`)

当使用 `script/ps/reusable_squash_first_parent.ps1` 时，请先在仓库 `temp/` 目录创建提交信息 `.txt` 文件，再将该文件路径作为 `-MessageFile` 传入脚本。

## Required workflow
- 先写入 `temp/<name>.txt`（UTF-8，无 BOM）。
- 先复制 `script/ps/reusable_squash_first_parent.ps1` 到 `temp/` 再从 `temp/` 中执行。
- 再执行 `reusable_squash_first_parent.ps1 -MessageFile <temp 文件路径>`。
- 不要在命令行中直接内联长 message 内容。
- 执行合并后不需要再询问用户，默认直接继续执行提交流程（force push）。
- 如果版本标签（如 `v0.4.0`）因历史重写受到影响，需要默认重挂到最新 `main/HEAD` 并推送。

## Why
- 这样可以减少命令过长/转义问题，避免推送前自动流程中的命令拦截。
- 脚本执行过程中会重写历史，先复制到 `temp/` 可避免原路径脚本在重写时消失导致流程中断。
