# AI 修仙宠物（RP2040-Zero + ST7735S）

一台运行在 RP2040-Zero 上的离线修仙宠物设备。

当前稳定版本：`v1.2.0`。V1.2 已完成并接入功法修炼、青竹灵境、秘境令直达 Boss、区域宝物、青竹灵境正式素材和战斗界面反馈优化；主机测试、固件编译和实机部分验收均已通过。

它使用 1.8 寸 ST7735S 128×160 彩屏和四个实体按键，支持背包、历练、首领战、分支进化、Flash 存档，并能通过 USB 串口接收 Codex、Claude Code、OpenCode、CodeFree-O 的工作状态。AI 任务成功结束后，宠物会获得经验、灵石和少量灵力。

![首页 UI](assets/raw/ui/reference/home_hud_reference.png)

## 快速开始

需要：一块 RP2040-Zero、一块 1.8 寸 ST7735S 屏（带 K1–K4 按键）、USB-C 数据线、Windows + PowerShell。

```powershell
# 1. 安装工具链 + Python 依赖（仅首次）
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\bootstrap-arduino.ps1
py -3 -m pip install -r .\host\requirements-dev.txt

# 2. 编译并烧录固件（按实际 COM 口替换）
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload-firmware.ps1 -Port COM7

# 3. 跑 Python 测试确认规则一致
py -3 -m pytest .\host\game_model .\host\hooks .\host\diagnostics -q

# 4.（可选）把 AI Hook 装进 Codex / Claude Code / OpenCode / CodeFree-O
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\install-ai-hooks.ps1 -Port COM7
```

烧录后屏幕会显示首页。串口发送 `STATUS` 可查看宠物状态。接线见下方[接线表](#接线)。

## 架构概览

```text
AI 工具（Codex / Claude Code / OpenCode / CodeFree-O）
        │  事件钩子 → host/hooks/ai_pet_hook.py
        ▼  JSON over USB Serial (115200, ≤384B)
┌─────────────────────────────────────────────┐
│ 固件 firmware/ai_pet/（RP2040-Zero, C++）       │
│   GameApp ── GameState（规则 + 存档）            │
│           ├─ AiEventProtocol（解析串口 JSON）     │
│           ├─ SaveStore（LittleFS A/B 双槽 + CRC） │
│           ├─ GameUi（多页面渲染）                  │
│           └─ DisplayDevice（ST7735S + 背光）       │
└─────────────────────────────────────────────┘
        ▲  规则镜像
host/game_model/（Python 参考实现 + pytest）
```

**核心设计**：固件的 `game_state.cpp` 与 Python 的 `host/game_model/progression.py` 是同一套游戏规则的两种实现——改规则时两边都要改并保持测试通过。AI 任务成功后按耗时给宠物结算经验、灵石和灵力。

> 面向 AI 助手的开发指南见 [AGENTS.md](AGENTS.md)，包含文件职责、代码规范、关键不变量和常见任务流程。

## 功能

- 中国古风修仙主题和中文 UI。
- 云海仙台背景、麒麟宠物和美术化状态图标。
- 心境、体力、灵力、灵石、等级和经验。
- 背包：灵草、回春丹、符箓和秘境令。
- 功德簿：保存最近 10 条成功 AI 任务记录。
- 区域历练：青云山道和青竹灵境自动前进，事件自动结算，秘境令可直达当前区域 Boss。
- 自动 Boss 战：等级、形态、四类成长倾向、功法、符箓和区域宝物共同参与结算。
- 首领可以重复轮回挑战，难度递增，奖励每轮重新生成。
- 区域宝物：青云剑、灵竹玉佩等通关随机掉落，20 次保底。
- 四门功法（剑、丹、体、灵）可用对应倾向和灵石升级到 Lv9。
- 四维成长倾向（剑、丹、体、灵）影响战斗与进化分支。
- 历练事件随机顺序：每次出发 Fisher-Yates 洗牌。
- 7 种宠物形态和两次分支进化。
- LV3 从灵卵进化为幼年麒麟，LV12 最终化形。
- Codex、Claude Code、OpenCode、CodeFree-O Hook 状态页。
- AI 成功任务按设备本地耗时奖励，一次只跟踪一个活动任务。
- LittleFS 双槽存档、CRC 校验；V1.1/V1.2 均不迁移旧结构存档。
- SDL2 桌面模拟器：免烧录预览固件 UI。
- PowerShell 编译、烧录、Hook 安装脚本。
- Python 手动 Hook 控制器。

## 硬件

### 主控

- RP2040-Zero
- RP2040 双核 ARM Cortex-M0+
- 2MB Flash
- USB Type-C

### 屏幕

- 1.8 寸 ST7735S TFT
- 128×160
- SPI 接口
- 3.3V 电平
- 板载 K1～K4 四个按键

首版不需要 MicroSD、扬声器或额外传感器。

## 接线

| 屏幕信号 | RP2040-Zero |
|---|---:|
| VCC | 3V3 |
| GND | GND |
| SCL / SCK | GP2 |
| SDA / MOSI | GP3 |
| DC | GP4 |
| CS | GP5 |
| RST | GP6 |
| BLK | GP7 |
| K1 | GP8 |
| K2 | GP9 |
| K3 | GP10 |
| K4 | GP11 |

已验证参数：

- 屏幕初始化：`INITR_BLACKTAB`
- 方向：竖屏向上，按键位于屏幕右侧
- SPI：8MHz
- 按键空闲：HIGH
- 按键按下：LOW
- 背光：HIGH/PWM
- Flash：2MB，Sketch 1792KB，LittleFS 256KB

注意：

- 屏幕必须使用 3.3V 电平。
- 所有模块必须共地。
- 焊接完成后先检查 VCC 与 GND 是否短路。
- K1～K4 当前使用屏幕板载上拉，不需要额外电阻。

## 按键操作

| 按键 | 通用功能 | 首页功能 |
|---|---|---|
| K1 | 确认 | 功德簿 |
| K2 | 上移 | 背包 |
| K3 | 下移 | 历练 |
| K4 | 返回 | 状态 |

功德簿：

- K2 / K3：翻页，每页显示 2 条成功 AI 任务。
- K4：返回首页。

背包页：

- K2 / K3：选择物品。
- K1：使用灵草或回春丹；符箓在首领战前使用。
- K4：返回首页。

历练页：

- K1：开始青云山道历练；开始消耗 3 灵力。
- 自动前进每 3 秒消耗 1 灵力，事件在检查点自动结算。
- K4：结束历练并返回首页。
- 进度达到 100% 后可以挑战当前区域 Boss；持有秘境令时，入境前可选择直接推进到 Boss。

战斗页：

- 战前 K2 / K3 分别选择攻击符和护身符，K1 开战。
- 开战至少需要 5 灵力，每回合固定消耗 1 灵力。
- 战斗全自动进行，K4 可主动撤退。

状态页：

- K2 / K3：切换状态、战斗属性、修为资源和功法概览。
- 在“功法概览”页按 K1 进入功法修炼。
- 功法修炼页 K2 / K3 选择功法，K1 升级，K4 返回状态页。

## 成长和进化

经验升级采用三阶阈值，最高 LV30：

- LV1→LV2：20 经验。
- LV3→LV11：每级 50 经验。
- LV12→LV30：每级 100 经验。

```text
LV1～LV2  混沌灵卵
    ↓ LV3
凌霄麒麟 / 镇岳麒麟
    ↓ LV12
太虚剑仙 / 九转丹仙 / 不灭武仙 / 万灵仙尊
```

进化分支由四维成长倾向决定：

- 剑倾向：提高普通伤害和暴击率。来源于妖兽事件和 Codex 任务。
- 丹倾向：提高每回合伤害加成和捷径推进。来源于捷径事件和 Claude Code 任务。
- 体倾向：降低受到的伤害并提高闪避率。来源于 OpenCode 任务。
- 灵倾向：战斗中概率恢复体力，影响友善事件。来源于灵草、伤者事件和 CodeFree-O 任务。

LV3 时，剑+丹 ≥ 体+灵走凌霄路线，否则走镇岳路线。LV12 时，凌霄路线中剑 ≥ 丹化形为太虚剑仙，否则为九转丹仙；镇岳路线中体 ≥ 灵化形为不灭武仙，否则为万灵仙尊。

状态页会显示当前形态、等级、下一阶段和四类倾向数值。

### 功法修炼

V1.2 新增四门被动功法，索引顺序与四维倾向一致：

| 功法 | 对应倾向 | 主要效果 |
|---|---|---|
| 太虚剑诀 | 剑 | 提高伤害、暴击率、暴击伤害和破防 |
| 九转丹法 | 丹 | 提高灵草、回春丹回复量；Lv9 将被动恢复间隔从 5 分钟缩短到 3 分钟 |
| 不灭玄功 | 体 | 提供护体值、减伤和低体力减伤 |
| 万灵息法 | 灵 | 提高闪避和灵力续航；Lv9 大幅提高灵力上限 |

功法最高 Lv9。升级消耗对应倾向门槛和灵石，但不会扣除倾向数值，也不会影响 LV3/LV12 的进化分支。境界显示为：Lv1-Lv3 入门，Lv4-Lv6 小成，Lv7-Lv9 大成。

经验来源：

- 青云山道事件：妖兽胜利 6 经验。
- 通关奖励：`6 + min(轮数, 10) + max(0, 轮数 - 10) / 5` 经验。
- AI 成功任务：每分钟 2 经验，最少 1 分钟、最多 60 分钟。

灵石来源：

- 妖兽胜利：5 灵石。
- 通关奖励：`15 + 2 × min(轮数, 10) + max(0, 轮数 - 10)` 灵石。
- AI 成功任务：每分钟 1 灵石。

轮回挑战中每次通关奖励独立生成，不递减。难度随轮数递增，第 50 轮封顶。详见[游戏攻略](#游戏攻略)。

## 游戏攻略

### 初始状态

新角色起始数值：LV1、经验 0、体力 80、灵力 10、灵石 30、心境 70、青云轮次 1。灵力上限在灵卵阶段为 20，幼年麒麟阶段为 40，最终化形后为 80。

### 资源恢复

灵力和体力均通过被动计时恢复。默认每 5 分钟恢复 1 点灵力（上限取决于形态）和 5 点体力（上限 100）；九转丹法 Lv9 后恢复间隔缩短为 3 分钟。满值时不累计恢复时间，关机期间不补算。背包中使用灵草可立即恢复 3 点灵力，回春丹可立即恢复 20 点体力，九转丹法会提高两者的回复量。万灵息法 Lv9 会提高灵力上限。

### 区域历练

按 K3 进入历练页，先选择区域，再按 K1 进入区域。V1.2 开放青云山道和青竹灵境，云海剑台、丹霞药谷、玄河古战场为锁定预置区域。青竹灵境需要 LV5、100 灵石，并至少击败过一次青云妖狼。

普通出发消耗 3 点灵力，之后每 3 秒自动前进消耗 1 点灵力，进度增加 2%。灵力归零时自动结束历练，保留当前进度。若进入区域时持有秘境令且灵力足够，设备会提示是否消耗 1 枚秘境令直接挑战当前区域 Boss；拒绝后按普通历练前进。

在 25%、45%、65%、85% 四个检查点触发事件。每次出发时事件顺序随机洗牌，因此同一次历练中四个事件各出现一次但顺序不固定。

### 事件详解

**灵草**：自动采集成功，获得 1 个灵草物品，灵倾向 +1。

**妖兽**：根据实力判定。判定值 = 等级 + 体力/10 + 剑倾向/4 + 随机(0–9)，需达到 15。胜利获得 6 经验、5 灵石、剑倾向 +2；失败损失体力（基础 12，受轮数倍率缩放）。低等级时建议保持高体力再出发。

**受伤修士**：判定值 = 等级 + 体力/10 + 灵倾向/2 + 随机(0–9)，需达到 22。成功获得秘境令，未成功获得回春丹。两种结果都增加灵倾向 +2。

**山道捷径**：判定值 = 等级 + 灵力 + 丹倾向/3 + 随机(0–9)，需达到 17。成功获得丹倾向 +2 并额外推进 8–15% 进度；失败损失体力（基础 10，受轮数缩放）。高灵力时捷径成功率很高。

事件判定包含随机分量（0–9），因此同样的属性有时成功有时失败。等级和倾向越高，成功率越稳定。

### 首领挑战

进度达到 100% 后可挑战当前区域 Boss。青云山道 Boss 为青云妖狼，青竹灵境 Boss 为竹灵守卫。开战需要至少 5 点灵力，每回合消耗 1 点灵力。战斗全自动进行，约 2.2 秒一回合。

**玩家伤害**：基础 = 6 + 等级 + min(10, 剑倾向×3/8) + min(8, 丹倾向/3)。形态额外加成：幼年麒麟 A +1，最终形态 A 系 +2。暴击率 = 5 + min(20, 剑倾向/3)%，暴击伤害翻倍。攻击符使伤害 ×1.2，青云剑使伤害 ×1.1，两者可叠加。

**受到伤害**：基础 = max(1, 10 − min(6, 体倾向/5))。形态减伤：幼年麒麟 B −1，最终形态 B 系 −2。护身符使受伤 ×0.8，青云剑使受伤 ×0.9，两者可叠加。闪避率 = 5 + min(20, 体倾向/3)%，闪避时不受伤害。

**亲和回复**：每回合有 min(20, 灵倾向/3)% 概率恢复 2 点体力。

**战败**：体力归零时战斗失败，体力重置为 30，当前轮进度和事件记录重置，轮数不变。**撤退**（K4）：首领血量重置，轮数不变，进度重置。**灵力耗尽**：立即撤退，效果同主动撤退。

### 轮回挑战与难度

击败区域 Boss 后当前区域轮数 +1，进度和事件重置，从 0% 开始下一轮。每次通关独立生成奖励，不递减。

**难度缩放**（轮数上限 50）：

- 青云山道前 10 轮：生命每轮 +25%，伤害每轮 +15%。
- 青云山道第 11 轮起：生命每轮 +6%，伤害每轮 +4%。
- 青竹灵境基础 HP 90、基础伤害 17，难度成长高于青云山道。

| 轮次 | 生命倍率 | 伤害倍率 | 青云妖狼 HP（基础 60） |
|---:|---:|---:|---:|
| 1 | 100% | 100% | 60 |
| 5 | 200% | 160% | 120 |
| 10 | 325% | 235% | 195 |
| 20 | 385% | 275% | 231 |
| 50 | 565% | 395% | 339 |

### 通关奖励

每次击败青云妖狼获得：

- 经验：`6 + min(轮数, 10) + max(0, 轮数 − 10) / 5`（整数除法）。第 1 轮 7 经验，第 10 轮 16 经验，第 50 轮 24 经验。
- 灵石：`15 + 2 × min(轮数, 10) + max(0, 轮数 − 10)`。第 1 轮 17 灵石，第 10 轮 35 灵石，第 50 轮 75 灵石。
- 随机物品：1–2 种消耗品（灵草、回春丹、攻击符、护身符），每种数量 = `1 + min(4, (轮数 − 1) / 10)`。第 1 轮每种 1 个，第 41 轮起每种 5 个。
- 四维倾向各增加 `2 + min(3, 轮数 / 5)`。第 1–4 轮各 +2，第 15 轮起各 +5。

### 青云剑

青云剑是青云山道唯一的极品宝物，通关时独立判定掉落。

- 掉落概率 = min(10%, 当前轮数 × 1%)。第 1 轮 1%，第 10 轮起 10%。
- 连续 19 次未获得时，第 20 次必定获得（保底）。
- 只能获得一次。持有后不再进行掉落判定。
- 效果：永久伤害 +10%，永久减伤 +10%。

青云剑可以在背包的宝物页查看。未获得时显示灰色剪影。

### 区域宝物

每个区域都有独立宝物掉落、未掉落次数和 20 次保底。V1.2 已接入：

- 青云剑：青云山道掉落，永久伤害 +10%，永久减伤 +10%。
- 灵竹玉佩：青竹灵境掉落，永久闪避率 +10%。

后续锁定区域的宝物效果已预留在规则结构中，但完整区域内容不属于 V1.2 发布范围。

### 物品使用

| 物品 | 效果 | 使用场景 |
|---|---|---|
| 灵草 | 灵力 +3 | 背包页 K1 使用 |
| 回春丹 | 体力 +20 | 背包页 K1 使用 |
| 攻击符 | 本场伤害 ×1.2 | 首领战前选择 |
| 护身符 | 本场受伤 ×0.8 | 首领战前选择 |
| 秘境令 | 进入区域时可直达当前 Boss | 入境提示时 K1 使用 |

攻击符和护身符各消耗 1 个，在开战确认页通过 K2/K3 选择是否使用。战斗开始后不能再使用物品。

### AI 任务与养成

AI 工具（Codex、Claude Code、OpenCode、CodeFree-O）完成任务后，设备按本地运行时长结算：

- 经验 = 分钟数 × 2。
- 灵石 = 分钟数。
- 灵力 = max(1, 分钟数 / 2)。
- 倾向：根据来源工具增加对应倾向 +1 至 +4（按每 5 分钟一阶）。

最短按 1 分钟结算，最长 60 分钟。30 分钟超时自动减半结算。不同来源对应不同倾向：Codex → 剑，Claude Code → 丹，OpenCode → 体，CodeFree-O → 灵。

AI 任务是经验和灵石的主要来源，历练和通关只作为补充。长期使用同一工具会使对应倾向突出，从而影响进化路线。

### 养成建议

前期（LV1–LV5）：灵力上限低（20），每次历练只能走约 51 秒。尽量在灵力满时出发，利用灵草恢复灵力延长历练时间。妖兽胜利给经验和灵石，但需要等级 + 体力 + 剑倾向的总和达到判定阈值，低等级时有一定失败率。

中期（LV5–LV12）：化形为幼年麒麟后灵力上限升至 40，可以走完大部分历练。关注倾向累积方向，剑+丹 vs 体+灵的选择在 LV3 已经确定，LV12 的最终形态取决于单项倾向的高低。

后期（LV12+）：化形后灵力上限 80，可以稳定完成每次历练。重点转向轮回挑战——每轮通关都有完整的经验和灵石奖励，轮数越高奖励越丰厚。第 10 轮起青云剑掉落概率封顶 10%，耐心刷到保底即可。

进化路线提示：想要太虚剑仙需要长期堆剑倾向（多用 Codex、打赢妖兽）；想要万灵仙尊需要堆体+灵倾向（多用 OpenCode 和 CodeFree-O、走稳健路线）。没有绝对最优形态，每种形态提供不同的战斗加成。

## Arduino IDE 烧录

推荐环境：

- Arduino IDE 2.3.x
- Earle F. Philhower RP2040 Core
- Adafruit GFX Library
- Adafruit ST7735 and ST7789 Library
- U8g2 for Adafruit GFX

### 1. 安装 RP2040 Core

在 Arduino IDE 的“文件 → 首选项 → 其他开发板管理器地址”加入：

```text
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```

在开发板管理器中安装：

```text
Raspberry Pi Pico/RP2040 by Earle F. Philhower, III
```

### 2. 打开草图

```text
firmware/ai_pet/ai_pet.ino
```

### 3. 选择开发板

```text
开发板：Waveshare RP2040 Zero
Flash Size：2MB (Sketch: 1792KB, FS: 256KB)
```

不能选择 `2MB (no FS)`，否则存档无法工作。

### 4. 选择端口并上传

连接 RP2040 后选择新出现的 USB 串口，例如 `COM7`，然后点击“验证”和“上传”。

如果无法进入烧录模式：

1. 按住 `BOOT`。
2. 按一下 `RESET`。
3. 先松开 `RESET`，再松开 `BOOT`。
4. Windows 出现 `RPI-RP2` 磁盘。
5. 返回 Arduino IDE 再次上传。

完整说明见 [Arduino IDE 使用指南](docs/arduino-ide-guide.md)。

## PowerShell 编译和烧录

首次安装本地工具链：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File .\scripts\bootstrap-arduino.ps1

py -3 -m pip install -r .\host\requirements-dev.txt
```

编译：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File .\scripts\compile-firmware.ps1
```

烧录：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File .\scripts\upload-firmware.ps1 -Port COM7
```

最近一次验证结果：

```text
Sketch: 1037548 bytes / 1830912 bytes（56%）
RAM:    10492 bytes / 262144 bytes（4%）
```

## SDL2 桌面模拟器

模拟器可以在不烧录硬件的情况下预览固件 UI，方便开发和调试。

```text
simulator/
├── src/shims/          Arduino 和 GFX 的桌面替代层
├── src/backend/        SDL2 渲染和键盘输入
├── src/overrides/      替换硬件依赖的 .cpp（SaveStore 等）
└── CMakeLists.txt
```

构建需要 MSYS2 MinGW64 环境（SDL2、CMake、GCC）：

```powershell
# 安装依赖（MSYS2 终端内）
pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc

# 构建
MSYSTEM=MINGW64 "C:/msys64/usr/bin/bash.exe" -lc ^
  "cd simulator/build && cmake -G 'MinGW Makefiles' .. && mingw32-make -j8"
```

运行 `pet_simulator.exe` 前，需要在同目录放置 `SDL2.dll`、`libgcc_s_seh-1.dll`、`libstdc++-6.dll`、`libwinpthread-1.dll`。键盘 WASD 对应 K1–K4。

## AI 工具 Hook

Hook 会把 AI 工作过程映射为修仙状态：

| Hook 事件 | 设备行为 |
|---|---|
| `start` | 进入修炼页并由设备开始计时 |
| `end` | 同来源任务成功结算并返回首页 |

设备一次只跟踪一个任务。运行中收到新的 `start`，或收到其他来源的
`end`，都会忽略。30 分钟未收到结束事件时自动减半结算。

### 安装 Hook

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File .\scripts\install-ai-hooks.ps1 -Port COM7
```

安装器会：

- 将脚本复制到 `%USERPROFILE%\.ai-pet-hooks\`。
- 保存 COM 端口。
- 备份并增量修改 Claude Code 配置。
- 保留已有 CursorLight Codex 通知逻辑。
- 检测到 OpenCode 后安装插件。
- 检测到 CodeFree-O 后安装插件。

安装后重启 Codex、Claude Code、OpenCode 或 CodeFree-O。

### Python 手动触发

```powershell
cd $env:USERPROFILE\.ai-pet-hooks

py -3 .\ai_pet_hook.py start --source codex
py -3 .\ai_pet_hook.py end --source codex
```

指定来源和串口：

```powershell
py -3 .\ai_pet_hook.py start --source codefree-o --port COM7
py -3 .\ai_pet_hook.py end --source codefree-o --port COM7
```

注意：`end` 会真实修改设备存档并发放奖励。

完整说明见 [AI Hook 使用指南](docs/ai-hooks-guide.md)。

## 测试

### Python 测试

安装依赖：

```powershell
py -3 -m pip install -r .\host\requirements-dev.txt
```

执行：

```powershell
py -3 -m pytest .\host\game_model .\host\hooks .\host\diagnostics -q
```

测试范围包括：

- 属性、资源和能量恢复。
- 青云山道推进、双选事件和青云妖狼自动战斗。
- LV3、LV12 分支进化。
- 轮回挑战难度缩放和通关奖励。
- AI 事件 JSON 协议。
- 单活动任务、来源匹配和 30 分钟超时结算。
- UI 局部刷新结构。
- 串口诊断报告解析。

### 固件编译测试

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File .\scripts\compile-firmware.ps1
```

编译成功后检查 Flash 和 RAM 用量，避免接近上限。

### 实机 Hook 测试

1. 烧录最新固件。
2. 关闭 Arduino 串口监视器，避免占用 COM7。
3. 执行：

```powershell
cd $env:USERPROFILE\.ai-pet-hooks
py -3 .\ai_pet_hook.py start --source codex
py -3 .\ai_pet_hook.py end --source codex
```

预期：

- 屏幕依次显示“接引任务”“炼器中”“修炼完成”。
- 每条命令返回 JSON ACK。
- `end` 至少增加 2 经验和 1 灵石。

再次使用相同任务 ID 发送完成事件时，设备应返回 `duplicate`，且不重复奖励。

### 串口状态检查

打开 115200 波特率串口，发送：

```text
STATUS
```

设备返回类似：

```text
STATUS level=3 form=1 xp=48 mood=100 stamina=100 coins=69 energy=20 page=0
```

### 不修改存档地预览宠物形态

在串口监视器中发送：

```text
PREVIEW 0
PREVIEW 1
PREVIEW 2
PREVIEW 3
PREVIEW 4
PREVIEW 5
PREVIEW 6
PREVIEW OFF
```

`PREVIEW 0～6` 只临时改变首页和状态页的显示形态，不修改等级、经验、倾向和 Flash 存档。`PREVIEW OFF` 恢复真实形态，设备重启也会自动退出预览。

启用预览后，`STATUS` 会同时报告真实形态和预览形态：

```text
STATUS level=15 form=3 preview=1 preview_form=1 ...
```

### 修改存档数据（测试用）

在串口监视器中发送 `SET` 命令直接修改存档字段，修改后自动写入 Flash：

```text
SET level 15        # 等级
SET form 3          # 形态（0=灵卵 1-6=进化形态）
SET xp 200          # 经验值
SET energy 50       # 灵力
SET stamina 80      # 体力
SET mood 60         # 心境
SET coins 999       # 灵石
SET t0 20           # 剑倾向（t0=剑 t1=丹 t2=体 t3=灵）
SET progress 90     # 山道进度
SET boss 1          # 解锁妖狼（0/1）
SET round 5         # 青云轮次
SET sword 1         # 青云剑（0/1）
SET item0 5         # 灵草数量（item0-item4）
SET phase 0         # 历练阶段（0=空闲 1=前进 2=选择 3=结算 4=首领）
```

每条命令返回 `SET xxx=yyy ok`。发送 `RESET` 可重置全部存档：

```text
RESET
```

返回 `RESET ok`，宠物回到初始状态。

## 项目结构

```text
firmware/ai_pet/       Arduino 固件、游戏逻辑、UI、素材头文件
simulator/             SDL2 桌面模拟器（shims + backend + overrides）
host/game_model/       Python 规则模型和测试
host/hooks/            Codex、Claude Code、OpenCode Hook 与手动控制器
host/diagnostics/      串口诊断验收
assets/raw/            原始美术素材
assets/processed/      处理后的素材和预览
scripts/               工具链、编译、烧录、Hook 安装脚本
docs/                  硬件、Arduino、Hook 和设计文档
```

本地 `硬件/` 目录用于保存实物照片，不纳入 Git。

## 存档

- 存档位于 RP2040 的 256KB LittleFS 分区。
- 使用 A/B 双槽轮换写入。
- 每个存档包含版本、序列号和 CRC32。
- 保存失败时不会覆盖最后一个有效存档。
- V1.1 不兼容 V1.0 存档，首次启动时从头修炼。
- 最近任务摘要会保存，用于断电后继续防止重复奖励。

## 常见问题

### 找不到 COM7

- 检查 USB-C 线是否支持数据传输。
- 在设备管理器中查找 `USB 串行设备`。
- COM4、COM5 可能是蓝牙虚拟串口，不是 RP2040。
- 端口变化后重新运行 Hook 安装器并指定新端口。

### Hook 提示串口被占用

关闭 Arduino IDE 串口监视器、其他串口工具和旧的测试程序。

### 屏幕白屏

检查 3V3、GND、GP2～GP7 焊线，确认选择 `INITR_BLACKTAB` 和正确开发板。

### 按键无响应

检查 K1～K4 是否分别连接 GP8～GP11，并确认按下时电平为 LOW。

### 存档无法保存

确认 Flash Size 选择：

```text
2MB (Sketch: 1792KB, FS: 256KB)
```

不要选择 `2MB (no FS)`。

### 中文乱码

源文件必须使用 UTF-8。终端显示乱码不一定代表屏幕字库错误，应以实机显示为准。

## 已验证硬件状态

- ST7735S 128×160：通过。
- 屏幕方向和颜色：通过。
- K1～K4：通过。
- 背光 PWM：通过。
- LittleFS 写入、读取、校验、清理：通过。
- AI Hook ACK、任务奖励、防重复：通过。
- LV3 自动进化：通过。
- Codex、Claude Code、OpenCode、CodeFree-O Hook 联调：通过。
- 30 分钟诊断稳定性测试：通过。

## 后续计划

- 增加更多历练区域和首领。
- 扩充事件文本和养成内容。
- 互动系统和心境玩法。

版本变更见 [CHANGELOG.md](CHANGELOG.md)。
