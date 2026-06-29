# AGENTS.md

本文件用于指导 AI 助手（Claude Code、Codex、OpenCode 等）在本仓库中高效工作。人类阅读的入门文档见 [README.md](README.md)。

---

## 1. 项目是什么

一台运行在 **RP2040-Zero** 上的离线「修仙宠物」设备：1.8 寸 ST7735S 彩屏 + 四个实体按键，通过 USB 串口接收 AI 工作状态，把 AI 任务的成功完成结算为宠物的经验、灵石和灵力。

技术栈：
- **固件**：Arduino C++（Earle Philhower RP2040 Core），运行在设备上。
- **主机**：Python 3，包含游戏规则的参考实现、AI Hook 控制器、串口验收测试。
- **协议**：JSON over USB Serial（115200，单行，≤384 字节）。

## 2. 架构总览

```text
┌─────────────────────────────────────────────────────────────┐
│  AI 工具（Codex / Claude Code / OpenCode / CodeFree-O）        │
│  事件钩子 ──► host/hooks/*.ps1 + ai_pet_hook.py               │
└──────────────────────────────┬──────────────────────────────┘
                               │ JSON over USB Serial (115200)
                               ▼
┌─────────────────────────────────────────────────────────────┐
│  固件 firmware/ai_pet/（RP2040-Zero）                          │
│                                                              │
│  ai_pet.ino ──► GameApp（编排）                                │
│                  ├─ ButtonScanner  ──► InputAction            │
│                  ├─ AiEventProtocol（解析串口 JSON）            │
│                  │     └─► GameState.applyAiTask()（结算奖励）   │
│                  ├─ GameState（规则 + 功法 + 区域 + 存档数据）       │
│                  ├─ SaveStore（LittleFS A/B 双槽 + CRC32）       │
│                  ├─ GameUi（多页面渲染：首页/背包/区域/历练/战斗/状态/功法）│
│                  └─ DisplayDevice（ST7735S + 背光 PWM）         │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│  host/game_model/（Python 参考模型，与固件规则一一对应）          │
│  progression.py  ◄── 镜像 GameState 的规则与数值                  │
│  ai_protocol.py  ◄── 镜像 AiEventProtocol 的解析与校验            │
└─────────────────────────────────────────────────────────────┘
```

**关键设计：固件和 host/game_model 是同一套游戏规则的两种实现。** 改规则时两边都要改并保持测试通过，否则设备行为与 Python 测试会漂移。

## 3. 文件地图

### firmware/ai_pet/（Arduino 草图，C++）

| 文件 | 职责 |
|---|---|
| `ai_pet.ino` | 入口，仅创建 `GameApp` 并调用 `begin()/update()` |
| `game_app.{h,cpp}` | 主编排器：串口、输入、秒级 tick、存档调度、AI 事件分发 |
| `game_state.{h,cpp}` | 游戏规则与 `PetSaveData`：历练、区域、战斗、功法、经验、进化、AI 任务结算 |
| `game_types.h` | 枚举（`PetForm`/`UiPage`/`ItemType`/`AdventurePhase` 等）+ `PetSaveData` 结构 |
| `region_config.{h,cpp}` | V1.2 区域配置：区域名、Boss、难度、解锁条件、奖励偏向 |
| `game_ui.{h,cpp}` | 全部 UI 渲染（首页/功德簿/背包/区域/历练/战斗/状态/功法/修炼 + 局部刷新） |
| `ai_event_protocol.{h,cpp}` | 解析 AI 任务 `start/end + source` 串口 JSON |
| `save_store.{h,cpp}` | LittleFS A/B 双槽存档 + CRC32 + 旧版迁移 |
| `display_device.{h,cpp}` | ST7735S 封装（init/背光/诊断图样） |
| `button_scanner.{h,cpp}` | 四按键去抖扫描 |
| `input_actions.h` | 按键索引 → `InputAction` 映射（K1=Confirm K2=Up K3=Down K4=Back） |
| `pet_renderer.{h,cpp}` | 宠物形态绘制 |
| `chinese_text.{h,cpp}` | U8g2 中文渲染封装 |
| `board_config.h` | 引脚与常量（屏幕/按键/波特率/SPI 频率/背光） |
| `diagnostics_app.{h,cpp}` | 串口诊断命令（`STATUS` 等） |
| `flash_probe.{h,cpp}` | LittleFS 探针 |
| `assets/` | 脚本生成的素材头文件（勿手改） |

### host/game_model/（Python 参考实现 + 测试）

| 文件 | 职责 |
|---|---|
| `progression.py` | `GameState` 数据类，镜像固件规则（数值必须一致） |
| `ai_protocol.py` | `parse_event()`，镜像固件协议校验 |
| `test_progression.py` / `test_ai_protocol.py` / `test_ui_refresh.py` | 规则与协议测试 |

### host/hooks/（AI 工具集成）

| 文件 | 职责 |
|---|---|
| `ai_pet_hook.py` | 手动触发 CLI：`py ai_pet_hook.py <command> [--source --session --port]` |
| `claude-hook.ps1` / `codex-hook.ps1` / `opencode-hook.ps1` / `codefree-o-hook.ps1` | 各工具的事件钩子 |
| `opencode-plugin.js` / `codefree-o-plugin.js` | OpenCode / CodeFree-O 插件 |
| `send-ai-pet-event.ps1` | 共用发送器 |
| `test_hook_payloads.py` | Hook 载荷测试 |

### host/diagnostics/（串口验收）

| 文件 | 职责 |
|---|---|
| `serial_acceptance.py` | 自动化验收脚本（`STATUS` 探测等） |
| `test_serial_acceptance.py` | 验收测试 |

### scripts/（PowerShell 工具链）

| 文件 | 职责 |
|---|---|
| `bootstrap-arduino.ps1` | 安装 `tools/arduino-cli/` 与所需库（首次必跑） |
| `compile-firmware.ps1` | 编译固件到 `build/firmware/` |
| `upload-firmware.ps1` | 烧录到指定 `-Port` |
| `install-ai-hooks.ps1` | 把 Hook 装进 Claude/Codex/OpenCode/CodeFree-O 配置 |
| `convert_background.py` / `convert_pet_sprites.py` / `convert_ui_icons.py` / `convert_v12_assets.py` | 素材转换 |

### 其他

- `docs/`：人类文档（hardware-bringup、arduino-ide-guide、ai-hooks-guide、art-generation-prompts、qingyun-asset-pipeline、evolution-sprite-pipeline）。
- `assets/raw/`、`assets/processed/`：美术素材。
- `tools/arduino-cli/`：捆绑的 arduino-cli（已 gitignore，由 bootstrap 安装）。
- `build/`：编译产物（已 gitignore）。

## 4. 构建与测试命令

### 首次环境准备（PowerShell）

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\bootstrap-arduino.ps1
py -3 -m pip install -r .\host\requirements-dev.txt
```

### 编译固件

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

FQBN：`rp2040:rp2040:waveshare_rp2040_zero:flash=2097152_262144`（Sketch 1792KB / LittleFS 256KB）。编译成功后检查 Flash/RAM 占用。

### 烧录固件

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload-firmware.ps1 -Port COM7
```

### Python 测试（规则、协议、Hook、验收解析）

```powershell
py -3 -m pytest .\host\game_model .\host\hooks .\host\diagnostics -q
```

### 安装 AI Hook

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\install-ai-hooks.ps1 -Port COM7
```

### 手动触发 Hook（冒烟测试）

```powershell
cd $env:USERPROFILE\.ai-pet-hooks
py -3 .\ai_pet_hook.py start --source codex
py -3 .\ai_pet_hook.py end   --source codex
```

`end` 会真实修改设备存档、发放奖励并写入功德簿。

### 串口状态检查

波特率 115200，发送 `STATUS`，返回例如 `STATUS level=3 form=1 xp=48 mood=100 ...`。

## 5. 代码规范

### C++（固件）

- `#pragma once` 作为头文件保护。
- 类成员 `snake_case_`（私有带尾部下划线），常量 `kPascalCase`（如 `kSaveMagic`）。
- 命名空间 `board::` 收纳硬件常量（`board_config.h`）。
- 头文件声明公有 API，`.cpp` 实现私有逻辑，私有方法以 `_` 结尾或放 `private:`。
- 用 `constexpr` 表达编译期常量；用 `static_assert` 校验引脚约束（见 `board_config.h:24`）。
- 数值用 `uint8_t/uint16_t/uint32_t`，配合 `clampPercent()` 等工具钳制到 0–100。
- 不使用 STL/动态分配（RP2040 嵌入式，需确定性内存）。
- 不写注释，除非用户要求；命名承担表达职责。

### Python（主机）

- `from __future__ import annotations`，dataclass + 类型注解。
- 测试用 pytest，函数式 `test_*`，arrange/act/assert 清晰。
- 协议常量（`SOURCES`/`STATES`）在固件 `.h` 与 Python 两处定义，必须保持一致。

### UI 文本

- 所有面向用户的字符串为中文古风措辞，集中在渲染层调用处。
- 源文件必须 UTF-8 编码。

## 6. 关键不变量（修改前必读）

1. **Flash 布局固定**：必须选 `2MB (Sketch: 1792KB, FS: 256KB)`。选 `2MB (no FS)` 会导致 `SaveStore` 失败、存档丢失。
2. **`PetSaveData` 是存档磁盘格式**：`game_types.h` 中的结构体字段顺序/类型即存档二进制布局。`save_store.cpp` 用 `kSaveMagic=0x50455431`、`kSaveVersion`（当前 10）、`size`、`sequence`、`crc32` 做校验。改结构必须 bump `kSaveVersion`。当前 V1.1/V1.2 均不迁移旧结构存档，旧存档应直接失效并初始化新角色。
3. **A/B 双槽**：`SaveStore` 轮流写入 A/B 两个文件，CRC 校验通过才视为有效，写入失败不覆盖最后有效存档。
4. **AI 协议上限 384 字节**：`AiEventProtocol::kMaximumMessageBytes=384`（`host/game_model/ai_protocol.py` 同步）。超长会被拒绝。
5. **单活动任务**：设备仅接受空闲时的 `start`，运行中忽略其他 `start`；只有来源匹配的 `end` 才结算。协议不使用 `task_id`。
6. **奖励规则**：设备按本地运行时间结算，每分钟 2 经验、1 灵石，最少按 1 分钟、最多按 60 分钟；30 分钟未结束自动减半结算。
7. **引脚分配写死**：`board_config.h` 定义 GP2–GP11，改引脚必须同步更新 `docs/hardware-bringup.md` 和 README 接线表。
8. **固件 ↔ Python 规则必须一致**：`firmware/ai_pet/game_state.cpp` 与 `host/game_model/progression.py` 是同一规则的两种实现。改一处必须改另一处并更新对应 pytest。
9. **进化阈值固定**：LV3 第一次分支、LV12 最终分支，每 20 经验一级，最高 LV30。
10. **串口独占**：Hook 与 Arduino 串口监视器不能同时占用同一 COM 口。

## 7. 常见开发任务

### 新增一个 UI 页面

1. 在 `game_types.h` 的 `enum class UiPage` 增加枚举值。
2. 在 `game_ui.h/cpp` 实现 `drawXxx(const PetSaveData&)`，并在 `draw()` 按 `page_` 分发。
3. 在 `input_actions.h` 或 `game_app.cpp` 的 `processInput` 里接入按键导航。
4. 若有持久状态，加入 `PetSaveData`（注意 bump `kSaveVersion`，是否迁移按当前版本设计执行）。
5. 在 `host/game_model/` 加对应 Python 镜像与测试（若涉及规则）。

### 调整游戏数值规则

1. 同时改 `firmware/ai_pet/game_state.cpp` 和 `host/game_model/progression.py`。
2. 更新/新增 `host/game_model/test_progression.py` 用例。
3. 跑 pytest，再编译固件确认。
4. 若影响存档结构，bump `kSaveVersion`，是否迁移按当前版本设计执行。

### 调整 AI 任务协议

1. 同步修改固件 `ai_event_protocol.*` 与 Python `ai_protocol.py`。
2. 保持协议仅包含 `start/end + source`。
3. 更新四种工具的开始与结束生命周期映射。
4. 更新协议、Hook 和安装器测试。
5. 更新 README 与 `docs/ai-hooks-guide.md`。

### 新增一个区域

1. 在 `region_config.{h,cpp}` 增加区域配置，确认 `kRegionCount` 与 `PetSaveData` 中 5 个区域槽位一致。
2. 同步 `host/game_model/progression.py` 的区域配置、解锁条件、难度阶梯和奖励偏向。
3. 若新增持久字段，修改 `game_types.h` 并 bump `kSaveVersion`。
4. 更新区域选择、Boss 展示、宝物页等 UI 文案与素材接口。
5. 更新 `host/game_model/test_progression.py` 和 `test_ui_refresh.py`。

### 新增素材

1. 放原始图到 `assets/raw/`。
2. 跑对应 `scripts/convert_*.py`，产物落到 `firmware/ai_pet/assets/` 头文件。
3. 不要手改 `assets/` 下的生成头文件。

### 新增修炼动画（evolution 帧）

详细管线文档：`docs/evolution-sprite-pipeline.md`

1. 将 4 张 128×128 RGBA 透明背景的独立帧放入 `assets/processed/pets/<form>/evolution-{1,2,3,4}.png`。
2. 运行 `py -3 .\scripts\convert_pet_sprites.py`，自动重新生成 `pet_sprites.h`。
3. 检查 `kEvolutionForms[]` 是否包含该形态。
4. 编译固件确认。
5. 不需要改 C++ 代码——`PetRenderer` 和 `drawCultivation()` 已支持所有 final form 的 evolution 帧。
6. 灵兽阶段（Egg/Rookie）修炼时仍使用 idle 帧，如需扩展需改 `pet_renderer.cpp`。

## 8. 验证清单（改完代码后）

- [ ] `py -3 -m pytest .\host\game_model .\host\hooks .\host\diagnostics -q` 全绿。
- [ ] `scripts\compile-firmware.ps1` 成功，Flash/RAM 未超限。
- [ ] 若改了规则，固件与 Python 两处数值一致。
- [ ] 若改了存档结构，`kSaveVersion` 已 bump，旧存档处理符合当前版本设计。
- [ ] 若改了引脚，`board_config.h`、README 接线表、`docs/hardware-bringup.md` 三处同步。
- [ ] 若改了协议，固件 `.h` 与 Python `ai_protocol.py` 两处常量一致。
- [ ] 不要提交 `build/`、`tools/`、`__pycache__/`（已在 `.gitignore`）。

## 9. 硬件事实（无需重新验证）

- 屏幕：ST7735S 128×160，`INITR_BLACKTAB`，rotation 0，SPI 8MHz，RGB 正常。
- 按键：K1–K4 = GP8–GP11，空闲 HIGH，按下 LOW，板载 4.7kΩ 上拉。
- 背光：GP7，HIGH/PWM 增亮。
- RP2040-Zero：2MB Flash，USB Type-C。
- 已通过验收：屏幕、按键、背光 PWM、LittleFS 读写校验、AI Hook ACK/奖励/防重复、LV3 进化。

## 10. V1.1 青云山道（已完成）

设计文档：`docs/superpowers/specs/2026-06-23-v1.1-qingyun-adventure-design.md`

开发目标：

1. ✅ 首页改为 K1 功德簿、K2 背包、K3 历练、K4 状态。
2. ✅ 删除喂食、互动、打坐和培养入口，保留心境字段供后续扩展。
3. ✅ 重做青云山道：自动前进、事件自动结算、场景化显示和灵力逐步消耗。
4. ✅ 增加青云妖狼：固定进度或信物解锁，全自动战斗，每回合消耗 1 点灵力。
5. ✅ 让等级、形态和四类成长倾向参与自动战斗，并保留少量暴击、闪避随机性。
6. ✅ 增加固定五种物品的轻量背包：灵草、回春丹、攻击符、护身符、青云信物（V1.2 起文案改为秘境令）。
7. ✅ 增加功德簿，持久保存最近 10 条成功 AI 任务，每页显示 2 条。
8. ✅ 升级存档版本，不迁移 V1.0 存档，首次启动 V1.1 时从头修炼。
9. ✅ 固件与 Python 参考模型同步实现，测试后再做实机验收。
10. ✅ AI Hook 协议简化为仅 `start/end + source`：设备只跟踪一个活动任务，忽略并发开始和来源不匹配的结束；30 分钟超时自动减半结算。

额外完成：

- ✅ 历练事件随机顺序：每次出发时 Fisher-Yates 洗牌，4 个事件在 25/45/65/85% 检查点随机出现。
- ✅ 通关后可重复轮回挑战，难度递增（HP +15%/伤害 +8% 前 10 轮，之后 +3%/+2%），第 50 轮封顶。
- ✅ 青云剑：通关随机掉落（概率 = min(10%, 轮数×1%)），20 次保底，永久 +10% 伤害/-10% 受伤。
- ✅ SDL2 桌面模拟器：免烧录预览固件 UI（shims + backend + overrides 架构）。

范围限制：

- V1.1 只完整重做青云山道。
- 不做完整装备、稀有度、随机词条、合成和每日任务系统。
- 第二个区域开始开发时，再判断是否抽取通用区域引擎。

## 11. 四维属性成长系统（已完成）

四维属性（剑、丹、体、灵）已接入战斗公式和进化分支判定，并通过以下途径提升：

- **历练事件自动结算**：灵草→灵、妖兽→剑、伤者→灵、捷径→丹，根据事件类型自动增加对应倾向。
- **击败 Boss**：根据战斗轮次奖励属性。
- **AI 任务完成**：根据时长分配倾向。

固件与 Python 参考模型已同步实现，测试通过。

## 12. V1.2 功法与青竹灵境（已完成）

开发目标：

1. ✅ 新增四门被动功法：太虚剑诀、九转丹法、不灭玄功、万灵息法。
2. ✅ 功法等级 Lv0-Lv9，升级消耗对应倾向门槛和灵石，不扣除倾向数值，不影响进化分支判定。
3. ✅ 状态页新增“功法概览”，可进入“功法修炼”页选择并升级功法。
4. ✅ 功法接入战斗、丹药回复、被动恢复、护体、闪避和灵力上限等规则。
5. ✅ 开放第二个区域“青竹灵境”，Boss 为“竹灵守卫”，奖励偏向丹和灵。
6. ✅ 预置云海剑台、丹霞药谷、玄河古战场的锁定区域配置、解锁条件、难度阶梯和奖励偏向。
7. ✅ 青云信物改为通用“秘境令”：进入历练时可选择消耗 1 枚，直接推进到当前区域 Boss 可挑战状态。
8. ✅ 区域宝物改为通用结构：青云剑、灵竹玉佩及后续区域宝物共用掉落和 20 次保底逻辑。
9. ✅ Boss HP 改为 `uint16_t`，V1.2 难度整体提高：青云山道基础 HP 60、基础伤害 13；青竹灵境基础 HP 90、基础伤害 17。
10. ✅ 固件与 Python 参考模型同步实现 V1.2 功法、青竹灵境、秘境令直达 Boss、区域宝物和难度阶梯。

素材状态：

- V1.2 代码不包含生成、绘制或合成正式素材。
- 缺失素材清单记录在 `docs/art-asset-self-service-guide.md` 和 `docs/art-generation-prompts.md`：青云剑图标、青竹灵境背景、竹灵守卫 Boss、灵竹玉佩图标。
- 用户后续把 PNG 放入指定路径后，再运行 `scripts/convert_v12_assets.py` 接入固件资源并编译验证。
