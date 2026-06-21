# 宠物美术与灵力恢复 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 接入混沌灵卵、凌霄麒麟幼体、镇岳麒麟幼体三套 4 帧像素动画，并实现被动恢复与打坐恢复灵力。

**Architecture:** 游戏规则先在 Python 模型中测试，再同步到 C++ `GameState`。存档升级到版本 3，并兼容版本 2 的固定结构迁移。三套素材由图像生成工具生成 2×2 洋红底精灵表，经统一处理、缩放和 RGB565+1bit 掩码转换后，由 `PetRenderer` 在现有离屏宠物画布中绘制。

**Tech Stack:** Arduino C++、LittleFS、Adafruit GFX、Python、Pillow、内置 imagegen、generate2dsprite、Arduino CLI

---

### Task 1：灵力恢复规则

**Files:**
- Modify: `host/game_model/progression.py`
- Modify: `host/game_model/test_progression.py`
- Modify: `firmware/ai_pet/game_state.h`
- Modify: `firmware/ai_pet/game_state.cpp`
- Modify: `firmware/ai_pet/game_types.h`

- [ ] 先增加失败测试：5 分钟恢复 1 点、上限 20、打坐恢复 3 点、满值不消耗次数、每周期最多 3 次、24 小时重置。
- [ ] 运行测试并确认因接口缺失失败。
- [ ] Python 增加 `tick_runtime(seconds)` 与 `meditate()`。
- [ ] C++ 增加相同规则，所有任务奖励也限制到 20。
- [ ] 运行全部规则测试。

核心常量：

```cpp
static constexpr uint16_t kMaxEnergy = 20;
static constexpr uint16_t kPassiveRecoverySeconds = 300;
static constexpr uint32_t kMeditationCycleSeconds = 86400;
static constexpr uint8_t kMeditationsPerCycle = 3;
```

### Task 2：存档版本 3 与版本 2 迁移

**Files:**
- Modify: `firmware/ai_pet/game_types.h`
- Modify: `firmware/ai_pet/game_state.cpp`
- Modify: `firmware/ai_pet/save_store.h`
- Modify: `firmware/ai_pet/save_store.cpp`

- [ ] 在 `PetSaveData` 的 CRC 前增加 `energyRecoverySeconds`、`meditationCycleSeconds`、`meditationsUsed`。
- [ ] 定义与旧布局完全一致的 `PetSaveDataV2`。
- [ ] `SaveStore` 按文件大小和版本分别校验 v2/v3。
- [ ] v2 加载后复制旧字段、新字段清零、灵力截断到 20，并在下一次保存写为 v3。
- [ ] 编译验证结构、CRC 和双槽写入。

### Task 3：培养页打坐与运行计时

**Files:**
- Modify: `firmware/ai_pet/game_app.cpp`
- Modify: `firmware/ai_pet/game_ui.cpp`
- Modify: `firmware/ai_pet/game_ui.h`
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] 培养页选择从 2 项改为 3 项。
- [ ] 第三张卡显示 `打坐 / 灵力 +3 / 余N次`。
- [ ] K1 调用 `meditate()` 并显示成功、灵力已满或今日已尽。
- [ ] `GameApp` 每秒调用 `tickRuntime(1)`，恢复或周期重置时请求保存和刷新。
- [ ] 运行 UI 结构测试与规则测试。

### Task 4：生成三套宠物精灵

**Files:**
- Create: `assets/raw/pets/<form>/idle_2x2.png`
- Create: `assets/processed/pets/<form>/`

- [ ] 每种形态用内置 imagegen 单独生成 2×2、4 帧待机精灵表，纯 `#FF00FF` 背景。
- [ ] 使用 generate2dsprite 处理：2 行 2 列、共享缩放、底部对齐、最大主体组件。
- [ ] 检查无越界、无洋红残边、四帧比例稳定。
- [ ] 输出透明精灵表、4 张单帧和 GIF。

### Task 5：转换并接入固件

**Files:**
- Create: `scripts/convert_pet_sprites.py`
- Create: `firmware/ai_pet/assets/pet_sprites.h`
- Modify: `firmware/ai_pet/pet_renderer.cpp`
- Modify: `firmware/ai_pet/pet_renderer.h`

- [ ] 转换脚本将每帧放入 48×48 单元，输出 RGB565 像素和 1bit 透明掩码。
- [ ] `PetRenderer` 对前三种形态按 `(now / 400) % 4` 选帧。
- [ ] 使用带掩码的 `drawRGBBitmap` 绘制，不覆盖洞府背景。
- [ ] 最终四形态继续使用程序占位图。
- [ ] 编译并检查 Flash/RAM 余量。

### Task 6：验证、刷写与提交

**Files:**
- Modify: `README.md`（如需补充打坐说明）

- [ ] 运行全部 Python 测试。
- [ ] 运行素材 QC 和 `git diff --check`。
- [ ] 编译固件。
- [ ] 上传 COM7。
- [ ] 提交规则、迁移、UI、素材和脚本。

验收：

```text
三种形态均显示正式 4 帧素材
宠物动画无闪烁和黑框
每 5 分钟恢复 1 灵力
培养页打坐恢复 3 灵力
24 小时累计周期最多打坐 3 次
旧存档核心数据可迁移
灵力始终不超过 20
```
