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
│  AI 工具（Codex / Claude Code / OpenCode）                    │
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
│                  ├─ GameState（规则 + 存档数据 PetSaveData）      │
│                  ├─ SaveStore（LittleFS A/B 双槽 + CRC32）       │
│                  ├─ GameUi（六页面渲染：首页/培养/历练/战斗/状态/修炼）│
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
| `game_state.{h,cpp}` | 游戏规则与 `PetSaveData`：培养、历练、战斗、经验、进化、AI 任务结算 |
| `game_types.h` | 枚举（`PetForm`/`UiPage`/`MeditationResult`）+ `PetSaveData` 结构 |
| `game_ui.{h,cpp}` | 全部 UI 渲染（六页面 + 局部刷新 + 反馈/通知/AI 状态页） |
| `ai_event_protocol.{h,cpp}` | 解析串口 JSON，校验 source/task_id/state |
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
| `hook_state.py` | 持久化任务状态（`~/.ai-pet-hooks/state.json`），生成 task_id、计算 duration |
| `claude-hook.ps1` / `codex-hook.ps1` / `opencode-hook.ps1` | 各工具的事件钩子 |
| `opencode-plugin.js` | OpenCode 插件 |
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
| `install-ai-hooks.ps1` | 把 Hook 装进 Claude/Codex/OpenCode 配置 |
| `convert_background.py` / `convert_pet_sprites.py` / `convert_ui_icons.py` | 素材转换 |

### 其他

- `docs/`：人类文档（hardware-bringup、arduino-ide-guide、ai-hooks-guide、art-generation-prompts）。
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
py -3 .\ai_pet_hook.py submitted --session smoke-test
py -3 .\ai_pet_hook.py editing   --session smoke-test
py -3 .\ai_pet_hook.py complete  --session smoke-test
```

`complete` 会真实修改设备存档并发放奖励。测试时务必用独立 `--session`，避免污染真实任务去重表。

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
2. **`PetSaveData` 是存档磁盘格式**：`game_types.h` 中的结构体字段顺序/类型即存档二进制布局。`save_store.cpp` 用 `kSaveMagic=0x50455431`、`kSaveVersion`（当前 4）、`size`、`sequence`、`crc32` 做校验。改结构必须 bump `kSaveVersion` 并写迁移逻辑（参考 `SaveStore::readSlot` 的迁移分支）。
3. **A/B 双槽**：`SaveStore` 轮流写入 A/B 两个文件，CRC 校验通过才视为有效，写入失败不覆盖最后有效存档。
4. **AI 协议上限 384 字节**：`AiEventProtocol::kMaximumMessageBytes=384`（`host/game_model/ai_protocol.py:32` 同步）。超长会被拒绝。
5. **任务去重**：`PetSaveData.recentTaskHashes[16]`（16 槽环形缓冲）记录最近任务哈希。同一 `source+taskId` 的成功事件只结算一次，重复返回 `duplicate`。固件 `applyAiTask()` 与 Python `hook_state.complete()` 都依赖此语义。
6. **奖励规则**：AI 成功任务按 `duration`（秒）结算，每分钟 2 经验，最少按 1 分钟、最多按 60 分钟；`duration` 在 hook 侧被 `max(60, min(3600, elapsed))` 钳制（`host/hooks/hook_state.py:70`）。失败/取消不结算奖励。
7. **引脚分配写死**：`board_config.h` 定义 GP2–GP11，改引脚必须同步更新 `docs/hardware-bringup.md` 和 README 接线表。
8. **固件 ↔ Python 规则必须一致**：`firmware/ai_pet/game_state.cpp` 与 `host/game_model/progression.py` 是同一规则的两种实现。改一处必须改另一处并更新对应 pytest。
9. **进化阈值固定**：LV3 第一次分支、LV12 最终分支，每 20 经验一级，最高 LV30。
10. **串口独占**：Hook 与 Arduino 串口监视器不能同时占用同一 COM 口。

## 7. 常见开发任务

### 新增一个 UI 页面

1. 在 `game_types.h` 的 `enum class UiPage` 增加枚举值。
2. 在 `game_ui.h/cpp` 实现 `drawXxx(const PetSaveData&)`，并在 `draw()` 按 `page_` 分发。
3. 在 `input_actions.h` 或 `game_app.cpp` 的 `processInput` 里接入按键导航。
4. 若有持久状态，加入 `PetSaveData`（注意 bump `kSaveVersion` + 迁移）。
5. 在 `host/game_model/` 加对应 Python 镜像与测试（若涉及规则）。

### 调整游戏数值规则

1. 同时改 `firmware/ai_pet/game_state.cpp` 和 `host/game_model/progression.py`。
2. 更新/新增 `host/game_model/test_progression.py` 用例。
3. 跑 pytest，再编译固件确认。
4. 若影响存档结构，bump `kSaveVersion` 并写迁移。

### 新增一种 AI 工作状态

1. 在 `firmware/ai_pet/ai_event_protocol.h` 的 `AiWorkState` 与状态名表加值。
2. 在 `host/game_model/ai_protocol.py` 的 `STATES` 与 `host/hooks/hook_state.py` 的 `STATES` 同步加值。
3. 在 `game_ui.cpp` 的 `showAiStatus` 文案表加对应中文。
4. 更新协议测试 `test_ai_protocol.py`。
5. 更新 README 的 Hook 状态表。

### 新增素材

1. 放原始图到 `assets/raw/`。
2. 跑对应 `scripts/convert_*.py`，产物落到 `firmware/ai_pet/assets/` 头文件。
3. 不要手改 `assets/` 下的生成头文件。

## 8. 验证清单（改完代码后）

- [ ] `py -3 -m pytest .\host\game_model .\host\hooks .\host\diagnostics -q` 全绿。
- [ ] `scripts\compile-firmware.ps1` 成功，Flash/RAM 未超限。
- [ ] 若改了规则，固件与 Python 两处数值一致。
- [ ] 若改了存档结构，`kSaveVersion` 已 bump 且有迁移。
- [ ] 若改了引脚，`board_config.h`、README 接线表、`docs/hardware-bringup.md` 三处同步。
- [ ] 若改了协议，固件 `.h` 与 Python `ai_protocol.py` 两处常量一致。
- [ ] 不要提交 `build/`、`tools/`、`__pycache__/`（已在 `.gitignore`）。

## 9. 硬件事实（无需重新验证）

- 屏幕：ST7735S 128×160，`INITR_BLACKTAB`，rotation 0，SPI 8MHz，RGB 正常。
- 按键：K1–K4 = GP8–GP11，空闲 HIGH，按下 LOW，板载 4.7kΩ 上拉。
- 背光：GP7，HIGH/PWM 增亮。
- RP2040-Zero：2MB Flash，USB Type-C。
- 已通过验收：屏幕、按键、背光 PWM、LittleFS 读写校验、AI Hook ACK/奖励/防重复、LV3 进化。
