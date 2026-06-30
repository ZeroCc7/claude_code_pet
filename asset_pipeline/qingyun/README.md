# 青云山道素材工作台

这个目录集中保存青云山道素材生成要用的提示词、转换脚本和操作说明。

## 输入素材

场景图：

```text
assets/raw/backgrounds/qingyun_scene.png
```

UI 图标九宫格：

```text
assets/raw/ui/qingyun_icons/icons_3x3.png
```

宠物小预览使用已有宠物帧：

```text
assets/processed/pets/firmware_preview/<form>/frame-1.png
```

## 提示词

```text
asset_pipeline/qingyun/prompts/qingyun_scene.prompt.txt
asset_pipeline/qingyun/prompts/qingyun_icons_3x3.prompt.txt
```

## 转换命令

在仓库根目录执行：

```powershell
py -3 .\asset_pipeline\qingyun\convert_qingyun_assets.py
```

兼容旧入口：

```powershell
py -3 .\scripts\convert_qingyun_assets.py
```

## 输出

预览图：

```text
assets/processed/backgrounds/qingyun_scene_128x54.png
assets/processed/ui/qingyun_icons/firmware_preview/
assets/processed/pets/qingyun_preview/
```

固件头文件：

```text
firmware/ai_pet/assets/qingyun_scene.h
firmware/ai_pet/assets/qingyun_ui_icons.h
firmware/ai_pet/assets/qingyun_pets.h
```

## 后续接素材

背景给干净横图，图标给透明或 #FF00FF 底九宫格，角色和 BOSS 给透明底单体图。详细规格见：

```text
docs/qingyun-asset-pipeline.md
```
