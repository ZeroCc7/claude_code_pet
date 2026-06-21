# 古风修仙操作界面 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将培养、历练、状态和战斗页面重做为统一的深青墨色、暗金边框古风修仙界面，并保持四键操作和无闪烁刷新。

**Architecture:** 保留现有 `GameUi` 全屏 `GFXcanvas16` 离屏合成机制，在其中增加统一的牌匾、卡片、进度条和底部按键栏绘制函数。页面函数只组织游戏数据；培养、探索和战斗反馈通过短时 UI 消息显示，不修改 `PetSaveData` 和存档版本。

**Tech Stack:** Arduino C++、Adafruit GFX、Adafruit ST7735、U8g2 中文字体、Python pytest、Arduino CLI

---

## 文件结构

- `firmware/ai_pet/game_ui.h`：增加主题色、通用绘制组件和短时操作提示状态。
- `firmware/ai_pet/game_ui.cpp`：实现统一古风组件，重写四个操作页面布局。
- `host/game_model/test_ui_refresh.py`：验证页面继续离屏合成，并检查统一组件与页面信息存在。

### Task 1：建立古风界面组件

**Files:**
- Modify: `host/game_model/test_ui_refresh.py`
- Modify: `firmware/ai_pet/game_ui.h`
- Modify: `firmware/ai_pet/game_ui.cpp`

- [ ] **Step 1：先写组件结构失败测试**

在 `host/game_model/test_ui_refresh.py` 增加：

```python
def test_operation_pages_use_shared_ancient_ui_components():
    for name in (
        "drawTitlePlaque(",
        "drawPanel(",
        "drawFooterHints(",
        "drawProgressBar(",
    ):
        assert name in UI_SOURCE
```

- [ ] **Step 2：运行测试并确认失败**

Run:

```powershell
py -3 -m pytest host/game_model/test_ui_refresh.py -q
```

Expected: 新测试因四个辅助函数不存在而失败。

- [ ] **Step 3：实现最小通用组件**

在 `GameUi` 中增加：

```cpp
void drawTitlePlaque(const char* title, uint16_t accent);
void drawPanel(int16_t x, int16_t y, int16_t width, int16_t height,
               bool selected);
void drawFooterHints(const char* left, const char* right);
void drawProgressBar(int16_t x, int16_t y, int16_t width, uint16_t value,
                     uint16_t maximum, uint16_t color);
```

主题颜色固定为深青墨底、暗金边框、暖白文字、朱砂警告色。所有函数使用 `target()` 和 `text()`，确保既能画到屏幕，也能画到菜单画布。

- [ ] **Step 4：运行测试与固件编译**

Run:

```powershell
py -3 -m pytest host/game_model -q
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

Expected: 测试通过，固件编译成功。

### Task 2：重做培养、历练和状态页

**Files:**
- Modify: `host/game_model/test_ui_refresh.py`
- Modify: `firmware/ai_pet/game_ui.cpp`

- [ ] **Step 1：先写页面内容失败测试**

增加源码结构测试，要求存在以下正式中文：

```python
def test_operation_pages_include_required_cultivation_information():
    for label in (
        "洞府培养",
        "体力 +20",
        "心境 +5",
        "秘境历练",
        "首领可战",
        "已镇守",
        "仙宠状态",
        "已臻化境",
    ):
        assert label in UI_SOURCE
```

- [ ] **Step 2：运行测试并确认失败**

Run:

```powershell
py -3 -m pytest host/game_model/test_ui_refresh.py -q
```

Expected: 因新标题和状态文本尚未实现而失败。

- [ ] **Step 3：重写三个页面**

`drawCare()` 使用两张纵向卡片：

```text
喂食    10灵石
体力 +20

互动
心境 +5
```

`drawAdventure()` 使用三张区域卡片，按 `regionProgress`、`bossDefeatedMask` 显示百分比、`首领可战`、`已镇守` 或 `尚未解锁`。

`drawStatus()` 显示当前形态、等级、修炼时间和剑/丹/体/灵四条倾向条；最高倾向用金色。等级小于 5 显示 `五级初醒`，小于 12 显示 `十二级化形`，否则显示 `已臻化境`。

- [ ] **Step 4：运行测试和编译**

Run:

```powershell
py -3 -m pytest host/game_model -q
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

Expected: 全部测试通过，中文字体与画布 API 编译成功。

### Task 3：重做战斗页并增加短时操作提示

**Files:**
- Modify: `host/game_model/test_ui_refresh.py`
- Modify: `firmware/ai_pet/game_ui.h`
- Modify: `firmware/ai_pet/game_ui.cpp`

- [ ] **Step 1：先写战斗信息失败测试**

```python
def test_battle_page_has_four_action_tiles_and_result_notice():
    for label in ("攻击", "法诀", "丹药", "防御", "敌方气血", "己方体力"):
        assert label in UI_SOURCE
    assert "startNotice(" in UI_SOURCE
```

- [ ] **Step 2：运行测试并确认失败**

Run:

```powershell
py -3 -m pytest host/game_model/test_ui_refresh.py -q
```

Expected: 因 `startNotice` 和新版战斗文字不存在而失败。

- [ ] **Step 3：实现战斗布局和提示**

在 `GameUi` 增加 RAM 内短时提示：

```cpp
char notice_[24] = {};
uint32_t noticeStartedAt_ = 0;
void startNotice(const char* message);
bool noticeActive(uint32_t now) const;
```

战斗操作前后比较 Boss HP、体力和战斗状态，分别显示 `造成伤害`、`气血恢复`、`防御架势`、`首领已伏` 或 `败退洞府`。战斗页显示敌我两条气血条，并用两行两列固定展示 K1 至 K4 招式。

- [ ] **Step 4：验证非阻塞刷新**

`draw()` 只在提示开始、提示结束、按键动作或现有动画帧时请求重画，不增加 `delay()`。菜单页仍由 `drawMenuFrame()` 一次写入屏幕。

- [ ] **Step 5：运行完整验证**

Run:

```powershell
py -3 -m pytest host/game_model -q
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
git diff --check
```

Expected: 测试零失败、编译退出码 0、无空白错误。

### Task 4：刷入实机并提交

**Files:**
- Modify: `firmware/ai_pet/game_ui.h`
- Modify: `firmware/ai_pet/game_ui.cpp`
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] **Step 1：确认 COM7**

Run:

```powershell
.\tools\arduino-cli\arduino-cli.exe board list
```

Expected: COM7 列出 Waveshare RP2040 Zero。

- [ ] **Step 2：刷入固件**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload-firmware.ps1 -Port COM7
```

Expected: UF2 写入成功并重新出现 COM7。

- [ ] **Step 3：实机检查清单**

- 进入培养、历练、状态、战斗无明显闪烁。
- 培养卡片选择清楚，资源与收益不重叠。
- 历练三张卡片能区分锁定、可战和已镇守。
- 状态四条倾向完整显示。
- 战斗四键说明和敌我气血可读。
- 返回主页后宠物动画正常。

- [ ] **Step 4：提交**

```powershell
git add firmware/ai_pet/game_ui.h firmware/ai_pet/game_ui.cpp host/game_model/test_ui_refresh.py docs/superpowers/plans/2026-06-21-ancient-cultivation-ui.md
git commit -m "feat: redesign cultivation operation UI"
```
