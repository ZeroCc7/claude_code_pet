# 美术素材自助生成与交付指南

本项目采用“人工生成原图、脚本确定性后处理、Codex 验收并集成”的协作方式。图片生成不由 Codex 执行，以减少 Token 消耗。

## 1. 分工

### 你负责

1. 根据素材任务表生成原始 PNG。
2. 使用 `generate2dsprite` 脚本去除洋红背景、切帧、对齐并生成 GIF。
3. 检查主体比例、锚点、跨格和动画连贯性。
4. 将通过检查的素材放入项目指定目录。

### Codex 负责

1. 给出素材名称、动作、帧数、网格、视角和目标路径。
2. 验收处理结果。
3. 使用项目 `scripts/convert_*.py` 转换为固件素材头文件。
4. 将素材接入固件渲染逻辑。

## 2. 原图硬性要求

- 背景必须为纯色 `#FF00FF`。
- 不得包含文字、数字、标签、边框、分格线或 UI。
- 同一动画中的主体身份、配色和身体比例必须一致。
- 主体保持相同像素尺寸和稳定的底部锚点。
- 主体只占每格中央约 60%～70%，四周保留洋红空白。
- 身体、尾巴、武器和特效不得跨越单元格边缘。
- 角色、宠物、首领等四帧动画默认使用 `2×2`，不要直接生成 `1×4`。
- 大范围刀光、投射物、命中特效应与角色身体动画分开生成。

## 3. 通用提示词模板

复制下列模板，并替换花括号内容：

```text
生成一张用于 RP2040 128×160 屏幕游戏的动画精灵表。

【画面规格】
- 素材类型：{角色 / 首领 / 宠物 / 特效 / 道具}
- 主体：{主体外形、颜色、服饰、识别特征}
- 动作：{待机 / 行走 / 攻击 / 受击 / 闪避 / 暴击}
- 视角：{3/4侧面 / 正侧面 / 俯视}
- 精灵表：严格2×2网格，共4帧
- 帧顺序：从左到右、从上到下
- 背景：100%纯洋红色 #FF00FF

【美术风格】
中国古风修仙题材，山海经神兽风格，16-bit掌机像素艺术。
青玉、墨黑、鎏金为主色，辅以少量象牙白、朱砂红和淡青灵光。
轮廓清晰，色块简洁，限制色彩数量，硬边像素。
不要抗锯齿，不要模糊，不要现代科技元素。

【动画要求】
- 四帧必须是同一个主体，身份、配色、服饰和身体比例完全一致
- 动作阶段依次为：{第1帧}、{第2帧}、{第3帧}、{第4帧}
- 每帧主体尺寸一致，中心位置稳定
- 双脚或身体底部保持在相同水平线上
- 主体只占每格中央约60%～70%
- 四周保留充足的纯洋红色空白
- 所有身体、尾巴、武器和光效必须完整位于各自格子内
- 任何内容都不能跨越格子边缘
- 不要添加分格线、边框、文字、标签、图标或UI
- 不要阴影背景、渐变背景或地面场景
```

## 4. 单个道具提示词模板

```text
生成一个中国古风修仙游戏道具图标。

道具：{灵草 / 回春丹 / 攻击符 / 护身符 / 青云信物}
外观：{材质、颜色、轮廓和识别特征}
视角：正面略带俯视
构图：单个道具居中，轮廓清晰
风格：16-bit掌机像素艺术，青玉、墨黑、鎏金配色
背景：100%纯洋红色 #FF00FF
主体占画面中央约60%，四周保留充足空白。

不要文字、数字、边框、UI、地面或投影。
无抗锯齿，无模糊，无渐变背景。
```

## 5. 后处理命令

四帧角色、宠物或首领：

```powershell
py -3 C:\Users\zero\.codex\skills\generate2dsprite\scripts\generate2dsprite.py process `
  --input .\raw-sheet.png `
  --target creature `
  --mode idle `
  --rows 2 `
  --cols 2 `
  --output-dir .\processed `
  --align bottom `
  --shared-scale `
  --component-mode largest `
  --reject-edge-touch
```

需要保留分离光效的特效素材，将 `--component-mode largest` 改为：

```text
--component-mode all
```

单个道具：

```powershell
py -3 C:\Users\zero\.codex\skills\generate2dsprite\scripts\generate2dsprite.py process `
  --input .\raw-item.png `
  --target asset `
  --mode single `
  --rows 1 `
  --cols 1 `
  --output-dir .\processed `
  --align center `
  --component-mode largest `
  --reject-edge-touch
```

## 6. 交付检查

处理目录应至少包含：

- 原始或清理后的精灵表。
- 透明背景精灵表。
- 独立帧 PNG。
- 动画 GIF（动画素材）。
- `pipeline-meta.json`。

交付前检查：

- `edge_touch_frames` 为空。
- 每帧主体大小基本一致。
- 宠物和角色脚底位置一致。
- 动画顺序正确且循环自然。
- 洋红残边不明显。
- 没有被错误删除的小部件。

如果角色身体因武器或特效变得明显小于待机图，应重新生成，并把大范围特效拆成单独素材。

## 7. 项目目录与命名

原始素材：

```text
assets/raw/<类别>/<名称>/<动作>_2x2.png
```

处理结果：

```text
assets/processed/<类别>/<名称>/<动作>-1.png
assets/processed/<类别>/<名称>/<动作>-2.png
assets/processed/<类别>/<名称>/<动作>-3.png
assets/processed/<类别>/<名称>/<动作>-4.png
assets/processed/<类别>/<名称>/<动作>-preview.gif
```

示例：

```text
assets/raw/bosses/qingyun_wolf/idle_2x2.png
assets/processed/bosses/qingyun_wolf/idle-1.png
assets/processed/bosses/qingyun_wolf/idle-preview.gif
```

不要直接修改 `firmware/ai_pet/assets/` 下的生成头文件。

## 8. 每批素材的任务表

Codex 后续按以下格式提出素材需求：

```text
素材：青云妖狼待机
格式：2×2，共4帧
视角：3/4侧面，朝左
锚点：底部
主体安全区：每格中央60%
背景：#FF00FF
原图路径：assets/raw/bosses/qingyun_wolf/idle_2x2.png
处理路径：assets/processed/bosses/qingyun_wolf/
```

## 9. V1.2 青竹灵境素材交付清单

V1.2 青竹灵境素材已由用户交付并接入固件。以下清单保留为后续复刻素材流程时的规格参考；素材替换时仍应检查规格、运行转换脚本、接入固件资源、编译和验证。

必需素材：

| 素材 | 用途 | 原图路径 | 固件目标 |
|---|---|---|---|
| 青云剑图标 | 宝物页展示区域 1 宝物 | `assets/raw/items/qingyun_sword.png` | 可并入区域宝物图标头文件 |
| 青竹灵境背景 | 历练主场景背景 | `assets/raw/backgrounds/bamboo_realm_scene.png` | `firmware/ai_pet/assets/bamboo_realm_scene.h` |
| 竹灵守卫 Boss | 青竹灵境 Boss 待机/战斗显示 | `assets/raw/bosses/bamboo_guardian_2x2.png` | `firmware/ai_pet/assets/bamboo_guardian.h` |
| 灵竹玉佩图标 | 宝物页展示区域 2 宝物 | `assets/raw/items/spirit_bamboo_jade.png` | 可并入区域宝物图标头文件 |

青云剑图标：

- PNG，单个道具居中。
- 背景为纯洋红 `#FF00FF` 或透明背景。
- 外观为青玉与银白色古风仙剑，带淡金云纹或剑穗。
- 轮廓清晰，小屏下能与灵竹玉佩、秘境令区分。
- 不包含文字、数字、边框、人物或大范围剑气特效。

青竹灵境背景：

- PNG，RGB 或 RGBA。
- 横向构图，比例接近 `128:54`。
- 推荐原图尺寸 `1280x540`，最低建议 `512x216`。
- 主题为青竹、灵雾、石径或竹林秘境。
- 不包含文字、UI、边框、水印、宠物、人物、Boss 或道具。
- 左侧或中间偏左留宠物站位，右侧留事件对象或 Boss 站位。
- 小屏下必须保持明暗层次，不能过亮导致浅色文字不可读。

竹灵守卫 Boss：

- PNG 精灵表，`2x2` 网格，共 4 帧。
- 背景为纯洋红 `#FF00FF` 或透明背景。
- 3/4 侧面或正侧面，适合放在 128x160 小屏右侧。
- 主体为竹木、青玉、灵光构成的守卫或灵兽，不使用现代机械元素。
- 四帧主体身份、比例、底部锚点和配色保持一致。
- 主体不跨格，不接触边缘，不包含文字、UI 或地面背景。

灵竹玉佩图标：

- PNG，单个道具居中。
- 背景为纯洋红 `#FF00FF` 或透明背景。
- 外观为青竹纹玉佩、竹叶吊坠或青玉牌。
- 轮廓清晰，小屏下能与青云剑、秘境令区分。
- 不包含文字、数字、边框或 UI。

可继续复用的素材：

- 四类事件图标继续复用 V1.1 青云图标。
- 宠物行走/站立素材继续复用现有青云宠物预览。
- 背包里的通用道具图标继续复用现有图标。
- 秘境令图标可沿用原青云信物图标，但 UI 文案应显示“秘境令”。
