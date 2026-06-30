# Simple AI Task Protocol Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 用 `start/end + source` 替换任务 ID 和中间状态协议，由设备计时并执行单活动任务、来源匹配和 30 分钟超时减半结算。

**Architecture:** 协议解析器只接受两种事件。`GameApp` 保存活动来源和开始毫秒数，结束或超时时调用 `GameState` 结算并写功德簿。主机 Hook 变为无状态发送器，各工具只映射开始和结束生命周期。

**Tech Stack:** Arduino C++、Python 3、pytest、PowerShell、JavaScript

---

### Task 1: Python 协议与奖励模型

- [ ] 更新协议测试，要求只接受 `start/end` 和四个连字符来源。
- [ ] 运行测试确认旧解析器失败。
- [ ] 实现新 `parse_event()`。
- [ ] 更新 `GameState`，移除任务哈希，增加无 ID 的成功结算记录。
- [ ] 运行模型测试并提交。

### Task 2: 固件协议与单任务控制器

- [ ] 更新源码结构测试，要求 `AiEventKind::Start/End`，不含 `taskId/state/duration/result`。
- [ ] 运行测试确认失败。
- [ ] 实现固件解析器和 `GameApp` 单活动任务状态。
- [ ] 实现同来源结束、错误来源忽略、30 分钟减半超时和迟到结束忽略。
- [ ] 编译并提交。

### Task 3: 无状态主机 Hook

- [ ] 更新 Hook 测试，只要求 `start/end` 载荷，不创建 `state.json`。
- [ ] 运行测试确认失败。
- [ ] 简化手动控制器、Codex、Claude Code、OpenCode 和 CodeFree-O 映射。
- [ ] 更新安装脚本，仅安装开始与结束 Hook。
- [ ] 运行 Hook 测试并提交。

### Task 4: 文档与完整验证

- [ ] 更新 README 和 Hook 指南。
- [ ] 运行全部 Python 测试。
- [ ] 编译固件。
- [ ] 运行 `git diff --check` 并提交。
