# AI Hook、持续成长与进化 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 让本地历练、重复首领和 Codex／Claude Code／OpenCode 成功任务持续提供经验，并在 LV3、LV12 自动进化。

**Architecture:** 游戏规则继续由 `GameState` 管理，串口 JSON 解析拆到独立的 `AiEventProtocol`，UI 只显示 AI 状态和奖励反馈。主机侧使用一个公共 PowerShell 发送器，各工具 Hook 仅负责把原始事件映射成统一协议，避免在多个脚本里复制串口逻辑。

**Tech Stack:** Arduino C++、定长 JSON 字段解析、LittleFS、Adafruit GFX、Python 主机模型测试、PowerShell、pyserial

---

## 文件结构

- `host/game_model/progression.py`：规则参考模型。
- `host/game_model/test_progression.py`：成长、重复首领和进化测试。
- `firmware/ai_pet/game_types.h`：新增首领胜场和近期任务摘要存档字段。
- `firmware/ai_pet/game_state.*`：历练经验、递减奖励、LV3/LV12 进化、任务去重。
- `firmware/ai_pet/save_store.*`：旧存档迁移到新版本。
- `firmware/ai_pet/ai_event_protocol.*`：有长度限制的 JSON 事件解析和 ACK。
- `firmware/ai_pet/game_app.*`：串口事件接入、奖励结算、保存请求。
- `firmware/ai_pet/game_ui.*`：独立修炼状态页、完成和进化提示。
- `host/hooks/send-ai-pet-event.ps1`：互斥串口发送与 ACK。
- `host/hooks/codex-hook.ps1`：Codex 事件映射。
- `host/hooks/claude-hook.ps1`：Claude Code 事件映射。
- `host/hooks/opencode-plugin.js`：OpenCode 事件映射。
- `scripts/install-ai-hooks.ps1`：备份并增量安装可用 Hook。
- `host/hooks/test_hook_payloads.py`：主机侧事件映射测试。

### Task 1：成长、重复首领与 LV3 进化

**Files:**
- Modify: `host/game_model/test_progression.py`
- Modify: `host/game_model/progression.py`
- Modify: `firmware/ai_pet/game_state.cpp`
- Modify: `firmware/ai_pet/game_state.h`

- [ ] 添加失败测试：区域 0/1/2 每次历练分别增加 1/2/3 经验。
- [ ] 运行模型测试，确认因历练不加经验而失败。
- [ ] 实现历练经验结算。
- [ ] 添加失败测试：蛋在 40 经验进入成长期。
- [ ] 运行测试，确认仍在 LV5 才进化。
- [ ] 将 Python 和固件首次进化门槛改为 LV3。
- [ ] 添加失败测试：首领可重复挑战，奖励依次为 100%、50%、25%，最低为 1。
- [ ] 实现每区胜场计数和递减奖励。
- [ ] 运行全部成长测试。

### Task 2：存档升级与任务去重

**Files:**
- Modify: `firmware/ai_pet/game_types.h`
- Modify: `firmware/ai_pet/save_store.cpp`
- Modify: `firmware/ai_pet/save_store.h`
- Modify: `host/game_model/progression.py`
- Modify: `host/game_model/test_progression.py`

- [ ] 添加失败测试：相同来源和任务 ID 只能结算一次。
- [ ] 添加失败测试：超过 16 个任务后按环形顺序替换旧摘要。
- [ ] 在参考模型中实现 FNV-1a 来源任务摘要和 16 槽环形表。
- [ ] 在 `PetSaveData` 增加 `bossWins[3]`、`recentTaskHashes[16]`、写入索引。
- [ ] 将存档版本升级并迁移旧版；已击败首领的胜场初始化为 1。
- [ ] 在固件 `GameState` 实现任务查重与登记。
- [ ] 运行模型测试和固件静态测试。

### Task 3：AI 串口事件协议

**Files:**
- Create: `firmware/ai_pet/ai_event_protocol.h`
- Create: `firmware/ai_pet/ai_event_protocol.cpp`
- Create: `host/game_model/ai_protocol.py`
- Create: `host/game_model/test_ai_protocol.py`
- Modify: `firmware/ai_pet/game_app.h`
- Modify: `firmware/ai_pet/game_app.cpp`

- [ ] 添加失败测试：解析三种来源的状态事件。
- [ ] 添加失败测试：成功完成事件返回奖励字段；失败事件不奖励。
- [ ] 添加失败测试：未知来源、缺字段、超长消息被拒绝。
- [ ] 实现主机参考解析器并跑绿。
- [ ] 实现固件解析器，最大消息长度 384 字节。
- [ ] 将 `STATUS` 调试命令和 JSON 事件同时接入 `GameApp`。
- [ ] 成功任务调用 `applyTask()`，重复任务返回 `duplicate`。
- [ ] 所有状态和奖励变化请求保存并输出一行 JSON ACK。

### Task 4：修炼状态页与进化提示

**Files:**
- Modify: `firmware/ai_pet/game_types.h`
- Modify: `firmware/ai_pet/game_ui.h`
- Modify: `firmware/ai_pet/game_ui.cpp`
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] 添加源码结构测试：存在独立 `Cultivation` 页面和七类状态文案。
- [ ] 添加源码结构测试：成功奖励、失败、超时和进化提示均有入口。
- [ ] 实现修炼页，显示来源、状态、形态、耗时和程序绘制动效。
- [ ] 成功后显示经验与灵石；形态变化时显示新形态。
- [ ] 10 分钟无事件自动返回首页。
- [ ] 使用局部/画布刷新，避免全屏闪烁。
- [ ] 运行 UI 结构测试。

### Task 5：公共 Hook 发送器

**Files:**
- Create: `host/hooks/send-ai-pet-event.ps1`
- Create: `host/hooks/hook_state.py`
- Create: `host/hooks/test_hook_payloads.py`

- [ ] 添加失败测试：开始事件生成稳定任务 ID 并记录开始时间。
- [ ] 添加失败测试：成功结束计算 1～60 分钟耗时。
- [ ] 添加失败测试：状态事件不包含奖励字段。
- [ ] 实现状态文件、事件 JSON 和来源校验。
- [ ] 实现 PowerShell 串口发现、命名互斥锁、发送、ACK 和日志。
- [ ] 串口不可用时在 2 秒内退出且不影响调用方。
- [ ] 运行 Hook 测试。

### Task 6：三种工具适配与安装

**Files:**
- Create: `host/hooks/codex-hook.ps1`
- Create: `host/hooks/claude-hook.ps1`
- Create: `host/hooks/opencode-plugin.js`
- Create: `scripts/install-ai-hooks.ps1`
- Create: `docs/ai-hooks-guide.md`

- [ ] Codex：保留现有 notify 链并包装成功结束事件；检测到生命周期 Hook 时启用更多状态。
- [ ] Claude Code：生成 `UserPromptSubmit`、`PreToolUse`、`PostToolUseFailure`、`Notification`、`Stop`、`SessionEnd` 映射。
- [ ] OpenCode：生成消息、工具、权限、错误、idle 和会话结束映射。
- [ ] 安装器备份配置、只增量添加本项目 Hook，不覆盖已有 CursorLight 配置。
- [ ] 文档说明固定 COM 口、自动发现、日志、卸载和手动测试。
- [ ] 运行脚本语法检查和载荷测试。

### Task 7：完整验证与硬件烧录

**Files:**
- Modify: `docs/arduino-ide-guide.md`

- [ ] 运行全部 Python 测试。
- [ ] 运行 `git diff --check`。
- [ ] 编译 Arduino 固件并确认 Flash/RAM 余量。
- [ ] COM7 可用时烧录固件。
- [ ] 发送手动 status 和 completed JSON，检查 ACK、奖励和页面切换。
- [ ] 更新 Arduino IDE 与 Hook 使用说明。
- [ ] 提交实现，保留用户的 `硬件/` 文件夹为未跟踪。
