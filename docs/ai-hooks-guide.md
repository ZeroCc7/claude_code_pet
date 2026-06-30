# Codex、Claude Code、OpenCode、CodeFree-O 联动

## 安装

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File .\scripts\install-ai-hooks.ps1 -Port COM7
```

安装器复制公共发送脚本、保存串口配置，并按工具安装两个生命周期 Hook：

| 工具 | 开始 | 结束 |
|---|---|---|
| Codex | `UserPromptSubmit` | `Stop` |
| Claude Code | `UserPromptSubmit` | `Stop` |
| OpenCode | `session.created` | `session.idle` |
| CodeFree-O | `session.created` | `session.idle` |

安装后重启对应工具。

## 协议

```json
{"type":"start","source":"codex"}
{"type":"end","source":"codex"}
```

支持 `codex`、`claude-code`、`opencode` 和 `codefree-o`。

设备负责计时，不使用任务 ID，也不创建主机任务状态文件。

- 空闲时接受 `start`。
- 已运行任务时忽略新的 `start`。
- 只接受与当前任务来源一致的 `end`。
- 没有活动任务时忽略 `end`。
- 30 分钟未结束时自动减半结算。
- 超时后的迟到 `end` 会被忽略。

## 手动测试

```powershell
cd $env:USERPROFILE\.ai-pet-hooks
py -3 .\ai_pet_hook.py start --source codex
py -3 .\ai_pet_hook.py end --source codex
```

`end` 会真实发放奖励并写入功德簿。

Hook、Arduino 串口监视器和其他串口工具不能同时打开同一 COM 口。
