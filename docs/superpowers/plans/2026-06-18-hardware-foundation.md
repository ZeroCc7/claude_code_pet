# AI 宠物硬件自检与固件底座 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 建立可重复编译和烧录的 RP2040-Zero 固件工程，完成 ST7735S、背光、四键、USB CDC 和板载 Flash 的硬件自检，并留下后续游戏系统可复用的驱动接口。

**Architecture:** 固件使用 Arduino-Pico Core，将硬件访问封装为 `DisplayDevice`、`ButtonScanner`、`FlashProbe`，由非阻塞的 `DiagnosticsApp` 组织自检页面。电脑端只提供工具链脚本、串口验收脚本和编译检查；这一阶段不实现宠物养成、战斗或 AI Hook。

**Tech Stack:** Arduino C++、Arduino-Pico Core、Adafruit GFX、Adafruit ST7735、LittleFS、USB CDC、PowerShell、Python 3、pyserial、pytest

---

## 项目拆分

完整项目分成四个独立实施计划：

1. 本计划：硬件自检与固件底座。
2. 本地宠物游戏：页面、培养、探索、Boss、进化和正式素材。
3. 主机状态桥：串口协议、持久队列、奖励事件和重连。
4. 工具适配器：Codex、Claude Code、OpenCode。

本计划结束时，设备必须可脱离后续系统独立完成硬件验收。

## 文件结构

```text
claude_code_pet/
├─ .gitignore
├─ README.md
├─ firmware/
│  └─ ai_pet/
│     ├─ ai_pet.ino                  程序入口和对象装配
│     ├─ board_config.h              唯一引脚与显示参数来源
│     ├─ display_device.h/.cpp       ST7735S、背光和测试画面
│     ├─ button_scanner.h/.cpp       四键消抖、长按和原始电平
│     ├─ flash_probe.h/.cpp          LittleFS 测试文件读写校验
│     └─ diagnostics_app.h/.cpp      非阻塞自检状态机与串口输出
├─ host/
│  ├─ requirements-dev.txt
│  └─ diagnostics/
│     ├─ serial_acceptance.py        自动读取并验证自检报告
│     └─ test_serial_acceptance.py   主机端协议单元测试
├─ scripts/
│  ├─ bootstrap-arduino.ps1          安装本地 Arduino CLI/Core/库
│  ├─ compile-firmware.ps1           固定参数编译并报告资源占用
│  └─ upload-firmware.ps1            指定 COM 口上传
└─ docs/
   └─ hardware-bringup.md            焊接、通电和实测结果记录
```

## Task 1: 建立可复现工具链

**Files:**
- Create: `.gitignore`
- Create: `host/requirements-dev.txt`
- Create: `scripts/bootstrap-arduino.ps1`
- Create: `scripts/compile-firmware.ps1`
- Create: `scripts/upload-firmware.ps1`
- Create: `README.md`

- [ ] **Step 1: 写入工具链验收测试**

创建 `scripts/compile-firmware.ps1`：

```powershell
param(
    [string]$Cli = "$PSScriptRoot\..\tools\arduino-cli\arduino-cli.exe"
)

$ErrorActionPreference = "Stop"
$root = Resolve-Path "$PSScriptRoot\.."
$sketch = Join-Path $root "firmware\ai_pet"
$build = Join-Path $root "build\firmware"

if (-not (Test-Path $Cli)) {
    throw "Arduino CLI not found: $Cli. Run scripts\bootstrap-arduino.ps1 first."
}

New-Item -ItemType Directory -Force $build | Out-Null
& $Cli compile `
    --fqbn "rp2040:rp2040:waveshare_rp2040_zero" `
    --warnings all `
    --output-dir $build `
    $sketch

if ($LASTEXITCODE -ne 0) {
    throw "Firmware compile failed with exit code $LASTEXITCODE"
}
```

- [ ] **Step 2: 运行测试并确认当前失败**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

Expected: FAIL，提示 `Arduino CLI not found`。

- [ ] **Step 3: 写入工具链安装脚本和依赖**

创建 `scripts/bootstrap-arduino.ps1`：

```powershell
$ErrorActionPreference = "Stop"
$root = Resolve-Path "$PSScriptRoot\.."
$toolDir = Join-Path $root "tools\arduino-cli"
$zipPath = Join-Path $env:TEMP "arduino-cli-windows.zip"
$cli = Join-Path $toolDir "arduino-cli.exe"

New-Item -ItemType Directory -Force $toolDir | Out-Null

if (-not (Test-Path $cli)) {
    $release = Invoke-RestMethod `
        -Headers @{ "User-Agent" = "claude-code-pet-bootstrap" } `
        "https://api.github.com/repos/arduino/arduino-cli/releases/latest"
    $asset = $release.assets |
        Where-Object { $_.name -match '^arduino-cli_.*_Windows_64bit\.zip$' } |
        Select-Object -First 1
    if (-not $asset) {
        throw "Arduino CLI Windows asset not found"
    }
    Invoke-WebRequest $asset.browser_download_url -OutFile $zipPath
    Expand-Archive $zipPath $toolDir -Force
}

& $cli config init --overwrite
& $cli config add board_manager.additional_urls `
    "https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json"
& $cli core update-index
& $cli core install "rp2040:rp2040"
& $cli lib install "Adafruit GFX Library"
& $cli lib install "Adafruit ST7735 and ST7789 Library"
& $cli version
& $cli core list
& $cli lib list
```

创建 `host/requirements-dev.txt`：

```text
pyserial>=3.5,<4
pytest>=8,<10
```

创建 `scripts/upload-firmware.ps1`：

```powershell
param(
    [Parameter(Mandatory = $true)][string]$Port,
    [string]$Cli = "$PSScriptRoot\..\tools\arduino-cli\arduino-cli.exe"
)

$ErrorActionPreference = "Stop"
$root = Resolve-Path "$PSScriptRoot\.."
$sketch = Join-Path $root "firmware\ai_pet"

& $Cli upload `
    --fqbn "rp2040:rp2040:waveshare_rp2040_zero" `
    --port $Port `
    $sketch

if ($LASTEXITCODE -ne 0) {
    throw "Firmware upload failed with exit code $LASTEXITCODE"
}
```

创建 `.gitignore`：

```text
build/
tools/
.pytest_cache/
__pycache__/
*.pyc
*.uf2
*.elf
*.bin
```

在 `README.md` 写明：

````markdown
# AI Pet RPG

RP2040-Zero + ST7735S 四键屏桌面宠物。

## 初始化

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\bootstrap-arduino.ps1
py -3 -m pip install -r .\host\requirements-dev.txt
```

## 编译

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

## 上传

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload-firmware.ps1 -Port COM5
```
````

- [ ] **Step 4: 安装工具链并验证版本**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\bootstrap-arduino.ps1
py -3 -m pip install -r .\host\requirements-dev.txt
.\tools\arduino-cli\arduino-cli.exe board listall | Select-String "Waveshare RP2040 Zero"
```

Expected: 输出 `Waveshare RP2040 Zero`，且无安装错误。

- [ ] **Step 5: 提交**

```powershell
git add .gitignore README.md host/requirements-dev.txt scripts
git commit -m "build: add reproducible RP2040 toolchain"
```

## Task 2: 固化板级配置并建立最小可编译固件

**Files:**
- Create: `firmware/ai_pet/board_config.h`
- Create: `firmware/ai_pet/ai_pet.ino`

- [ ] **Step 1: 写入最小入口并故意引用缺失配置**

创建 `firmware/ai_pet/ai_pet.ino`：

```cpp
#include "board_config.h"

void setup() {
  Serial.begin(board::kUsbBaud);
  pinMode(board::kBacklightPin, OUTPUT);
  digitalWrite(board::kBacklightPin, LOW);
}

void loop() {
}
```

- [ ] **Step 2: 编译并确认失败**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

Expected: FAIL，错误包含 `board_config.h: No such file or directory`。

- [ ] **Step 3: 写入唯一板级配置**

创建 `firmware/ai_pet/board_config.h`：

```cpp
#pragma once

#include <Arduino.h>

namespace board {
constexpr uint8_t kTftSckPin = 2;
constexpr uint8_t kTftMosiPin = 3;
constexpr uint8_t kTftDcPin = 4;
constexpr uint8_t kTftCsPin = 5;
constexpr uint8_t kTftResetPin = 6;
constexpr uint8_t kBacklightPin = 7;
constexpr uint8_t kButtonPins[] = {8, 9, 10, 11};
constexpr size_t kButtonCount = 4;

constexpr uint16_t kScreenWidth = 128;
constexpr uint16_t kScreenHeight = 160;
constexpr uint32_t kUsbBaud = 115200;
constexpr uint32_t kInitialSpiHz = 8000000;
constexpr uint8_t kBacklightInitial = 0;
constexpr uint8_t kBacklightNormal = 180;

static_assert(kButtonCount == 4, "UI requires exactly four buttons");
static_assert(kTftSckPin != kTftMosiPin, "SPI pins must be unique");
}
```

- [ ] **Step 4: 编译并确认通过**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

Expected: PASS，并生成 `build/firmware/ai_pet.ino.uf2`。

- [ ] **Step 5: 提交**

```powershell
git add firmware/ai_pet
git commit -m "feat: add RP2040 board configuration"
```

## Task 3: 实现屏幕与背光自检

**Files:**
- Create: `firmware/ai_pet/display_device.h`
- Create: `firmware/ai_pet/display_device.cpp`
- Modify: `firmware/ai_pet/ai_pet.ino`

- [ ] **Step 1: 在入口中调用尚不存在的显示驱动**

将 `ai_pet.ino` 改为：

```cpp
#include "display_device.h"

DisplayDevice display;

void setup() {
  Serial.begin(board::kUsbBaud);
  display.begin();
  display.drawColorBars();
}

void loop() {
}
```

- [ ] **Step 2: 编译并确认失败**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

Expected: FAIL，错误包含 `display_device.h: No such file or directory`。

- [ ] **Step 3: 实现显示驱动**

创建 `firmware/ai_pet/display_device.h`：

```cpp
#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#include "board_config.h"

class DisplayDevice {
 public:
  DisplayDevice();
  void begin();
  void setBacklight(uint8_t value);
  void drawSolid(uint16_t color, const char* label);
  void drawColorBars();
  void drawGrid();
  Adafruit_ST7735& raw();

 private:
  Adafruit_ST7735 tft_;
};
```

创建 `firmware/ai_pet/display_device.cpp`：

```cpp
#include "display_device.h"

DisplayDevice::DisplayDevice()
    : tft_(board::kTftCsPin, board::kTftDcPin, board::kTftResetPin) {}

void DisplayDevice::begin() {
  pinMode(board::kBacklightPin, OUTPUT);
  analogWrite(board::kBacklightPin, board::kBacklightInitial);

  SPI.setSCK(board::kTftSckPin);
  SPI.setTX(board::kTftMosiPin);
  SPI.begin();

  tft_.initR(INITR_BLACKTAB);
  tft_.setSPISpeed(board::kInitialSpiHz);
  tft_.setRotation(0);
  tft_.fillScreen(ST77XX_BLACK);
  setBacklight(board::kBacklightNormal);
}

void DisplayDevice::setBacklight(uint8_t value) {
  analogWrite(board::kBacklightPin, value);
}

void DisplayDevice::drawSolid(uint16_t color, const char* label) {
  tft_.fillScreen(color);
  tft_.setTextColor(color == ST77XX_WHITE ? ST77XX_BLACK : ST77XX_WHITE);
  tft_.setTextSize(2);
  tft_.setCursor(4, 4);
  tft_.print(label);
}

void DisplayDevice::drawColorBars() {
  constexpr uint16_t colors[] = {
      ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE, ST77XX_WHITE, ST77XX_BLACK};
  constexpr int count = sizeof(colors) / sizeof(colors[0]);
  const int barWidth = board::kScreenWidth / count;
  for (int i = 0; i < count; ++i) {
    tft_.fillRect(i * barWidth, 0, barWidth, board::kScreenHeight, colors[i]);
  }
}

void DisplayDevice::drawGrid() {
  tft_.fillScreen(ST77XX_BLACK);
  for (int x = 0; x < board::kScreenWidth; x += 8) {
    tft_.drawFastVLine(x, 0, board::kScreenHeight, ST77XX_BLUE);
  }
  for (int y = 0; y < board::kScreenHeight; y += 8) {
    tft_.drawFastHLine(0, y, board::kScreenWidth, ST77XX_GREEN);
  }
  tft_.drawRect(0, 0, board::kScreenWidth, board::kScreenHeight, ST77XX_RED);
}

Adafruit_ST7735& DisplayDevice::raw() {
  return tft_;
}
```

- [ ] **Step 4: 编译并烧录屏幕测试**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload-firmware.ps1 -Port COM5
```

Expected: 编译上传成功；屏幕显示五色竖条，背光点亮，无持续白屏。

- [ ] **Step 5: 记录实测初始化参数**

若颜色顺序、旋转或边缘偏移不正确，只允许调整：

```cpp
tft_.initR(INITR_BLACKTAB);
tft_.setRotation(0);
```

在 `docs/hardware-bringup.md` 记录最终 `initR` 参数、rotation、颜色顺序和边缘偏移；不得把未经实测的参数写成已确认。

- [ ] **Step 6: 提交**

```powershell
git add firmware/ai_pet docs/hardware-bringup.md
git commit -m "feat: add ST7735 display diagnostics"
```

## Task 4: 实现四键扫描、消抖和有效电平探测

**Files:**
- Create: `firmware/ai_pet/button_scanner.h`
- Create: `firmware/ai_pet/button_scanner.cpp`
- Modify: `firmware/ai_pet/ai_pet.ino`
- Modify: `docs/hardware-bringup.md`

- [ ] **Step 1: 在入口中引用缺失的按键扫描器**

在 `ai_pet.ino` 中增加：

```cpp
#include "button_scanner.h"

ButtonScanner buttons;
```

并在 `setup()` 调用 `buttons.begin()`，在 `loop()` 调用 `buttons.update(millis())`。

- [ ] **Step 2: 编译并确认失败**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

Expected: FAIL，错误包含 `button_scanner.h: No such file or directory`。

- [ ] **Step 3: 实现按键扫描器**

创建 `firmware/ai_pet/button_scanner.h`：

```cpp
#pragma once

#include <Arduino.h>

#include "board_config.h"

enum class ButtonEvent : uint8_t { None, Pressed, Released, LongPressed };

struct ButtonState {
  bool rawLevel;
  bool pressed;
  bool longSent;
  uint32_t changedAt;
  uint32_t pressedAt;
  ButtonEvent event;
};

class ButtonScanner {
 public:
  void begin();
  void update(uint32_t now);
  const ButtonState& state(size_t index) const;
  void setActiveLevel(bool activeLevel);
  bool activeLevel() const;

 private:
  static constexpr uint32_t kDebounceMs = 25;
  static constexpr uint32_t kLongPressMs = 800;
  bool activeLevel_ = LOW;
  ButtonState states_[board::kButtonCount]{};
};
```

创建 `firmware/ai_pet/button_scanner.cpp`：

```cpp
#include "button_scanner.h"

void ButtonScanner::begin() {
  for (size_t i = 0; i < board::kButtonCount; ++i) {
    pinMode(board::kButtonPins[i], INPUT);
    const bool level = digitalRead(board::kButtonPins[i]);
    states_[i] = {level, level == activeLevel_, false, millis(), 0,
                  ButtonEvent::None};
  }
}

void ButtonScanner::update(uint32_t now) {
  for (size_t i = 0; i < board::kButtonCount; ++i) {
    ButtonState& s = states_[i];
    s.event = ButtonEvent::None;
    const bool raw = digitalRead(board::kButtonPins[i]);

    if (raw != s.rawLevel) {
      s.rawLevel = raw;
      s.changedAt = now;
    }

    const bool nextPressed = raw == activeLevel_;
    if (nextPressed != s.pressed && now - s.changedAt >= kDebounceMs) {
      s.pressed = nextPressed;
      s.longSent = false;
      if (nextPressed) {
        s.pressedAt = now;
        s.event = ButtonEvent::Pressed;
      } else {
        s.event = ButtonEvent::Released;
      }
    }

    if (s.pressed && !s.longSent && now - s.pressedAt >= kLongPressMs) {
      s.longSent = true;
      s.event = ButtonEvent::LongPressed;
    }
  }
}

const ButtonState& ButtonScanner::state(size_t index) const {
  return states_[index];
}

void ButtonScanner::setActiveLevel(bool activeLevel) {
  activeLevel_ = activeLevel;
}

bool ButtonScanner::activeLevel() const {
  return activeLevel_;
}
```

- [ ] **Step 4: 编译并烧录原始电平测试**

在入口中每 100ms 输出一行：

```cpp
Serial.printf("BUTTON_RAW %d %d %d %d\n",
              buttons.state(0).rawLevel,
              buttons.state(1).rawLevel,
              buttons.state(2).rawLevel,
              buttons.state(3).rawLevel);
```

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload-firmware.ps1 -Port COM5
```

Expected: 每个按键按下时只有对应一位改变，松手后恢复。

- [ ] **Step 5: 固化有效电平**

根据实测把：

```cpp
bool activeLevel_ = LOW;
```

保留为 `LOW` 或改为 `HIGH`，并在 `docs/hardware-bringup.md` 记录空闲电平、按下电平和是否需要 `INPUT_PULLUP`。若输入悬空，则将 `pinMode(..., INPUT)` 改为 `INPUT_PULLUP` 并重新测试。

- [ ] **Step 6: 提交**

```powershell
git add firmware/ai_pet docs/hardware-bringup.md
git commit -m "feat: add debounced four-button input"
```

## Task 5: 实现 LittleFS 非破坏性 Flash 探测

**Files:**
- Create: `firmware/ai_pet/flash_probe.h`
- Create: `firmware/ai_pet/flash_probe.cpp`
- Modify: `firmware/ai_pet/ai_pet.ino`

- [ ] **Step 1: 引用尚不存在的 Flash 探测器**

在入口中增加：

```cpp
#include "flash_probe.h"

FlashProbe flashProbe;
```

并在 `setup()` 中输出 `flashProbe.run()` 的结果。

- [ ] **Step 2: 编译并确认失败**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

Expected: FAIL，错误包含 `flash_probe.h: No such file or directory`。

- [ ] **Step 3: 实现写入、回读、校验和清理**

创建 `firmware/ai_pet/flash_probe.h`：

```cpp
#pragma once

#include <Arduino.h>

struct FlashProbeResult {
  bool mounted;
  bool wrote;
  bool verified;
  bool cleaned;
};

class FlashProbe {
 public:
  FlashProbeResult run();
};
```

创建 `firmware/ai_pet/flash_probe.cpp`：

```cpp
#include "flash_probe.h"

#include <LittleFS.h>

FlashProbeResult FlashProbe::run() {
  constexpr char kPath[] = "/diagnostic.tmp";
  constexpr uint8_t kPayload[] = {
      0x43, 0x4C, 0x41, 0x55, 0x44, 0x45, 0x50, 0x45, 0x54};

  FlashProbeResult result{};
  result.mounted = LittleFS.begin();
  if (!result.mounted) {
    return result;
  }

  File out = LittleFS.open(kPath, "w");
  result.wrote = out && out.write(kPayload, sizeof(kPayload)) == sizeof(kPayload);
  out.close();
  if (!result.wrote) {
    LittleFS.remove(kPath);
    return result;
  }

  File in = LittleFS.open(kPath, "r");
  result.verified = in && in.size() == sizeof(kPayload);
  for (size_t i = 0; result.verified && i < sizeof(kPayload); ++i) {
    result.verified = in.read() == kPayload[i];
  }
  in.close();

  result.cleaned = LittleFS.remove(kPath);
  return result;
}
```

- [ ] **Step 4: 编译、烧录并验证**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload-firmware.ps1 -Port COM5
```

Expected serial line:

```text
FLASH mounted=1 wrote=1 verified=1 cleaned=1
```

- [ ] **Step 5: 提交**

```powershell
git add firmware/ai_pet
git commit -m "feat: add LittleFS flash probe"
```

## Task 6: 组织非阻塞硬件自检状态机

**Files:**
- Create: `firmware/ai_pet/diagnostics_app.h`
- Create: `firmware/ai_pet/diagnostics_app.cpp`
- Modify: `firmware/ai_pet/ai_pet.ino`

- [ ] **Step 1: 将入口切换到尚不存在的 DiagnosticsApp**

将 `ai_pet.ino` 改为：

```cpp
#include "diagnostics_app.h"

DiagnosticsApp app;

void setup() {
  app.begin();
}

void loop() {
  app.update(millis());
}
```

- [ ] **Step 2: 编译并确认失败**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

Expected: FAIL，错误包含 `diagnostics_app.h: No such file or directory`。

- [ ] **Step 3: 实现状态机接口**

创建 `firmware/ai_pet/diagnostics_app.h`：

```cpp
#pragma once

#include "button_scanner.h"
#include "display_device.h"
#include "flash_probe.h"

class DiagnosticsApp {
 public:
  void begin();
  void update(uint32_t now);

 private:
  enum class Page : uint8_t { Red, Green, Blue, White, Black, Bars, Grid, Keys };
  void showPage(Page page);
  void printButtonEvents();
  void printReady();

  DisplayDevice display_;
  ButtonScanner buttons_;
  FlashProbe flashProbe_;
  Page page_ = Page::Red;
  uint32_t pageStartedAt_ = 0;
  uint32_t lastKeyDrawAt_ = 0;
};
```

创建 `firmware/ai_pet/diagnostics_app.cpp`，实现以下确定行为：

```cpp
#include "diagnostics_app.h"

namespace {
constexpr uint32_t kPageMs = 1200;
}

void DiagnosticsApp::begin() {
  Serial.begin(board::kUsbBaud);
  const uint32_t waitStarted = millis();
  while (!Serial && millis() - waitStarted < 2000) {
    delay(10);
  }

  Serial.println("DIAG boot");
  display_.begin();
  buttons_.begin();

  const FlashProbeResult flash = flashProbe_.run();
  Serial.printf("FLASH mounted=%d wrote=%d verified=%d cleaned=%d\n",
                flash.mounted, flash.wrote, flash.verified, flash.cleaned);

  showPage(page_);
  pageStartedAt_ = millis();
}

void DiagnosticsApp::update(uint32_t now) {
  buttons_.update(now);
  printButtonEvents();

  if (page_ != Page::Keys && now - pageStartedAt_ >= kPageMs) {
    page_ = static_cast<Page>(static_cast<uint8_t>(page_) + 1);
    pageStartedAt_ = now;
    showPage(page_);
    if (page_ == Page::Keys) {
      printReady();
    }
  }

  if (page_ == Page::Keys && now - lastKeyDrawAt_ >= 50) {
    lastKeyDrawAt_ = now;
    Adafruit_ST7735& tft = display_.raw();
    tft.fillRect(0, 24, board::kScreenWidth, 100, ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    for (size_t i = 0; i < board::kButtonCount; ++i) {
      tft.setCursor(8, 28 + static_cast<int>(i) * 22);
      tft.printf("K%d: %s", static_cast<int>(i + 1),
                 buttons_.state(i).pressed ? "DOWN" : "UP");
    }
  }
}

void DiagnosticsApp::showPage(Page page) {
  switch (page) {
    case Page::Red: display_.drawSolid(ST77XX_RED, "RED"); break;
    case Page::Green: display_.drawSolid(ST77XX_GREEN, "GREEN"); break;
    case Page::Blue: display_.drawSolid(ST77XX_BLUE, "BLUE"); break;
    case Page::White: display_.drawSolid(ST77XX_WHITE, "WHITE"); break;
    case Page::Black: display_.drawSolid(ST77XX_BLACK, "BLACK"); break;
    case Page::Bars: display_.drawColorBars(); break;
    case Page::Grid: display_.drawGrid(); break;
    case Page::Keys: display_.drawSolid(ST77XX_BLACK, "KEY TEST"); break;
  }
}

void DiagnosticsApp::printButtonEvents() {
  for (size_t i = 0; i < board::kButtonCount; ++i) {
    const ButtonEvent event = buttons_.state(i).event;
    if (event == ButtonEvent::None) {
      continue;
    }
    const char* name = event == ButtonEvent::Pressed
                           ? "pressed"
                           : event == ButtonEvent::Released
                                 ? "released"
                                 : "long";
    Serial.printf("BUTTON k=%d event=%s\n", static_cast<int>(i + 1), name);
  }
}

void DiagnosticsApp::printReady() {
  Serial.println("DIAG ready");
}
```

- [ ] **Step 4: 编译并烧录完整自检**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload-firmware.ps1 -Port COM5
```

Expected:

- 屏幕依次显示红、绿、蓝、白、黑、色条、网格、按键页。
- 串口出现 `DIAG boot`、成功的 `FLASH` 行和 `DIAG ready`。
- K1 至 K4 均产生 `pressed`、`released`，长按产生 `long`。

- [ ] **Step 5: 提交**

```powershell
git add firmware/ai_pet
git commit -m "feat: add non-blocking hardware diagnostics"
```

## Task 7: 建立主机端串口自动验收

**Files:**
- Create: `host/diagnostics/serial_acceptance.py`
- Create: `host/diagnostics/test_serial_acceptance.py`

- [ ] **Step 1: 写失败的报告解析测试**

创建 `host/diagnostics/test_serial_acceptance.py`：

```python
from serial_acceptance import DiagnosticReport


def test_report_requires_boot_flash_and_ready():
    report = DiagnosticReport()
    report.consume("DIAG boot")
    report.consume("FLASH mounted=1 wrote=1 verified=1 cleaned=1")
    report.consume("DIAG ready")

    assert report.complete
    assert report.errors == []


def test_report_rejects_failed_flash_verification():
    report = DiagnosticReport()
    report.consume("DIAG boot")
    report.consume("FLASH mounted=1 wrote=1 verified=0 cleaned=1")
    report.consume("DIAG ready")

    assert not report.complete
    assert report.errors == ["flash probe failed"]
```

- [ ] **Step 2: 运行测试并确认失败**

Run:

```powershell
py -3 -m pytest .\host\diagnostics\test_serial_acceptance.py -q
```

Expected: FAIL，错误包含 `ModuleNotFoundError: No module named 'serial_acceptance'`。

- [ ] **Step 3: 实现解析器和串口入口**

创建 `host/diagnostics/serial_acceptance.py`：

```python
from __future__ import annotations

import argparse
import time

import serial


class DiagnosticReport:
    def __init__(self) -> None:
        self.saw_boot = False
        self.saw_flash = False
        self.saw_ready = False
        self.errors: list[str] = []

    def consume(self, line: str) -> None:
        line = line.strip()
        if line == "DIAG boot":
            self.saw_boot = True
        elif line.startswith("FLASH "):
            self.saw_flash = True
            required = ("mounted=1", "wrote=1", "verified=1", "cleaned=1")
            if not all(item in line for item in required):
                self.errors.append("flash probe failed")
        elif line == "DIAG ready":
            self.saw_ready = True

    @property
    def complete(self) -> bool:
        return (
            self.saw_boot
            and self.saw_flash
            and self.saw_ready
            and not self.errors
        )


def collect(port: str, timeout: float) -> DiagnosticReport:
    report = DiagnosticReport()
    deadline = time.monotonic() + timeout
    with serial.Serial(port, 115200, timeout=0.25) as device:
        while time.monotonic() < deadline and not report.complete:
            raw = device.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", errors="replace").strip()
            print(line)
            report.consume(line)
    return report


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--timeout", type=float, default=20.0)
    args = parser.parse_args()

    report = collect(args.port, args.timeout)
    if report.complete:
        print("ACCEPTANCE PASS")
        return 0
    print(f"ACCEPTANCE FAIL errors={report.errors}")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 4: 运行单元测试**

Run:

```powershell
py -3 -m pytest .\host\diagnostics\test_serial_acceptance.py -q
```

Expected: `2 passed`。

- [ ] **Step 5: 对真实设备运行自动验收**

先按一次 RP2040-Zero RESET，再运行：

```powershell
py -3 .\host\diagnostics\serial_acceptance.py --port COM5 --timeout 20
```

Expected: 输出 `ACCEPTANCE PASS`，退出码为 0。

- [ ] **Step 6: 提交**

```powershell
git add host/diagnostics
git commit -m "test: add serial hardware acceptance"
```

## Task 8: 完成焊接记录与稳定性验收

**Files:**
- Modify: `docs/hardware-bringup.md`
- Modify: `README.md`

- [ ] **Step 1: 完成硬件记录**

`docs/hardware-bringup.md` 必须包含以下实际测量字段，不允许凭推测填写：

```markdown
# Hardware Bring-up

## Wiring

| Signal | RP2040 GPIO | Verified |
|---|---:|---|
| SCL | GP2 | yes/no |
| SDA | GP3 | yes/no |
| DC | GP4 | yes/no |
| CS | GP5 | yes/no |
| RST | GP6 | yes/no |
| BLK | GP7 | yes/no |
| K1 | GP8 | yes/no |
| K2 | GP9 | yes/no |
| K3 | GP10 | yes/no |
| K4 | GP11 | yes/no |

## Measured behavior

- Display init profile:
- Rotation:
- Color order:
- Pixel offset:
- Stable SPI frequency:
- Button idle level:
- Button pressed level:
- Button pin mode:
- Backlight active polarity:
- Flash probe result:
```

- [ ] **Step 2: 运行完整软件检查**

Run:

```powershell
py -3 -m pytest .\host\diagnostics -q
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
git diff --check
```

Expected: pytest 全部通过、固件编译成功、`git diff --check` 无输出。

- [ ] **Step 3: 运行 30 分钟硬件稳定性测试**

烧录自检固件，让按键页持续局部刷新 30 分钟。期间每个按键至少短按 20 次、长按 5 次。

验收条件：

- 无白屏、错位、随机色块。
- USB 串口没有异常断开。
- 每次按键只产生一次 `pressed` 和一次 `released`。
- 长按只产生一次 `long`。
- RP2040 没有复位。

- [ ] **Step 4: 更新 README 的已验证硬件状态**

只记录真实通过项目，例如：

```markdown
## Hardware status

- ST7735S 128×160: verified
- Backlight PWM: verified
- K1-K4: verified
- LittleFS read/write: verified
- 30-minute diagnostics soak: passed
```

- [ ] **Step 5: 最终验证并提交**

Run:

```powershell
py -3 -m pytest .\host\diagnostics -q
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
git status --short
```

Expected: 测试和编译通过；状态中只出现准备提交的 README 与硬件记录。

```powershell
git add README.md docs/hardware-bringup.md
git commit -m "docs: record verified hardware bring-up"
```

## 阶段完成条件

只有以下条件全部满足，才进入“本地宠物游戏”实施计划：

- 工具链可由脚本在新环境安装。
- 固件可命令行编译并上传。
- 屏幕方向、颜色、偏移和 SPI 频率均经实物确认。
- 四个按键的有效电平、消抖和长按均经实物确认。
- 背光 PWM 正常。
- LittleFS 写入、回读和清理成功。
- Python 串口自动验收通过。
- 连续运行 30 分钟无显示、按键、串口或复位故障。
