# AI Pet RPG

基于 RP2040-Zero 与 ST7735S 四键屏的 AI 桌面宠物。

## Arduino IDE

日常开发使用 Arduino IDE 2.3.10。完整安装、编译和烧录步骤见：

[Arduino IDE 使用指南](docs/arduino-ide-guide.md)

主草图：

```text
firmware/ai_pet/ai_pet.ino
```

开发板菜单必须设置：

```text
Board: Waveshare RP2040 Zero
Flash Size: 2MB (Sketch: 1792KB, FS: 256KB)
```

## 辅助命令行工具

安装本地 Arduino CLI、RP2040 Core 和显示库：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\bootstrap-arduino.ps1
py -3 -m pip install -r .\host\requirements-dev.txt
```

编译：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

上传，其中 `COM端口` 替换为 Arduino IDE 中显示的实际端口：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload-firmware.ps1 -Port COM端口
```

## Hardware status

- ST7735S 128×160：已验证
- 背光 PWM：已验证
- K1–K4：已验证
- LittleFS 256KB：已验证
- 串口自动验收：通过
- 30 分钟稳定性测试：用户选择跳过

## 当前玩法

- 全中文修仙界面
- 程序化水墨洞府背景
- 培养：喂食、互动
- 历练：青竹灵境、云海剑台、玄岳古域
- 区域进度达到100%后挑战首领
- 战斗：攻击、法诀、丹药、防御
- 5级进入凌霄/镇岳分支，12级化形为四种青年仙人
