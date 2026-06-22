# Codex 生命周期 Hook 联动设计

## 目标

让 Codex 的任务和工具生命周期驱动 RP2040 宠物的修炼状态页，同时保留现有 Codex Desktop `notify` 配置，不影响桌面通知功能。

## 配置方式

- Hook 脚本安装到 `%USERPROFILE%\.ai-pet-hooks\`。
- Codex 生命周期配置增量写入 `%USERPROFILE%\.codex\hooks.json`。
- 不修改 `%USERPROFILE%\.codex\config.toml` 中现有的 `notify`。
- 安装器提供只安装 Codex 的入口，不修改 Claude Code 或 OpenCode 配置。
- 已存在的非宠物 Hook 必须完整保留；重复安装不得重复添加宠物 Hook。

## 状态映射

| Codex 事件 | 宠物状态 |
|---|---|
| `UserPromptSubmit` | `submitted`，显示“接引任务” |
| `PreToolUse`，工具为 `apply_patch`、`Edit` 或 `Write` | `editing`，显示“炼器中” |
| 其他 `PreToolUse` | `tool`，显示“施法中” |
| `PermissionRequest` | `waiting`，显示“等待指令” |
| `PostToolUse` | `thinking`，显示“推演中” |
| `Stop` | `complete`，显示结算并发放成功奖励 |

Codex Hook 输入从标准输入读取 JSON。会话标识优先读取 `session_id`、`thread_id`、`turn_id`，同时兼容连字符字段；缺失时使用稳定的 `codex-default`。

## 运行约束

- 串口固定使用安装时指定的 COM 口，本机为 `COM5`。
- 串口不可用、输入格式异常或设备无 ACK 时，Hook 记录日志并快速退出，不能阻塞 Codex。
- `Stop` 只执行一次成功结算；固件已有任务 ID 去重作为第二层保护。
- Codex 首次加载新增 Hook 后，需要用户通过 `/hooks` 检查并信任配置。

## 验收

- `hooks.json` 中原有 Hook 保持不变。
- 重复运行安装器后，每个宠物 Hook 仍只有一份。
- 手动调用每种 Codex 事件时，设备返回 JSON ACK。
- `submitted`、`tool`、`editing`、`waiting`、`thinking` 状态页持续显示，不立即返回首页。
- `Stop` 显示结算页并只发放一次奖励。
- 现有 `notify` 配置内容不变。
