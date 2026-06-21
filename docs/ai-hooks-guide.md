# Codex、Claude Code、OpenCode 联动

## 安装

先插入 RP2040，确认设备端口。当前硬件通常为 `COM7`：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File .\scripts\install-ai-hooks.ps1 -Port COM7
```

安装器执行：

- 将公共脚本复制到 `%USERPROFILE%\.ai-pet-hooks\`。
- 设置用户环境变量 `AI_PET_PORT`。
- 备份并增量修改 `~/.claude/settings.json`。
- 在现有 CursorLight Codex 通知脚本末尾追加宠物完成事件，不覆盖灯效逻辑。
- 仅在检测到 OpenCode 时安装全局插件。

修改后重启对应 AI 工具。

## 状态映射

| 工具事件 | 设备显示 |
|---|---|
| 提交请求 | 接引任务 |
| 思考/生成 | 推演中 |
| 工具调用 | 施法中 |
| 文件修改 | 炼器中 |
| 等待用户或权限 | 等待指令 |
| 工具失败或拒绝 | 修炼受阻 |
| 成功结束 | 修炼完成并结算 |
| 会话结束 | 返回首页 |

当前 Codex 桌面版若只提供结束通知，则只能可靠触发“修炼完成”。项目已保留其他状态入口，后续 Codex 暴露更多生命周期 Hook 时可直接映射。

## 奖励

- 只在完整任务成功结束时奖励。
- 按持续时间计算，每分钟 `+2经验、+1灵石`。
- 最少按 1 分钟、最多按 60 分钟。
- 相同来源和任务 ID 重复上报不会重复奖励。

## 手动测试

推荐使用 Python 控制器：

```powershell
cd $env:USERPROFILE\.ai-pet-hooks
py -3 .\ai_pet_hook.py submitted
py -3 .\ai_pet_hook.py thinking
py -3 .\ai_pet_hook.py tool
py -3 .\ai_pet_hook.py editing
py -3 .\ai_pet_hook.py waiting
py -3 .\ai_pet_hook.py blocked
py -3 .\ai_pet_hook.py complete
py -3 .\ai_pet_hook.py idle
```

默认来源是 `codex`，任务会话名是 `manual`。也可以指定：

```powershell
py -3 .\ai_pet_hook.py thinking --source claude_code `
  --session demo-1 --port COM7
```

`complete` 会发放成功奖励；`failure` 和 `cancelled` 不发奖励。

```powershell
$payload = '{"type":"status","source":"codex","task_id":"manual-1","state":"thinking","timestamp":1}'
& "$env:USERPROFILE\.ai-pet-hooks\send-ai-pet-event.ps1" `
  -Payload $payload -Port COM7
```

成功任务：

```powershell
$payload = '{"type":"task","source":"codex","task_id":"manual-1","state":"completed","duration":300,"result":"success"}'
& "$env:USERPROFILE\.ai-pet-hooks\send-ai-pet-event.ps1" `
  -Payload $payload -Port COM7
```

设备应返回一行 JSON ACK。

## 日志与排查

日志：

```text
%USERPROFILE%\.ai-pet-hooks\ai-pet-hooks.log
```

端口变化时重新执行安装器，或设置：

```powershell
setx AI_PET_PORT COM7
```

Hook 无法连接设备时会快速退出，不阻塞 Codex、Claude Code 或 OpenCode。

## 卸载

安装器会在原配置旁创建带时间戳的 `.bak` 文件。恢复对应备份后，删除：

```powershell
Remove-Item -Recurse -Force "$env:USERPROFILE\.ai-pet-hooks"
[Environment]::SetEnvironmentVariable("AI_PET_PORT", $null, "User")
```

若安装了 OpenCode 插件，再删除：

```powershell
Remove-Item "$env:USERPROFILE\.config\opencode\plugins\ai-pet.js"
```
