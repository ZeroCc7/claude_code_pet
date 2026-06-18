# 本地宠物游戏 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在已验证的 RP2040 硬件底座上实现可离线游玩的宠物主页、培养、探索、Boss 战、分支进化和可靠存档。

**Architecture:** 游戏逻辑使用不依赖屏幕的纯数据模块，UI 只读取状态并发送动作。首个可烧录版本先完成游戏状态、存档、主页和四键导航，再逐步加入探索、战斗与进化，始终保持设备可运行。

**Tech Stack:** Arduino C++、Adafruit GFX/ST7735、LittleFS、USB CDC、Python pytest、Arduino CLI

---

## 文件结构

```text
firmware/ai_pet/
├─ game_types.h                 游戏枚举与固定大小数据结构
├─ game_state.h/.cpp            属性、资源、时间推进和玩家动作
├─ save_store.h/.cpp            双槽存档、序列号与 CRC32
├─ input_actions.h              四键到语义动作的映射
├─ pet_renderer.h/.cpp          程序化像素宠物和状态动画
├─ game_ui.h/.cpp               主页、培养、冒险、战斗、状态页面
├─ game_app.h/.cpp              游戏主循环、自动保存和诊断模式切换
└─ ai_pet.ino                   程序入口
host/game_model/
├─ progression.py               与固件规则一致的可执行规则模型
└─ test_progression.py          奖励、探索、战斗和进化测试
```

## Task 1：游戏状态与规则模型

**Files:**
- Create: `host/game_model/progression.py`
- Create: `host/game_model/test_progression.py`
- Create: `firmware/ai_pet/game_types.h`
- Create: `firmware/ai_pet/game_state.h`
- Create: `firmware/ai_pet/game_state.cpp`

- [ ] 先写 pytest，覆盖新档默认值、互动提高心情、喂食消耗金币、探索消耗能量、失败任务只给少量经验。
- [ ] 运行 `py -3 -m pytest host/game_model -q`，确认因模块不存在而失败。
- [ ] 实现 Python 规则模型，让测试通过。
- [ ] 按相同常量实现 C++ `GameState`，所有数值使用定宽整数并做上下限保护。
- [ ] 运行 pytest 和固件编译。
- [ ] 提交 `feat: add offline pet game state`。

核心数据：

```cpp
enum class PetForm : uint8_t { Egg, RookieA, RookieB, FinalA1, FinalA2, FinalB1, FinalB2 };
enum class UiPage : uint8_t { Home, Care, Adventure, Battle, Status };

struct PetSaveData {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  uint32_t sequence;
  PetForm form;
  uint8_t level;
  uint16_t experience;
  uint8_t mood;
  uint8_t stamina;
  uint16_t coins;
  uint16_t energy;
  uint16_t tendencies[4];
  uint8_t regionProgress[3];
  uint8_t bossDefeatedMask;
  uint32_t playSeconds;
  uint32_t crc32;
};
```

## Task 2：双槽存档

**Files:**
- Create: `firmware/ai_pet/save_store.h`
- Create: `firmware/ai_pet/save_store.cpp`
- Modify: `host/game_model/test_progression.py`

- [ ] 增加 CRC 损坏和高序列号槽优先测试。
- [ ] 实现标准 CRC32 测试向量 `123456789 -> 0xCBF43926`。
- [ ] 使用 `/save_a.bin`、`/save_b.bin` 轮换写入；完整回读校验后才报告成功。
- [ ] 两槽都无效时创建默认档，不格式化已有文件系统。
- [ ] 编译并提交 `feat: add dual-slot pet save store`。

## Task 3：主页与四键导航

**Files:**
- Create: `firmware/ai_pet/input_actions.h`
- Create: `firmware/ai_pet/pet_renderer.h`
- Create: `firmware/ai_pet/pet_renderer.cpp`
- Create: `firmware/ai_pet/game_ui.h`
- Create: `firmware/ai_pet/game_ui.cpp`

- [ ] 将 K1/K2/K3/K4 映射为确认、上、下、返回。
- [ ] 实现 128×160 竖屏主页：顶部等级和连接图标、中部宠物、底部心情/体力/能量。
- [ ] 用矩形和有限调色板绘制 32×32 占位宠物，不引入大位图。
- [ ] K2 进入培养、K3 进入冒险、K4 进入状态；子页 K4 返回。
- [ ] 页面切换只重绘脏区域。
- [ ] 编译并提交 `feat: add pet home screen and navigation`。

## Task 4：培养与探索

**Files:**
- Modify: `firmware/ai_pet/game_state.h`
- Modify: `firmware/ai_pet/game_state.cpp`
- Modify: `firmware/ai_pet/game_ui.cpp`
- Modify: `host/game_model/progression.py`
- Modify: `host/game_model/test_progression.py`

- [ ] 测试喂食、互动、开始探索、每60秒推进和区域100%封顶。
- [ ] 培养页提供“喂食”和“互动”。
- [ ] 冒险页提供三个区域；未解锁区域显示锁。
- [ ] 探索每分钟消耗1能量并增加1至3进度，奖励使用确定性轻量 PRNG。
- [ ] 进度100%后解锁该区域 Boss。
- [ ] 关键资源变化后请求保存，普通时间推进最多60秒保存一次。
- [ ] 测试、编译并提交 `feat: add care and exploration loop`。

## Task 5：Boss 回合战斗

**Files:**
- Modify: `firmware/ai_pet/game_types.h`
- Modify: `firmware/ai_pet/game_state.h`
- Modify: `firmware/ai_pet/game_state.cpp`
- Modify: `firmware/ai_pet/game_ui.cpp`
- Modify: `host/game_model/test_progression.py`

- [ ] 测试普通攻击、技能消耗、防御减伤、道具恢复、胜利奖励和失败回主页。
- [ ] 三个 Boss 使用固定属性表。
- [ ] 战斗菜单：K1 攻击、K2 技能、K3 道具、K4 防御。
- [ ] 每个完整回合结束后保存检查点。
- [ ] 胜利设置 Boss 位并解锁下一区域；失败不扣等级和形态。
- [ ] 测试、编译并提交 `feat: add turn-based boss battles`。

## Task 6：分支进化

**Files:**
- Modify: `firmware/ai_pet/game_state.cpp`
- Modify: `firmware/ai_pet/pet_renderer.cpp`
- Modify: `firmware/ai_pet/game_ui.cpp`
- Modify: `host/game_model/test_progression.py`

- [ ] 测试等级门槛、四类倾向排序和7种形态路径。
- [ ] 等级5从 Egg 进入 RookieA/B；等级12进入四种最终形态。
- [ ] 状态页显示四类倾向条和当前进化预兆。
- [ ] 进化触发后播放非阻塞闪烁动画并立即保存。
- [ ] 每种形态使用独立颜色、耳朵/角/装甲轮廓。
- [ ] 测试、编译并提交 `feat: add branching pet evolution`。

## Task 7：游戏主程序与实物验收

**Files:**
- Create: `firmware/ai_pet/game_app.h`
- Create: `firmware/ai_pet/game_app.cpp`
- Modify: `firmware/ai_pet/ai_pet.ino`
- Modify: `README.md`

- [ ] `GameApp` 初始化显示、按钮、LittleFS 和存档。
- [ ] 启动时长按 K1+K4 进入原硬件诊断，否则进入宠物主页。
- [ ] 主循环处理按键、游戏时间、动画、脏区重绘和节流保存。
- [ ] 串口支持 `STATUS`，返回等级、形态、资源和页面。
- [ ] 运行全部 pytest、固件编译和 `git diff --check`。
- [ ] 烧录 COM7，实测主页、四键导航、喂食、探索、断电恢复。
- [ ] 更新 README 并提交 `feat: ship offline pet game vertical slice`。

## 完成标准

- 开机进入宠物主页，硬件诊断仍可通过组合键进入。
- 四键能完成主页、培养、冒险、战斗、状态页闭环操作。
- 断电重启后等级、资源、探索和战斗检查点不丢失。
- 7种形态和三段进化路径可通过测试触发。
- 3区域和3 Boss 可完整通关。
- 固件保持在 1792KB 程序分区内，RAM 留有至少 150KB。
