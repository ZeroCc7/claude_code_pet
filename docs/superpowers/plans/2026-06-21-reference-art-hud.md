# 参考图美术 HUD Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将参考图中的莲花、心火、灵珠、灵石图标和暗金面板风格接入首页，同时保留动态数据。

**Architecture:** 四个图标由 imagegen 生成 2×2 洋红底素材表，经透明化和尺寸归一后转换为 RGB565+掩码。面板与顶栏使用固件绘制的暗青填充、双层暗金边线、角花和端帽，避免保存整张固定 HUD。

**Tech Stack:** imagegen、generate2dsprite、Python/Pillow、Arduino C++、Adafruit GFX

---

### Task 1：生成并处理四个 HUD 图标

- [ ] 以用户参考图为视觉基准，生成 2×2 图标表：莲花、心火、灵珠、金晶。
- [ ] 纯 `#FF00FF` 背景，无文字、数值、边框和阴影。
- [ ] 拆分为四个透明图标，归一到 14×14 固件画布。
- [ ] 视觉检查缩小后的辨识度。

### Task 2：转换固件资源

**Files:**
- Create: `scripts/convert_ui_icons.py`
- Create: `firmware/ai_pet/assets/home_ui_icons.h`

- [ ] 输出四个 RGB565 像素数组和 1bit 掩码。
- [ ] 保存固件尺寸预览。
- [ ] 检查透明角、主体覆盖率和头文件大小。

### Task 3：重做美术 HUD

**Files:**
- Modify: `firmware/ai_pet/game_ui.h`
- Modify: `firmware/ai_pet/game_ui.cpp`
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] 顶栏加入双层暗金边框、卷云角花和经验槽端帽。
- [ ] 心境、体力面板绘制暗金双线框并加入莲花、心火图标。
- [ ] 灵力、灵石面板加入灵珠、金晶图标。
- [ ] 底栏加入左右角花和完整四键中文提示。
- [ ] 数据和进度保持动态。

### Task 4：验证和刷写

- [ ] 运行全部测试。
- [ ] 编译并检查 Flash/RAM。
- [ ] 上传 COM7。
- [ ] 提交素材、转换脚本和 HUD 代码。
