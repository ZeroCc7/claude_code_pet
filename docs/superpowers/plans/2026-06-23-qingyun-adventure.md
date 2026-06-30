# 青云山道与自动战斗 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将旧版三秘境与四键手动战斗替换为完整的青云山道自动历练、双选事件、青云妖狼自动战斗和结算流程。

**Architecture:** Python `GameState` 先定义可重复测试的青云山道状态机与战斗公式，固件 `GameState` 保持字段、边界和随机种子算法一致。`GameApp` 只负责按固定时间驱动历练与战斗回合，`GameUi` 负责按状态显示场景、选项和结果；正式美术到位前只使用现有宠物绘制、色块、线条和简单几何图形，素材替换不得影响规则层。

**Tech Stack:** Python 3、pytest、Arduino C++、Earle Philhower RP2040 Core、Adafruit GFX、LittleFS A/B 存档

---

## 文件结构

- `host/game_model/progression.py`：青云山道事件、自动历练、妖狼战斗及确定性随机规则的参考实现。
- `host/game_model/test_progression.py`：所有规则、资源边界、倾向加成和物品效果测试。
- `firmware/ai_pet/game_types.h`：持久化枚举、历练状态、战斗状态和 V1.1 存档字段。
- `firmware/ai_pet/game_state.{h,cpp}`：与 Python 一致的规则实现，不处理时间和绘制。
- `firmware/ai_pet/game_app.{h,cpp}`：历练周期、战斗回合周期、保存和 UI 刷新调度。
- `firmware/ai_pet/game_ui.{h,cpp}`：青云山道、事件选择、战前确认、自动战斗和结果页面。
- `host/game_model/test_ui_refresh.py`：固件接口、页面状态和无正式素材依赖的源码镜像测试。
- `CHANGELOG.md`、`README.md`：V1.1 操作方式和当前进度。

---

### Task 1: Python 青云山道状态机

**Files:**
- Modify: `host/game_model/progression.py`
- Modify: `host/game_model/test_progression.py`

- [ ] **Step 1: 写历练启动、周期消耗和退出的失败测试**

```python
def test_qingyun_adventure_starts_for_three_energy_and_advances_one_step():
    state = GameState(energy=5)

    assert state.start_qingyun_adventure()
    assert state.energy == 2
    assert state.adventure_phase == AdventurePhase.ADVANCING

    result = state.tick_qingyun_adventure(seed=0)

    assert result == AdventureTick.ADVANCED
    assert state.energy == 1
    assert state.qingyun_progress == 2


def test_qingyun_adventure_stops_at_zero_energy_and_keeps_progress():
    state = GameState(energy=4, qingyun_progress=20)
    assert state.start_qingyun_adventure()

    assert state.tick_qingyun_adventure(seed=0) == AdventureTick.ENERGY_DEPLETED
    assert state.energy == 0
    assert state.qingyun_progress == 22
    assert state.adventure_phase == AdventurePhase.IDLE


def test_event_choice_phase_pauses_energy_consumption():
    state = GameState(
        energy=8,
        qingyun_progress=24,
        adventure_phase=AdventurePhase.CHOOSING,
        current_event=QingyunEvent.SPIRIT_HERB,
    )

    assert state.tick_qingyun_adventure(seed=9) == AdventureTick.WAITING_FOR_CHOICE
    assert state.energy == 8
    assert state.qingyun_progress == 24
```

- [ ] **Step 2: 运行目标测试并确认失败**

Run: `py -3 -m pytest .\host\game_model\test_progression.py -q`

Expected: FAIL，缺少 `AdventurePhase`、`AdventureTick`、`QingyunEvent` 或青云山道 API。

- [ ] **Step 3: 增加最小状态与推进实现**

在 `progression.py` 增加：

```python
class AdventurePhase(IntEnum):
    IDLE = 0
    ADVANCING = 1
    CHOOSING = 2
    RESULT = 3
    BOSS_READY = 4


class AdventureTick(IntEnum):
    INACTIVE = 0
    ADVANCED = 1
    EVENT_TRIGGERED = 2
    WAITING_FOR_CHOICE = 3
    ENERGY_DEPLETED = 4
    BOSS_UNLOCKED = 5


class QingyunEvent(IntEnum):
    NONE = 0
    SPIRIT_HERB = 1
    DEMON_BEAST = 2
    WOUNDED_CULTIVATOR = 3
    SHORTCUT = 4
```

给 `GameState` 增加 `qingyun_progress`、`adventure_phase`、`current_event`、`current_event_result`、`qingyun_event_mask`、`qingyun_boss_unlocked`，并实现：

```python
def start_qingyun_adventure(self) -> bool:
    if self.adventure_phase not in (AdventurePhase.IDLE,
                                    AdventurePhase.BOSS_READY):
        return False
    if self.energy < 3:
        return False
    self.energy -= 3
    self.adventure_phase = AdventurePhase.ADVANCING
    self.current_event = QingyunEvent.NONE
    return True


def stop_qingyun_adventure(self) -> None:
    self.adventure_phase = (
        AdventurePhase.BOSS_READY
        if self.qingyun_boss_unlocked else AdventurePhase.IDLE
    )
    self.current_event = QingyunEvent.NONE


def tick_qingyun_adventure(self, seed: int) -> AdventureTick:
    if self.adventure_phase == AdventurePhase.CHOOSING:
        return AdventureTick.WAITING_FOR_CHOICE
    if self.adventure_phase != AdventurePhase.ADVANCING:
        return AdventureTick.INACTIVE
    self.energy -= 1
    self.qingyun_progress = min(100, self.qingyun_progress + 2)
    if self.qingyun_progress >= 100:
        self.qingyun_boss_unlocked = True
        self.adventure_phase = AdventurePhase.BOSS_READY
        return AdventureTick.BOSS_UNLOCKED
    if self.energy == 0:
        self.stop_qingyun_adventure()
        return AdventureTick.ENERGY_DEPLETED
    return AdventureTick.ADVANCED
```

- [ ] **Step 4: 重跑测试并确认通过**

Run: `py -3 -m pytest .\host\game_model\test_progression.py -q`

Expected: PASS。

- [ ] **Step 5: 提交**

```powershell
git add host/game_model/progression.py host/game_model/test_progression.py
git commit -m "feat: add qingyun adventure state machine"
```

---

### Task 2: Python 双选事件与信物解锁

**Files:**
- Modify: `host/game_model/progression.py`
- Modify: `host/game_model/test_progression.py`

- [ ] **Step 1: 写四种事件、选择暂停和结果边界测试**

```python
def test_progress_checkpoint_triggers_each_qingyun_event_once():
    state = GameState(energy=20, qingyun_progress=23,
                      adventure_phase=AdventurePhase.ADVANCING)

    assert state.tick_qingyun_adventure(seed=0) == AdventureTick.EVENT_TRIGGERED
    assert state.current_event == QingyunEvent.SPIRIT_HERB
    assert state.adventure_phase == AdventurePhase.CHOOSING
    assert state.qingyun_event_mask & 0b0001


def test_collecting_spirit_herb_adds_inventory_and_resumes():
    state = GameState(
        adventure_phase=AdventurePhase.CHOOSING,
        current_event=QingyunEvent.SPIRIT_HERB,
    )

    result = state.resolve_qingyun_event(choice=0, seed=0)

    assert result == EventResult.ITEM_GAINED
    assert state.items[ItemType.SPIRIT_HERB] == 1
    assert state.adventure_phase == AdventurePhase.RESULT
    state.acknowledge_adventure_result()
    assert state.adventure_phase == AdventurePhase.ADVANCING


def test_wounded_cultivator_can_award_qingyun_token_and_unlock_boss():
    state = GameState(
        level=12,
        stamina=100,
        tendencies=[0, 0, 0, 20],
        adventure_phase=AdventurePhase.CHOOSING,
        current_event=QingyunEvent.WOUNDED_CULTIVATOR,
    )

    state.resolve_qingyun_event(choice=0, seed=0)

    assert state.items[ItemType.QINGYUN_TOKEN] == 1
    assert state.qingyun_boss_unlocked


def test_abandoning_event_ends_adventure_without_extra_cost():
    state = GameState(
        energy=7,
        adventure_phase=AdventurePhase.CHOOSING,
        current_event=QingyunEvent.DEMON_BEAST,
    )

    state.abandon_qingyun_event()

    assert state.energy == 7
    assert state.adventure_phase == AdventurePhase.IDLE
```

- [ ] **Step 2: 运行目标测试并确认失败**

Run: `py -3 -m pytest .\host\game_model\test_progression.py -q`

Expected: FAIL，缺少事件触发、`resolve_qingyun_event()` 或结果确认接口。

- [ ] **Step 3: 实现固定检查点和确定性事件结果**

事件在进度首次跨过 `25/45/65/85` 时依次触发，使用 `qingyun_event_mask` 防止重复。实现 `EventResult`，将最近结果保存到 `current_event_result`，并遵守：

- 灵草：采集成功获得 1 灵草；离去无奖励。
- 妖兽：迎战结果受等级、体力、战斗倾向和 `seed % 100` 影响，可获得经验/灵石或损失体力；避开继续前进。
- 受伤修士：相助结果受等级、体力、亲和倾向影响，可获得回春丹或青云信物；无视继续前进。
- 山道捷径：冒险结果受等级、灵力、灵修倾向影响，可增加 8–15 进度或损失体力；稳行增加 3 进度。
- 所有资源使用 `min()`/`max()` 钳制，不产生永久负面状态。
- `acknowledge_adventure_result()` 将 `RESULT` 恢复为 `ADVANCING`；若已解锁首领则进入 `BOSS_READY`。

- [ ] **Step 4: 增加数量上限测试**

```python
def test_event_item_rewards_saturate_at_uint16_max():
    state = GameState(
        items=[65535, 0, 0, 0, 0],
        adventure_phase=AdventurePhase.CHOOSING,
        current_event=QingyunEvent.SPIRIT_HERB,
    )

    state.resolve_qingyun_event(choice=0, seed=0)

    assert state.items[ItemType.SPIRIT_HERB] == 65535
```

- [ ] **Step 5: 重跑规则测试并提交**

```powershell
py -3 -m pytest .\host\game_model\test_progression.py -q
git add host/game_model/progression.py host/game_model/test_progression.py
git commit -m "feat: add qingyun choice events"
```

Expected: PASS。

---

### Task 3: Python 青云妖狼自动战斗

**Files:**
- Modify: `host/game_model/progression.py`
- Modify: `host/game_model/test_progression.py`

- [ ] **Step 1: 写开战、逐回合灵力消耗和退出测试**

```python
def test_qingyun_wolf_requires_unlock_and_five_energy():
    locked = GameState(energy=20)
    ready = GameState(energy=4, qingyun_boss_unlocked=True)
    capable = GameState(energy=5, qingyun_boss_unlocked=True)

    assert not locked.start_qingyun_wolf_battle(False, False)
    assert not ready.start_qingyun_wolf_battle(False, False)
    assert capable.start_qingyun_wolf_battle(False, False)
    assert capable.in_battle
    assert capable.boss_hp == capable.boss_max_hp


def test_auto_battle_consumes_one_energy_each_round():
    state = GameState(energy=6, qingyun_boss_unlocked=True)
    assert state.start_qingyun_wolf_battle(False, False)

    result = state.tick_qingyun_wolf_battle(seed=50)

    assert state.energy == 5
    assert state.battle_round == 1
    assert result in (BattleResult.CONTINUE, BattleResult.VICTORY)


def test_zero_energy_retreats_and_resets_boss_hp():
    state = GameState(energy=1, qingyun_boss_unlocked=True)
    assert not state.start_qingyun_wolf_battle(False, False)
    state.energy = 5
    assert state.start_qingyun_wolf_battle(False, False)
    state.energy = 1

    assert state.tick_qingyun_wolf_battle(seed=99) == BattleResult.ENERGY_DEPLETED
    assert not state.in_battle
    assert state.boss_hp == state.boss_max_hp


def test_manual_retreat_resets_boss_without_other_penalty():
    state = GameState(energy=8, stamina=67, qingyun_boss_unlocked=True)
    assert state.start_qingyun_wolf_battle(False, False)
    state.boss_hp -= 5

    state.retreat_qingyun_wolf()

    assert not state.in_battle
    assert state.stamina == 67
    assert state.boss_hp == state.boss_max_hp
```

- [ ] **Step 2: 写倾向、形态、暴击、闪避和符箓测试**

```python
def test_attack_tendency_improves_damage_with_diminishing_returns():
    low = GameState(level=8, tendencies=[0, 0, 0, 0])
    high = GameState(level=8, tendencies=[40, 0, 0, 0])

    assert high.qingyun_attack_damage(seed=50) > low.qingyun_attack_damage(seed=50)
    high.tendencies[0] = 80
    second_gain = high.qingyun_attack_damage(seed=50)
    assert second_gain - low.qingyun_attack_damage(seed=50) < 40


def test_talismans_are_consumed_at_battle_start_and_modify_one_battle():
    state = GameState(
        energy=10,
        qingyun_boss_unlocked=True,
        items=[0, 0, 1, 1, 0],
    )

    assert state.start_qingyun_wolf_battle(True, True)
    assert state.items[ItemType.ATTACK_TALISMAN] == 0
    assert state.items[ItemType.GUARD_TALISMAN] == 0
    assert state.battle_attack_talisman
    assert state.battle_guard_talisman
```

- [ ] **Step 3: 实现固定公式**

增加 `BattleResult` 和以下规则，Python/C++ 必须逐项一致：

- 青云妖狼基础生命 `48`，基础攻击 `8`。
- 宠物基础伤害 `4 + level`。
- 战斗倾向：`min(8, tendency // 5)` 点伤害，暴击率 `5 + min(15, tendency // 4)`。
- 灵修倾向：`min(10, tendency // 4)` 点伤害，不降低每回合 1 灵力消耗。
- 稳健倾向：受到伤害减少 `min(5, tendency // 8)`，闪避率 `5 + min(15, tendency // 4)`。
- 亲和倾向：每回合恢复概率 `min(20, tendency // 3)`，成功恢复 `2` 体力。
- 形态只提供小幅路线加成：`RookieA/FinalA1/FinalA2` 增加 1–2 伤害，`RookieB/FinalB1/FinalB2` 减少 1–2 受伤；不改变灵力消耗。
- 攻击符最终伤害乘 `120%`，护身符最终受伤乘 `80%`，使用整数除法。
- `seed % 100` 控制暴击，`(seed // 100) % 100` 控制闪避，`(seed // 10000) % 100` 控制亲和恢复，确保测试可复现。
- 体力归零时战败，退出战斗并恢复到 `30`。
- 胜利奖励基础为 30 经验、25 灵石；重复胜利按已有右移减半规则，最低 1。

- [ ] **Step 4: 写胜负和重复奖励测试**

```python
def test_qingyun_wolf_victory_marks_defeat_and_halves_repeat_rewards():
    state = GameState(level=30, energy=20, stamina=100,
                      qingyun_boss_unlocked=True)
    rewards = []

    for _ in range(3):
        assert state.start_qingyun_wolf_battle(False, False)
        state.boss_hp = 1
        before = (state.experience, state.coins)
        assert state.tick_qingyun_wolf_battle(seed=50) == BattleResult.VICTORY
        rewards.append((state.experience - before[0], state.coins - before[1]))

    assert rewards == [(30, 25), (15, 12), (7, 6)]
```

- [ ] **Step 5: 运行测试并提交**

```powershell
py -3 -m pytest .\host\game_model\test_progression.py -q
git add host/game_model/progression.py host/game_model/test_progression.py
git commit -m "feat: add qingyun wolf auto battle"
```

Expected: PASS。

---

### Task 4: 固件存档结构同步

**Files:**
- Modify: `firmware/ai_pet/game_types.h`
- Modify: `firmware/ai_pet/game_state.cpp`
- Modify: `firmware/ai_pet/save_store.cpp`
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] **Step 1: 写固件结构失败测试**

在 `test_ui_refresh.py` 断言存在：

```python
def test_qingyun_state_is_persisted_in_v1_1_save_data():
    for token in (
        "enum class AdventurePhase",
        "enum class QingyunEvent",
        "uint8_t qingyunProgress;",
        "uint8_t qingyunEventMask;",
        "uint8_t qingyunBossUnlocked;",
        "AdventurePhase adventurePhase;",
        "QingyunEvent currentEvent;",
        "EventResult currentEventResult;",
        "uint8_t battleRound;",
        "uint8_t battleAttackTalisman;",
        "uint8_t battleGuardTalisman;",
    ):
        assert token in GAME_TYPES
```

- [ ] **Step 2: 运行目标测试并确认失败**

Run: `py -3 -m pytest .\host\game_model\test_ui_refresh.py::test_qingyun_state_is_persisted_in_v1_1_save_data -q`

Expected: FAIL。

- [ ] **Step 3: 替换旧三区域字段**

在 `PetSaveData` 中删除 `regionProgress[3]`、`activeRegion`、`battleRegion`、`bossDefeatedMask` 与 `bossWins[3]`，改为单个 `qingyunBossWins` 和 `qingyunBossDefeated`。加入 Task 1–3 所需字段，字段使用固定宽度整数。同步删除固件与源码测试中的所有旧字段引用，避免同一存档同时存在新旧两套区域状态。

- [ ] **Step 4: 提升存档版本**

将 `game_state.cpp` 与 `save_store.cpp` 的 `kSaveVersion` 从 `5` 升至 `6`。继续执行“不迁移旧存档”：版本或大小不匹配时初始化新角色。

- [ ] **Step 5: 更新结构测试并运行**

```powershell
py -3 -m pytest .\host\game_model\test_ui_refresh.py -q
git diff --check
```

Expected: PASS；无空白错误。

- [ ] **Step 6: 提交**

```powershell
git add firmware/ai_pet/game_types.h firmware/ai_pet/game_state.cpp firmware/ai_pet/save_store.cpp host/game_model/test_ui_refresh.py
git commit -m "feat: persist qingyun adventure state"
```

---

### Task 5: 固件青云山道规则镜像

**Files:**
- Modify: `firmware/ai_pet/game_state.h`
- Modify: `firmware/ai_pet/game_state.cpp`
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] **Step 1: 写固件 API 镜像测试**

```python
def test_firmware_exposes_qingyun_adventure_and_event_api():
    for signature in (
        "bool startQingyunAdventure();",
        "void stopQingyunAdventure();",
        "AdventureTick tickQingyunAdventure(uint32_t seed);",
        "EventResult resolveQingyunEvent(uint8_t choice, uint32_t seed);",
        "void acknowledgeAdventureResult();",
        "void abandonQingyunEvent();",
    ):
        assert signature in GAME_STATE_HEADER
```

- [ ] **Step 2: 删除旧 API 并实现新 API**

删除 `startExploration()`、`regionUnlocked()`、`tickExploration()`。按 Task 1–2 的字段、检查点、资源钳制和随机公式实现 C++ 镜像。

- [ ] **Step 3: 增加关键常量镜像断言**

测试源码包含事件检查点 `25, 45, 65, 85`、启动消耗 `3`、推进消耗 `1` 和 `uint16_t` 物品饱和处理。

- [ ] **Step 4: 运行完整 Python 测试**

Run: `py -3 -m pytest .\host\game_model .\host\hooks .\host\diagnostics -q`

Expected: PASS。

- [ ] **Step 5: 提交**

```powershell
git add firmware/ai_pet/game_state.h firmware/ai_pet/game_state.cpp host/game_model/test_ui_refresh.py
git commit -m "feat: mirror qingyun adventure rules in firmware"
```

---

### Task 6: 固件自动战斗规则镜像

**Files:**
- Modify: `firmware/ai_pet/game_state.h`
- Modify: `firmware/ai_pet/game_state.cpp`
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] **Step 1: 写自动战斗 API 失败测试**

```python
def test_firmware_exposes_qingyun_auto_battle_api():
    for signature in (
        "bool startQingyunWolfBattle(bool useAttackTalisman,",
        "BattleResult tickQingyunWolfBattle(uint32_t seed);",
        "void retreatQingyunWolf();",
    ):
        assert signature in GAME_STATE_HEADER
    assert "battleAction(" not in GAME_STATE_HEADER
```

- [ ] **Step 2: 实现与 Python 相同的伤害辅助函数**

在 `private:` 增加：

```cpp
uint8_t qingyunAttackDamage(uint32_t seed) const;
uint8_t qingyunIncomingDamage(uint32_t seed) const;
bool qingyunAffinityHeal(uint32_t seed) const;
void finishQingyunVictory();
```

按 Task 3 的所有公式实现；中间计算使用 `uint16_t`，写回生命值时钳制到字段范围。

- [ ] **Step 3: 删除手动战斗逻辑**

删除 `battleAction(uint8_t action)` 及攻击、法诀、丹药、防御分支。K4 撤退由 UI 调用 `retreatQingyunWolf()`。

- [ ] **Step 4: 运行规则与源码镜像测试**

Run: `py -3 -m pytest .\host\game_model\test_progression.py .\host\game_model\test_ui_refresh.py -q`

Expected: PASS。

- [ ] **Step 5: 提交**

```powershell
git add firmware/ai_pet/game_state.h firmware/ai_pet/game_state.cpp host/game_model/test_ui_refresh.py
git commit -m "feat: mirror qingyun auto battle in firmware"
```

---

### Task 7: 时间驱动与保存调度

**Files:**
- Modify: `firmware/ai_pet/game_app.h`
- Modify: `firmware/ai_pet/game_app.cpp`
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] **Step 1: 写运行循环失败测试**

```python
def test_game_app_ticks_adventure_and_battle_separately():
    assert "lastAdventureStepAt_" in APP_HEADER
    assert "lastBattleRoundAt_" in APP_HEADER
    assert "tickQingyunAdventure(" in APP_SOURCE
    assert "tickQingyunWolfBattle(" in APP_SOURCE
    assert "tickExploration(" not in APP_SOURCE
```

- [ ] **Step 2: 定义两个固定节奏**

在 `game_app.cpp` 增加：

```cpp
constexpr uint32_t kAdventureStepMs = 3000;
constexpr uint32_t kBattleRoundMs = 1200;
```

历练只在 `AdventurePhase::Advancing` 时每 3 秒推进；战斗只在 `inBattle` 时每 1.2 秒结算。每次状态变化调用 `requestSave()` 并强制刷新 UI。

- [ ] **Step 3: 处理结果通知**

根据 `AdventureTick`/`BattleResult` 显示简短通知：

- 事件触发：进入事件选择，不继续推进。
- 灵力耗尽：显示“灵力耗尽”并留在青云山道页面。
- 首领解锁：显示“妖狼现踪”。
- 胜利、失败、撤退：进入独立结果状态，由 K1 返回青云山道。

- [ ] **Step 4: 运行测试并提交**

```powershell
py -3 -m pytest .\host\game_model\test_ui_refresh.py -q
git add firmware/ai_pet/game_app.h firmware/ai_pet/game_app.cpp host/game_model/test_ui_refresh.py
git commit -m "feat: drive qingyun adventure and battle ticks"
```

Expected: PASS。

---

### Task 8: 青云山道与事件 UI

**Files:**
- Modify: `firmware/ai_pet/game_ui.h`
- Modify: `firmware/ai_pet/game_ui.cpp`
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] **Step 1: 写页面状态与控制测试**

断言：

- 首页 K3 进入青云山道。
- 空闲页 K1 开始或进入战前确认，K4 返回首页。
- 自动前进时无操作菜单，K4 结束历练。
- 事件页 K2/K3 切换两个选项，K1 确认，K4 放弃并结束历练。
- 事件结果页 K1 继续。

- [ ] **Step 2: 拆分绘制函数**

在 `game_ui.h` 增加：

```cpp
void drawQingyunAdventure(const PetSaveData& data, uint32_t now);
void drawQingyunEvent(const PetSaveData& data);
void drawQingyunEventResult(const PetSaveData& data);
void drawQingyunBossPrompt(const PetSaveData& data);
void drawQingyunScene(const PetSaveData& data, uint32_t now);
```

删除旧三区域卡片绘制。

- [ ] **Step 3: 实现无正式素材的占位场景**

上半屏只使用：

- 深青色渐层背景色块。
- 三条斜线表示山道。
- 圆和三角形表示远山、灵草、修士和妖兽。
- 现有 `PetRenderer` 绘制宠物，并按 `now` 做 2px 上下行走摆动。

禁止新增生成图片或 `assets/` 头文件；所有占位绘制集中在 `drawQingyunScene()`，后续素材替换只改此函数及新增资源引用。

- [ ] **Step 4: 实现下半屏信息**

显示：

- “青云山道”
- 进度条与百分比
- 灵力、体力
- 预计可推进次数：当前灵力
- 当前状态：前行、抉择、妖狼已现、灵力耗尽

- [ ] **Step 5: 更新源码测试并提交**

```powershell
py -3 -m pytest .\host\game_model\test_ui_refresh.py -q
git add firmware/ai_pet/game_ui.h firmware/ai_pet/game_ui.cpp host/game_model/test_ui_refresh.py
git commit -m "feat: add qingyun adventure event ui"
```

Expected: PASS。

---

### Task 9: 战前确认、自动战斗与结果 UI

**Files:**
- Modify: `firmware/ai_pet/game_types.h`
- Modify: `firmware/ai_pet/game_ui.h`
- Modify: `firmware/ai_pet/game_ui.cpp`
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] **Step 1: 写战斗 UI 失败测试**

断言：

- 战前页可分别开关攻击符与护身符，库存为 0 时不可启用。
- K1 确认开战，至少需要 5 灵力。
- 战斗中除 K4 撤退外不接受出招。
- 页面显示双方血条、回合、灵力和最近一次暴击/闪避/恢复提示。
- 战斗结束进入胜利、失败或灵力耗尽结果页。

- [ ] **Step 2: 增加瞬时 UI 状态**

`GameUi` 保存 `useAttackTalisman_`、`useGuardTalisman_`、`battleResult_` 和最近一回合表现标记。符箓选择在离开战前页时清零，不写入永久存档；实际是否生效由战斗开始时写入 `PetSaveData` 的本场字段。

- [ ] **Step 3: 实现几何妖狼占位**

使用圆角矩形、三角耳、线段尾巴和两点眼睛绘制妖狼。攻击、受击、暴击、闪避动画只做位置偏移、闪烁和短线冲击效果，不创建正式素材。

- [ ] **Step 4: 替换旧四键出招页面**

删除“攻击/法诀/丹药/防御”四块按钮。战斗页底部只显示“自动交锋”和“K4撤退”，每个回合由 `GameApp` 驱动。

- [ ] **Step 5: 运行测试并提交**

```powershell
py -3 -m pytest .\host\game_model\test_ui_refresh.py -q
git add firmware/ai_pet/game_types.h firmware/ai_pet/game_ui.h firmware/ai_pet/game_ui.cpp host/game_model/test_ui_refresh.py
git commit -m "feat: add qingyun auto battle ui"
```

Expected: PASS。

---

### Task 10: 文档、全量验证与编译

**Files:**
- Modify: `README.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: 更新操作文档**

README 写明：

- K3 进入青云山道。
- 自动前进与每周期 1 灵力。
- 事件 K2/K3 选择、K1 确认、K4 放弃。
- 青云妖狼自动战斗、每回合 1 灵力、K4 撤退。
- 攻击符与护身符在战前选择。

- [ ] **Step 2: 更新 V1.1 进度**

在 `CHANGELOG.md` 增加青云山道、四种事件、青云信物提前解锁、自动战斗、倾向加成和符箓效果。注明正式场景与妖狼美术尚待素材替换，不将占位图形描述为最终美术。

- [ ] **Step 3: 运行完整 Python 测试**

Run: `py -3 -m pytest .\host\game_model .\host\hooks .\host\diagnostics -q`

Expected: 全部 PASS。

- [ ] **Step 4: 编译固件**

Run: `powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1`

Expected: 编译成功，Flash/RAM 未超限。

- [ ] **Step 5: 检查差异**

```powershell
git diff --check
git status --short
```

Expected: `git diff --check` 无输出；工作区只包含 README 和 CHANGELOG 的待提交修改。

- [ ] **Step 6: 提交**

```powershell
git add README.md CHANGELOG.md
git commit -m "docs: document qingyun adventure"
```

---

## 实机验收留项

正式代码与编译通过后再烧录，不把以下项目阻塞在桌面开发阶段：

- 青云山道 3 秒推进节奏是否舒适。
- 事件选择字体和 128×160 屏幕可读性。
- 自动战斗 1.2 秒回合节奏与动画闪烁。
- 灵力耗尽、战败恢复 30 体力、K4 撤退。
- 攻击符和护身符各消耗 1 个且只影响当前战斗。
- 断电后进度、事件标记、信物、首领胜场和功德簿保留。
- 用户提供正式素材后替换场景与妖狼几何占位，不修改规则或存档字段。
