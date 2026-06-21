# Arduino IDE 使用指南

## 已确认环境

```text
Arduino IDE：2.3.10
安装位置：C:\Program Files\Arduino IDE\Arduino IDE.exe
项目草图：G:\code\claude_code_pet\firmware\ai_pet\ai_pet.ino
```

## 第一次配置

### 1. 添加 RP2040 板卡地址

打开 Arduino IDE，进入：

```text
文件 -> 首选项
```

在“其他开发板管理器地址”加入：

```text
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```

如果原来已有其他地址，点击输入框右侧按钮，每行保留一个地址，不要覆盖原地址。

### 2. 安装 RP2040 Core

1. 打开左侧“开发板管理器”。
2. 搜索 `Raspberry Pi Pico/RP2040`。
3. 安装 `Raspberry Pi Pico/RP2040 by Earle F. Philhower, III`。
4. 安装结束后重启 Arduino IDE。

### 3. 安装显示库

打开左侧“库管理器”，依次安装：

```text
Adafruit GFX Library
Adafruit ST7735 and ST7789 Library
U8g2_for_Adafruit_GFX
```

如果 IDE 提示安装依赖，选择安装全部依赖。

## 打开项目

进入：

```text
文件 -> 打开
```

选择：

```text
G:\code\claude_code_pet\firmware\ai_pet\ai_pet.ino

## AI 工具联动

烧录游戏固件后，可运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File G:\code\claude_code_pet\scripts\install-ai-hooks.ps1 -Port COM7
```

详细说明见 `docs/ai-hooks-guide.md`。
```

Arduino IDE 会把同目录的 `.h`、`.cpp` 文件显示为顶部标签。

## 选择开发板

进入：

```text
工具 -> 开发板 -> Raspberry Pi RP2040 Boards -> Waveshare RP2040 Zero
```

不要选择 ESP32、Arduino Nano RP2040 Connect 或 Raspberry Pi Pico W。

随后进入：

```text
工具 -> Flash Size -> 2MB (Sketch: 1792KB, FS: 256KB)
```

不能选择 `2MB (no FS)`，否则宠物存档无法挂载。

## 连接和选择端口

1. 使用支持数据传输的 USB-C 线连接 RP2040-Zero。
2. 进入 `工具 -> 端口`。
3. 选择插入开发板后新出现的 COM 端口。

当前电脑中的 COM4、COM5 是蓝牙虚拟串口，不能用于烧录。开发板接入后应出现新的端口或 `RPI-RP2` 磁盘。

## 编译和上传

1. 点击左上角“验证”按钮编译。
2. 确认底部输出没有红色错误。
3. 点击“上传”按钮烧录。
4. 上传结束后打开右上角“串口监视器”。
5. 波特率选择 `115200`，行尾选择“新行”。

每次验收顺序：

```text
验证 -> 上传 -> 串口监视器 -> 检查实物屏幕和按键
```

## 首次无法上传

1. 按住 RP2040-Zero 的 `BOOT`。
2. 点按一次 `RESET`。
3. 先松开 `RESET`，再松开 `BOOT`。
4. Windows 出现 `RPI-RP2` 磁盘。
5. 返回 Arduino IDE 再点击“上传”。

## 常见故障

| 现象 | 处理 |
|---|---|
| 找不到 Waveshare RP2040 Zero | 检查附加网址，重新安装 Philhower RP2040 Core |
| `Adafruit_ST7735.h` 不存在 | 安装 `Adafruit ST7735 and ST7789 Library` |
| 没有新端口 | 更换支持数据的 USB-C 线并检查设备管理器 |
| 上传一直等待设备 | 使用 BOOT+RESET 进入 `RPI-RP2` 模式 |
| 串口监视器乱码 | 波特率选择 115200 |
| 屏幕白屏 | 检查 3V3、GND、SCL、SDA、CS、DC、RST 焊线 |
| 屏幕完全不亮 | 检查 BLK 接线和背光有效电平 |
