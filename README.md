# AI Pet RPG

基于 RP2040-Zero 与 ST7735S 四键屏的 AI 桌面宠物。

## Arduino IDE

日常开发使用 Arduino IDE 2.3.10。完整安装、编译和烧录步骤见：

[Arduino IDE 使用指南](docs/arduino-ide-guide.md)

主草图：

```text
firmware/ai_pet/ai_pet.ino
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

