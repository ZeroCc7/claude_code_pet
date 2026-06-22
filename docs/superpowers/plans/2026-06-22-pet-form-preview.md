# 宠物形态预览模式 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 通过串口 `PREVIEW 0..6` 临时切换渲染形态，并用 `PREVIEW OFF` 恢复真实存档形态。

**Architecture:** 预览状态只存在 `GameUi` RAM 中，`GameApp` 负责命令解析。绘制代码通过 `displayForm()` 选择预览形态或真实形态，游戏状态和保存路径完全不接触预览值。

**Tech Stack:** Arduino C++、USB CDC 串口、Python 源码结构测试、Adafruit GFX

---

### Task 1：预览状态 API

**Files:**
- Modify: `host/game_model/test_ui_refresh.py`
- Modify: `firmware/ai_pet/game_ui.h`
- Modify: `firmware/ai_pet/game_ui.cpp`

- [ ] 添加失败测试，要求 `setPreviewForm()`、`clearPreviewForm()`、`previewEnabled()`、`previewForm()` 和 `displayForm()`。
- [ ] 运行测试并确认失败。
- [ ] 在 `GameUi` 添加 RAM 预览字段和 API。
- [ ] 首页宠物和状态页名称改用 `displayForm()`。
- [ ] 运行测试并确认通过。

### Task 2：串口命令

**Files:**
- Modify: `host/game_model/test_ui_refresh.py`
- Modify: `firmware/ai_pet/game_app.h`
- Modify: `firmware/ai_pet/game_app.cpp`

- [ ] 添加失败测试，要求解析 `PREVIEW OFF` 和 `PREVIEW 0..6`。
- [ ] 添加断言，预览处理路径不调用 `mutableData()` 或 `requestSave()`。
- [ ] 运行测试并确认失败。
- [ ] 实现 `processPreviewCommand()`。
- [ ] 非法编号返回 `PREVIEW error`。
- [ ] 成功后强制重绘首页并返回预览状态。

### Task 3：STATUS 和文档

**Files:**
- Modify: `firmware/ai_pet/game_app.cpp`
- Modify: `README.md`

- [ ] `STATUS` 增加 `preview` 和 `preview_form`。
- [ ] README 增加串口预览命令和安全说明。
- [ ] 运行全部 Python 测试。
- [ ] 运行固件编译。

### Task 4：实机测试

- [ ] 烧录 COM7。
- [ ] 发送 `PREVIEW 0`、`PREVIEW 1`、`PREVIEW 2` 和 `PREVIEW OFF`。
- [ ] 每步读取 `STATUS`，确认真实 `form` 始终不变。
- [ ] 提交代码。

