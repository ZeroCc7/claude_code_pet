# 宠物形态预览模式设计

## 目标

允许在不修改等级、经验、倾向和 Flash 存档的情况下，临时预览 7 种宠物形态，便于检查首页位置、尺寸、背景对比和按钮图标。

## 串口命令

```text
PREVIEW 0
PREVIEW 1
PREVIEW 2
PREVIEW 3
PREVIEW 4
PREVIEW 5
PREVIEW 6
PREVIEW OFF
```

形态编号：

| 编号 | 形态 |
|---:|---|
| 0 | 混沌灵卵 |
| 1 | 凌霄麒麟 |
| 2 | 镇岳麒麟 |
| 3 | 太虚剑仙 |
| 4 | 九转丹仙 |
| 5 | 不灭武仙 |
| 6 | 万灵仙尊 |

非法编号返回：

```text
PREVIEW error
```

成功启用返回：

```text
PREVIEW form=1
```

关闭返回：

```text
PREVIEW off
```

## 数据边界

- 预览形态只保存在 `GameUi` 的 RAM 字段中。
- 不调用 `GameState::mutableData()`。
- 不调用 `requestSave()`。
- 设备重启后自动恢复真实形态。
- 游戏升级、进化和奖励始终使用真实 `PetSaveData.form`。
- 预览只影响首页和状态页的形态显示；战斗与奖励逻辑不受影响。

## UI 数据流

- `GameApp` 解析 `PREVIEW` 命令。
- `GameUi::setPreviewForm()` 启用临时形态。
- `GameUi::clearPreviewForm()` 关闭临时形态。
- `GameUi::displayForm()` 在绘制时返回预览形态或真实形态。
- 首页宠物、状态页名称统一使用 `displayForm()`。

## STATUS

`STATUS` 增加预览信息：

```text
STATUS level=15 form=3 preview=1 preview_form=1 ...
```

关闭预览后：

```text
STATUS level=15 form=3 preview=0 preview_form=3 ...
```

其中 `form` 永远是真实存档形态。

## 验收

- `PREVIEW 0..6` 能逐个显示对应形态。
- `PREVIEW OFF` 恢复真实 LV15 形态。
- 预览期间发送 `STATUS`，真实 `form` 不变化。
- 断电重启后预览状态消失。
- 预览命令不触发存档写入。
- Python 测试和固件编译通过。

