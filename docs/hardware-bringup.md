# Hardware Bring-up

本文档只记录实物验证结果。未实际验证的项目保持 `pending`。

## Wiring

| Signal | RP2040 GPIO | Verified |
|---|---:|---|
| SCL | GP2 | yes |
| SDA | GP3 | yes |
| DC | GP4 | yes |
| CS | GP5 | yes |
| RST | GP6 | yes |
| BLK | GP7 | yes |
| K1 | GP8 | yes |
| K2 | GP9 | yes |
| K3 | GP10 | yes |
| K4 | GP11 | yes |

## Measured behavior

- Display init profile: `INITR_BLACKTAB`
- Rotation: `0`，竖屏向上，按键位于屏幕右侧
- Color order: RGB 正常
- Pixel offset: 未见可见偏移
- Stable SPI frequency: 8MHz
- Button idle level: HIGH
- Button pressed level: LOW
- Button pin mode: `INPUT`，屏幕板载 4.7kΩ 电阻
- Backlight active polarity: HIGH/PWM 增亮
- Flash layout: 2MB，Sketch 1792KB，LittleFS 256KB
- Flash probe result: `mounted=1 wrote=1 verified=1 cleaned=1`

## Automated acceptance

```text
Port: COM7
Host tests: 3 passed
Result: ACCEPTANCE PASS
```

## Stability test

- 30-minute diagnostics soak: pending
