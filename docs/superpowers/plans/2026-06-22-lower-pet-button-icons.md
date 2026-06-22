# 首页宠物位置与按钮图标 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将首页宠物下移 14px，并用四枚正式古风像素图标替换底部占位图形。

**Architecture:** 宠物继续使用现有 72×100 离屏画布，只调整画布内基准坐标。四枚按钮图标由一张 2×2 原始素材表生成，经去背、裁切和 14×14 归一化后转换为 RGB565 固件头文件。

**Tech Stack:** built-in image generation、generate2dsprite、Python/Pillow、Arduino C++、RGB565、Adafruit GFX

---

### Task 1：锁定布局测试

**Files:**
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] 添加断言，要求 `petY = kPetRegionY + 33`。
- [ ] 添加断言，要求首页引用 `home_button_icons.h` 和四枚按钮图标。
- [ ] 运行测试，确认新断言失败。

### Task 2：生成并处理四枚图标

**Files:**
- Create: `assets/raw/ui/button_icons/buttons_2x2.png`
- Create: `assets/processed/ui/button_icons/*`

- [ ] 使用 2×2 网格和纯洋红背景生成四枚图标。
- [ ] 使用 generate2dsprite 处理透明背景和四个单帧。
- [ ] 将四个单帧归一化为 14×14。
- [ ] 视觉检查含义、边缘、透明区域和一致性。

### Task 3：转换并集成固件素材

**Files:**
- Create: `scripts/convert_button_icons.py`
- Create: `firmware/ai_pet/assets/home_button_icons.h`
- Modify: `firmware/ai_pet/game_ui.h`
- Modify: `firmware/ai_pet/game_ui.cpp`

- [ ] 将四枚 PNG 转换为 RGB565 像素和 1-bit mask。
- [ ] 添加 `drawButtonIcon()`。
- [ ] 替换四段程序绘制占位图形。
- [ ] 将宠物基准位置下移 14px。
- [ ] 运行 UI 测试并确认通过。

### Task 4：编译和实机验证

**Files:**
- Modify: `README.md`（仅当布局说明需要更新）

- [ ] 运行全部 Python 测试。
- [ ] 运行 `git diff --check`。
- [ ] 编译固件并检查 Flash/RAM。
- [ ] COM7 可用时烧录并检查宠物位置及四枚图标。
- [ ] 提交本次更改。

