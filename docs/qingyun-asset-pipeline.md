# 可直接接入烧录的青云山道素材交付规范

本文目的：指导你提供已经整理好的素材，让 Codex 不再做美术生成、抠图、裁剪、修图或重新设计，只做必要的固件格式转换、编译和烧录。

这里的“直接使用”指：

- 你给的图片已经符合构图、尺寸、透明背景、顺序和命名要求。
- Codex 不需要再判断画面怎么裁、不需要重新生成、不需要修图。
- Codex 只运行转换脚本，把 PNG 转成固件头文件，然后编译烧录。

不是指直接把 PNG 放进 RP2040。固件仍然需要把图片转成 RGB565 数组和透明 mask。

## 1. 最推荐的交付方式

把素材按下面路径放好，然后告诉 Codex：

```text
素材已放好，直接转换、编译、烧录。
```

推荐目录：

```text
assets/raw/backgrounds/qingyun_scene.png
assets/raw/ui/qingyun_icons/icons_3x3.png
assets/processed/pets/firmware_preview/<form>/frame-1.png
```

然后 Codex 执行：

```powershell
py -3 .\asset_pipeline\qingyun\convert_qingyun_assets.py
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload-firmware.ps1 -Port COM7
```

兼容旧入口仍可用：

```powershell
py -3 .\scripts\convert_qingyun_assets.py
```

## 2. 青云山道背景图交付标准

保存路径：

```text
assets/raw/backgrounds/qingyun_scene.png
```

格式：

```text
PNG
RGB 或 RGBA
横向构图
```

推荐尺寸：

```text
1280×540
```

最低建议：

```text
512×216
```

比例要求：

```text
约 2.37:1
也就是接近 128:54
```

画面要求：

- 不要文字。
- 不要 UI。
- 不要边框。
- 不要水印。
- 不要人物。
- 不要宠物。
- 不要怪物。
- 不要 BOSS。
- 不要 NPC。
- 不要道具。
- 左侧或中间偏左留出宠物站位。
- 右侧留出事件对象或怪物站位。
- 画面不要太碎，小屏会糊。
- 整体不要太亮，否则白字、浅青字不清楚。

转换后固件实际显示区域：

```text
128×54
```

也就是说，你给的背景最好已经按这个比例构图。这样 Codex 不需要帮你重新裁切判断。

## 3. 青云 UI 图标九宫格交付标准

保存路径：

```text
assets/raw/ui/qingyun_icons/icons_3x3.png
```

格式：

```text
PNG
正方形
3×3 九宫格
透明背景或纯 #FF00FF 洋红背景
```

推荐尺寸：

```text
900×900
1024×1024
1200×1200
```

九宫格顺序必须固定：

```text
1 灵草      2 回春丹    3 攻击符
4 护身符    5 青云信物  6 妖兽遭遇
7 受伤修士  8 山道捷径  9 灵力珠
```

每个图标要求：

- 单个主体居中。
- 不要文字。
- 不要数字。
- 不要标签。
- 不要边框。
- 不要网格线。
- 不要水印。
- 不要复杂投影。
- 不要大面积发光。
- 不要多个无关主体挤在一格里。
- 主体四周保留安全边距，不要贴边。

背景要求：

- 最好透明。
- 如果无法透明，使用纯洋红：

```text
#FF00FF
```

转换后固件实际图标尺寸：

```text
18×18
```

历险事件界面会按固件逻辑放大显示，所以原图不用做大图标，只要九宫格干净即可。

## 4. 青云历险宠物静态预览交付标准

当前青云历险里使用的是宠物静态预览图，不是完整运动图。

保存路径：

```text
assets/processed/pets/firmware_preview/<form>/frame-1.png
```

当前形态目录：

```text
egg
rookie_a
rookie_b
final_a1
final_a2
final_b1
final_b2
```

格式：

```text
PNG
透明背景
单只宠物
```

推荐尺寸：

```text
64×64
96×96
128×128
```

画面要求：

- 宠物完整，不裁角、不裁尾巴、不裁特效核心。
- 主体居中。
- 脚底或身体底部基本对齐。
- 不要文字。
- 不要边框。
- 不要水印。
- 不要背景。
- 不要地面。
- 不要大面积阴影。
- 不要复杂光效。

转换后固件基础尺寸：

```text
32×32
```

历险前进态会按固件逻辑放大到约：

```text
48×48
```

## 5. 后续宠物运动图交付记录

后续青云历险前进态应改用单独的宠物运动图，不长期使用静态图缩放。

建议未来交付规格：

```text
PNG
透明背景
2×2 帧表
共 4 帧
每帧同一只宠物
每帧底部对齐
无文字
无边框
无水印
```

动作建议：

```text
第 1 帧：站立
第 2 帧：身体轻微上移
第 3 帧：衣摆/毛发/尾巴轻动
第 4 帧：回到接近第 1 帧，方便循环
```

后续规则记录：

```text
灵卵阶段不开放青云历险。
第二形态及以上才可以进入青云山道。
```

## 6. 后续 BOSS 素材交付标准

当前 BOSS 素材暂未生成、暂未接入。

以后如果要接青云妖狼，建议保存为：

```text
assets/raw/bosses/qingyun_wolf.png
```

格式：

```text
PNG
透明背景
单只 BOSS
```

推荐尺寸：

```text
128×128
256×256
```

画面要求：

- 单只妖狼。
- 侧身或 3/4 朝向。
- 不要文字。
- 不要边框。
- 不要水印。
- 不要地面背景。
- 不要复杂烟雾。
- 不要满屏特效。
- 轮廓清楚，缩小后还能看出是狼。

未来固件预计显示尺寸：

```text
40×40 或 48×48
```

## 7. 一次性交付清单

如果你要一次性交付一批可以直接接入的青云素材，建议提供：

```text
assets/raw/backgrounds/qingyun_scene.png
assets/raw/ui/qingyun_icons/icons_3x3.png
assets/processed/pets/firmware_preview/egg/frame-1.png
assets/processed/pets/firmware_preview/rookie_a/frame-1.png
assets/processed/pets/firmware_preview/rookie_b/frame-1.png
assets/processed/pets/firmware_preview/final_a1/frame-1.png
assets/processed/pets/firmware_preview/final_a2/frame-1.png
assets/processed/pets/firmware_preview/final_b1/frame-1.png
assets/processed/pets/firmware_preview/final_b2/frame-1.png
```

如果只替换场景，就只给：

```text
assets/raw/backgrounds/qingyun_scene.png
```

如果只替换图标，就只给：

```text
assets/raw/ui/qingyun_icons/icons_3x3.png
```

## 8. 不能直接使用的素材

以下素材需要重新处理，不算“直接可用”：

- 截图。
- 带文字的图。
- 带水印的图。
- 带 UI 边框的图。
- 图标顺序不对的九宫格。
- 图标有网格线的九宫格。
- 角色或 BOSS 带背景。
- 角色被裁切。
- 主体贴边。
- 多个角色混在一张单体图里。
- 背景比例差太多，需要人工判断裁哪里。
- 图太写实，缩小到小屏后看不清。
- 光效、雾、渐变太多，RGB565 后糊成一片。

## 9. Codex 收到素材后的工作边界

如果素材符合本文规范，Codex 只做：

1. 运行转换脚本。
2. 检查生成的预览图是否存在。
3. 编译固件。
4. 烧录到 COM7。
5. 用 `STATUS` 确认设备响应。

Codex 不做：

- 重新生成图。
- 重新设计风格。
- 手动抠图。
- 手动修边。
- 手动补画。
- 猜测九宫格顺序。
- 判断复杂构图该裁哪里。

一句话：

```text
你给干净、透明、按路径和顺序放好的 PNG；Codex 负责转固件、编译、烧录。
```

