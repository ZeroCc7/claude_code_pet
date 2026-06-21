# 云海仙台主界面 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 使用云海仙台背景，并将正式宠物放大、下移，使其与背景清楚分离。

**Architecture:** 新背景继续转换为 128×160 RGB565 资源。宠物固件帧从 48×48 提升为 62×62，主页局部离屏画布同步扩大到 72×76，动画仍只更新局部区域。

**Tech Stack:** Python、Pillow、Arduino C++、Adafruit GFX、Arduino CLI

---

### Task 1：转换云海仙台背景

**Files:**
- Create: `assets/processed/backgrounds/cloud_terrace_home_128x160.png`
- Create: `firmware/ai_pet/assets/cloud_terrace_home.h`
- Modify: `firmware/ai_pet/game_ui.cpp`

- [ ] 将原图裁切、缩放、64 色量化为 128×160。
- [ ] 输出 RGB565 头文件。
- [ ] 主界面改用新背景资源。

### Task 2：放大并下移宠物

**Files:**
- Modify: `scripts/convert_pet_sprites.py`
- Modify: `firmware/ai_pet/assets/pet_sprites.h`
- Modify: `firmware/ai_pet/game_ui.h`
- Modify: `firmware/ai_pet/game_ui.cpp`

- [ ] 将固件帧尺寸改为 62×62，主体最大高度 60。
- [ ] 宠物区域改为 72×76，区域起点约为 `(28, 30)`。
- [ ] 宠物底部锚定在仙台表面，整体较上一版下移约 10 像素。
- [ ] 保留跳跃动画和局部离屏刷新。

### Task 3：验证和刷写

- [ ] 检查三种形态预览尺寸、透明角和主体覆盖率。
- [ ] 运行全部规则/UI 测试。
- [ ] 编译并检查 Flash/RAM。
- [ ] 上传 COM7。
- [ ] 提交背景、素材转换和布局改动。
