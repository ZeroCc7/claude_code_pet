# 修炼动画素材管线

本文档描述修炼（cultivation/evolution）动画素材从原始图片到固件头文件的完整流程。任何 agent 拿到素材后可直接按本文执行，无需阅读源码。

## 1. 系统概述

修炼页面在 AI 任务进行期间显示宠物的修炼动画，替代普通待机动画。目前仅灵兽最终化形（FinalA1–FinalB2）支持独立修炼帧，灵兽阶段（Egg/RookieA/RookieB）修炼时仍使用待机帧。

架构层次：

```text
原始 2×2 网格 (raw)
    ↓ 手动或脚本切帧
4 张独立帧 (processed)
    ↓ convert_pet_sprites.py
C++ 头文件 (firmware/ai_pet/assets/pet_sprites.h)
    ↓ PetRenderer::draw(evolution=true)
屏幕显示
```

关键文件：

```text
scripts/convert_pet_sprites.py          转换脚本（idle + evolution 一起处理）
firmware/ai_pet/assets/pet_sprites.h    生成的 C++ 头文件（勿手改，用 #include "../game_types.h" 相对路径）
firmware/ai_pet/pet_renderer.h          PetRenderer 声明
firmware/ai_pet/pet_renderer.cpp        帧选择 + 渲染逻辑
firmware/ai_pet/game_ui.cpp             drawCultivation() 调用 PetRenderer
```

## 2. 素材目录与命名

### 2.1 原始网格图

```text
assets/raw/pets/<form>/cultivate_2x2.png
```

### 2.2 切帧后的独立帧

```text
assets/processed/pets/<form>/evolution-1.png
assets/processed/pets/<form>/evolution-2.png
assets/processed/pets/<form>/evolution-3.png
assets/processed/pets/<form>/evolution-4.png
assets/processed/pets/<form>/evolution-preview.gif   （可选，用于预览）
```

### 2.3 转换脚本生成的预览

```text
assets/processed/pets/firmware_preview/<form>/evolution-1.png
assets/processed/pets/firmware_preview/<form>/evolution-2.png
assets/processed/pets/firmware_preview/<form>/evolution-3.png
assets/processed/pets/firmware_preview/<form>/evolution-4.png
```

形态目录名：

```text
egg          混沌灵卵（不支持 evolution，修炼用 idle）
rookie_a     凌霄麒麟（不支持 evolution，修炼用 idle）
rookie_b     镇岳麒麟（不支持 evolution，修炼用 idle）
final_a1     太虚剑仙·凌霄
final_a2     九转丹仙·瑶华
final_b1     不灭武仙·镇岳
final_b2     万灵仙尊·清岚
```

## 3. 原始素材规格

### 3.1 网格图（cultivate_2x2.png）

```text
格式：PNG
色彩：RGB 或 RGBA
背景：纯洋红 #FF00FF（RGB）或透明（RGBA）
布局：严格 2×2 等分网格
帧序：左上=帧1、右上=帧2、左下=帧3、右下=帧4
```

灵兽形态（Egg/Rookie）每帧目标 48×48，网格图推荐 384×384 或更大。

最终仙人形态每帧目标 64×64，网格图推荐 512×512 或更大（实际交付约 1122×1402）。

画面要求：

- 不得包含文字、数字、标签、边框、分格线、水印。
- 四帧主体身份、配色、服饰、身体比例完全一致。
- 主体只占每格中央约 65% 安全区。
- 身体、角、尾巴、法器和特效不跨越格边缘。
- 灵气特效不遮挡脸部和主体轮廓。

### 3.2 切帧后的独立帧（evolution-N.png）

```text
格式：PNG
色彩：RGBA（必须含透明通道）
尺寸：128×128
背景：透明
```

主体居中，底部对齐，四周留安全边距。与 idle 帧相同规格。

## 4. 从网格图切帧

如果提供的是 2×2 网格图，需要先切帧。目前没有自动切帧脚本，手动流程如下。

### 4.1 Python 切帧命令

```python
from pathlib import Path
from PIL import Image

ROOT = Path("E:/WorkSpace/Zerocc/Private/claude_code_pet")
form = "final_a2"  # 替换为目标形态
source = Image.open(ROOT / f"assets/raw/pets/{form}/cultivate_2x2.png").convert("RGB")
cell_w = source.width // 2
cell_h = source.height // 2
cells = [
    (0, 0, cell_w, cell_h),                       # 帧1 左上
    (cell_w, 0, source.width, cell_h),             # 帧2 右上
    (0, cell_h, cell_w, source.height),            # 帧3 左下
    (cell_w, cell_h, source.width, source.height), # 帧4 右下
]
out = ROOT / f"assets/processed/pets/{form}"
for i, box in enumerate(cells):
    cell = source.crop(box).convert("RGBA")
    # 去洋红
    for y in range(cell.height):
        for x in range(cell.width):
            r, g, b, _ = cell.getpixel((x, y))
            if r > 210 and b > 180 and g < 90:
                cell.putpixel((x, y), (r, g, b, 0))
    # 裁切到主体边界框
    bbox = cell.getchannel("A").getbbox()
    if bbox is None:
        continue
    subject = cell.crop(bbox)
    # 缩放到 128×128
    target = 128
    scale = min(target / subject.width, target / subject.height)
    new_size = (max(1, round(subject.width * scale)), max(1, round(subject.height * scale)))
    subject = subject.resize(new_size, Image.Resampling.NEAREST)
    frame = Image.new("RGBA", (target, target), (0, 0, 0, 0))
    frame.alpha_composite(subject, ((target - new_size[0]) // 2, (target - new_size[1]) // 2))
    frame.save(out / f"evolution-{i+1}.png")
```

也可以用外部工具（如 generate2dsprite）切帧，只要输出符合 128×128 RGBA 透明背景即可。

### 4.2 直接提供切好的帧

如果素材已经是 4 张独立帧，直接放到 `assets/processed/pets/<form>/evolution-{1,2,3,4}.png`，跳过切帧步骤。

## 5. 运行转换脚本

素材就位后执行：

```powershell
py -3 .\scripts\convert_pet_sprites.py
```

脚本行为：

1. 处理所有形态的 idle 帧（idle-1.png ~ idle-4.png），生成 `kPet_<form>_frames[]`。
2. 对每个 final form，检查 `evolution-{1,2,3,4}.png` 是否存在。
   - 存在：用 evolution 帧生成 `kPet_<form>_evolution_frames[]`。
   - 不存在：用 idle 帧作为 fallback，仍然生成 `kPet_<form>_evolution_frames[]`（内容与 idle 相同）。
3. 生成 `kEvolutionForms[]` 查找表，记录哪些形态有真实 evolution 数据。
4. 输出到 `firmware/ai_pet/assets/pet_sprites.h`。

生成产物：

```text
firmware/ai_pet/assets/pet_sprites.h                              C++ 头文件（自动覆盖）
assets/processed/pets/firmware_preview/<form>/frame-N.png         idle 预览
assets/processed/pets/firmware_preview/<form>/evolution-N.png     evolution 预览
```

## 6. 固件渲染逻辑

### 6.1 PetRenderer::draw() 参数

```cpp
void PetRenderer::draw(
    Adafruit_GFX& target,
    PetForm form,
    int16_t x, int16_t y,
    uint32_t now,
    PetEffect effect = PetEffect::None,
    uint32_t effectElapsed = 0,
    bool evolution = false    // 新增参数
);
```

帧选择逻辑：

1. 根据 form 选择 idle 帧数组 `kPet_<form>_frames`。
2. 若 `evolution == true` 且 `form >= PetForm::FinalA1`，切换到 `kPet_<form>_evolution_frames`。
3. 帧索引 = `(now / 400) % 4`，即每 400ms 切一帧，4 帧一个循环。
4. effect 层独立渲染，不受 evolution 参数影响。

### 6.2 drawCultivation() 调用

```cpp
// game_ui.cpp drawCultivation() 中：
pet_.draw(tft, form, petX, petY, now, petEffect_, effectElapsed, finalForm);
```

`finalForm` 为 `form >= PetForm::FinalA1`。灵兽阶段（非 final）传 false，使用 idle 帧。

### 6.3 头文件中的关键常量

```cpp
constexpr uint8_t kPetSpriteWidth = 62;       // 灵兽帧宽
constexpr uint8_t kPetSpriteHeight = 62;      // 灵兽帧高
constexpr uint8_t kFinalPetSpriteWidth = 64;  // 仙人帧宽
constexpr uint8_t kFinalPetSpriteHeight = 82; // 仙人帧高
constexpr uint8_t kPetSpriteFrameCount = 4;   // 动画帧数

constexpr uint8_t kEvolutionFormCount = N;    // 有真实 evolution 数据的形态数
constexpr PetForm kEvolutionForms[] = {...};  // 形态枚举列表
```

## 7. 添加新形态的 evolution 素材

步骤：

1. 将 4 张独立帧放入 `assets/processed/pets/<form>/evolution-{1,2,3,4}.png`。
   - 格式：128×128 PNG RGBA 透明背景。
   - 命名必须为 `evolution-1.png` 到 `evolution-4.png`。

2. 运行转换脚本：
   ```powershell
   py -3 .\scripts\convert_pet_sprites.py
   ```

3. 验证：
   - 检查 `firmware/ai_pet/assets/pet_sprites.h` 中是否包含 `kPet_<form>_evolution_frames[]`。
   - 检查 `kEvolutionForms[]` 数组是否包含该形态。
   - 检查 `assets/processed/pets/firmware_preview/<form>/evolution-{1,2,3,4}.png` 预览是否正确。

4. 编译固件确认（Arduino IDE）：
   - FQBN：`rp2040:rp2040:waveshare_rp2040_zero:flash=2097152_262144`
   - 确认 Flash/RAM 未超限。

5. 烧录验收：
   - 触发 AI 任务进入修炼页，确认播放 evolution 动画而非 idle。
   - 4 帧循环流畅，灵气变化有层次感。

不需要改任何 C++ 代码。PetRenderer 和 drawCultivation 已支持所有 final form 的 evolution 帧。

## 8. 验证清单

- [ ] `evolution-{1,2,3,4}.png` 存在于 `assets/processed/pets/<form>/`。
- [ ] 4 帧均为 128×128 RGBA 透明背景。
- [ ] `py -3 scripts/convert_pet_sprites.py` 无报错。
- [ ] `pet_sprites.h` 包含 `kPet_<form>_evolution_frames[]`。
- [ ] `kEvolutionForms[]` 包含该形态的 PetForm 枚举。
- [ ] `firmware_preview/<form>/evolution-{1,2,3,4}.png` 预览正确（64×82）。
- [ ] `py -3 -m pytest .\host\game_model .\host\hooks .\host\diagnostics -q` 全绿。
- [ ] Arduino IDE 编译成功，Flash/RAM 未超限。
- [ ] 实机修炼页播放 evolution 动画。

## 9. 常见问题

**Q: evolution 帧和 idle 帧有什么区别？**
idle 是普通待机循环，evolution 是修炼专用循环（灵气聚集、法阵展开等）。修炼页面自动选择 evolution 帧（仅 final form），其他页面始终使用 idle 帧。

**Q: 灵兽形态（Egg/Rookie）能用 evolution 帧吗？**
不能。当前 PetRenderer 只在 `form >= PetForm::FinalA1` 时切换 evolution 帧。灵兽阶段修炼时仍显示 idle 帧 + 特效层。如果未来需要灵兽修炼动画，需要扩展 PetRenderer 的逻辑。

**Q: 只提供了部分形态的 evolution 素材怎么办？**
没关系。转换脚本对没有 evolution 文件的形态自动使用 idle 帧作为 fallback。`kEvolutionForms[]` 只记录有真实数据的形态。等新素材到位后重新跑脚本即可。

**Q: 生成的头文件很大怎么办？**
正常。每组 evolution 帧（64×82 RGB565 + mask）约 42KB，4 组约 168KB。RP2040 有 2MB Flash，固件代码约 200KB，空间充足。如果 Flash 紧张，可以减少已支持的 evolution 形态数量。

**Q: 修炼动画播放速度怎么调？**
在 `pet_renderer.cpp` 的 `draw()` 方法中，帧切换公式为 `(now / 400) % 4`。改 400 为更大值则更慢，更小值则更快。evolution 和 idle 共用同一个帧率参数。
